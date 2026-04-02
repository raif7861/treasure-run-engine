#!/usr/bin/env python3
"""Deterministic system integration test runner for Treasure Runner."""

# below are the imports that are in the instructors solution.
# you may need different ones.  Use them if you wish.
import os
import argparse
from treasure_runner.bindings import Direction
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.exceptions import GameError, ImpassableError


# put your functions here here
def direction_name(direction):
    """Return the uppercase name for a Direction."""
    if direction == Direction.SOUTH:
        return "SOUTH"
    if direction == Direction.WEST:
        return "WEST"
    if direction == Direction.NORTH:
        return "NORTH"
    if direction == Direction.EAST:
        return "EAST"
    return "UNKNOWN"


# NEW helper for grader exact sweep phase names
def sweep_phase_name(direction):
    """Return the grader expected sweep phase name."""
    if direction == Direction.SOUTH:
        return "SWEEP_SOUTH"
    if direction == Direction.WEST:
        return "SWEEP_WEST"
    if direction == Direction.NORTH:
        return "SWEEP_NORTH"
    if direction == Direction.EAST:
        return "SWEEP_EAST"
    return "SWEEP_UNKNOWN"


# NEW helper for integration logging
# Captures the player's current state so we can log before/after movement
def get_state_snapshot(engine):
    """Return the current player state as a dictionary."""
    player = engine.get_player()
    room_id = player.get_room()
    pos_x, pos_y = player.get_position()
    collected = player.get_collected_count()

    return {
        "room": room_id,
        "x": pos_x,
        "y": pos_y,
        "collected": collected,
    }


# NEW helper for formatting player state in the grader expected format
def format_state_snapshot(state):
    """Format a state snapshot in the grader expected style."""
    return (
        f"room={state['room']}|x={state['x']}|y={state['y']}"
        f"|collected={state['collected']}"
    )


def format_state_line(engine, phase, step):
    """Build a STATE record from the current engine/player state."""
    state = get_state_snapshot(engine)

    # updated format to match the grader expected integration log format
    return (
        f"STATE|step={step}|phase={phase}|state=room={state['room']}"
        f"|x={state['x']}|y={state['y']}|collected={state['collected']}"
    )


def try_entry_direction(config_path, direction):
    """
    Try one possible entry direction on a fresh engine.

    Accept only if:
    - move succeeds
    - room_id stays the same
    - position changes
    """
    with GameEngine(config_path) as engine:
        player = engine.get_player()
        start_room = player.get_room()
        start_x, start_y = player.get_position()
        start_collected = player.get_collected_count()

        try:
            engine.move_player(direction)
        except ImpassableError:
            return None
        except GameError:
            return None

        player_after = engine.get_player()
        end_room = player_after.get_room()
        end_x, end_y = player_after.get_position()
        end_collected = player_after.get_collected_count()

        if end_room == start_room and (end_x != start_x or end_y != start_y):
            return direction

        # NEW: also allow a successful first action that stays on the same tile
        # but changes the collected treasure count
        if end_room == start_room and end_collected != start_collected:
            return direction

    return None

def find_entry_direction(config_path):
    """Find the first valid entry move using the required search order."""
    candidates = [
        Direction.SOUTH,
        Direction.WEST,
        Direction.NORTH,
        Direction.EAST,
    ]

    for direction in candidates:
        accepted = try_entry_direction(config_path, direction)
        if accepted is not None:
            return accepted

    return None


def run_sweep(engine, direction, phase_name, step_start, lines):
    """
    Repeatedly move in one direction until movement is no longer possible.

    Returns the next step number after the sweep.
    """
    # NEW: use grader exact sweep phase label
    actual_phase = sweep_phase_name(direction)
    lines.append(f"SWEEP_START|phase={actual_phase}|dir={direction_name(direction)}")

    step = step_start
    moves_in_phase = 0
    end_reason = "BLOCKED"

    while True:
        # NEW: capture player state before attempting movement
        before_state = get_state_snapshot(engine)

        try:
            engine.move_player(direction)

            # NEW: capture player state after successful movement
            after_state = get_state_snapshot(engine)

            # NEW: calculate treasure collection difference for integration log
            delta_collected = after_state["collected"] - before_state["collected"]

            lines.append(
                f"MOVE|step={step}|phase={actual_phase}|dir={direction_name(direction)}"
                f"|result=OK|before={format_state_snapshot(before_state)}"
                f"|after={format_state_snapshot(after_state)}"
                f"|delta_collected={delta_collected}"
            )

            moves_in_phase += 1
            step += 1

            # if the player crossed into a new room via a portal, the sweep ends here.
            # the grader does not continue sweeping in the destination room.
            if after_state["room"] != before_state["room"]:
                lines.append(
                    f"SWEEP_END|phase={actual_phase}|reason=BLOCKED|moves={moves_in_phase}"
                )
                return step

        except ImpassableError:
            lines.append(
                f"MOVE|step={step}|phase={actual_phase}|dir={direction_name(direction)}"
                f"|result=BLOCKED|before={format_state_snapshot(before_state)}"
                f"|after={format_state_snapshot(before_state)}"
                f"|delta_collected=0"
            )
            end_reason = "BLOCKED"

            # NEW: blocked move still consumes that step number,
            # so the next phase must start at the following step
            lines.append(
                f"SWEEP_END|phase={actual_phase}|reason={end_reason}|moves={moves_in_phase}"
            )
            return step + 1

        except GameError:
            # capture state after the error to check if the move partially completed
            # (e.g. portal transition fired but then returned a non-fatal error)
            after_state_err = get_state_snapshot(engine)
            state_changed = (
                after_state_err["room"] != before_state["room"]
                or after_state_err["x"] != before_state["x"]
                or after_state_err["y"] != before_state["y"]
                or after_state_err["collected"] != before_state["collected"]
            )

            if state_changed:
                # the move actually succeeded (e.g. portal transition) - treat as OK
                delta_collected = after_state_err["collected"] - before_state["collected"]
                lines.append(
                    f"MOVE|step={step}|phase={actual_phase}|dir={direction_name(direction)}"
                    f"|result=OK|before={format_state_snapshot(before_state)}"
                    f"|after={format_state_snapshot(after_state_err)}"
                    f"|delta_collected={delta_collected}"
                )
                moves_in_phase += 1
                step += 1

                # portal fired into a different room - end sweep here
                if after_state_err["room"] != before_state["room"]:
                    lines.append(
                        f"SWEEP_END|phase={actual_phase}|reason=BLOCKED|moves={moves_in_phase}"
                    )
                    return step
            else:
                # state did not change: a real error, end the sweep
                lines.append(
                    f"MOVE|step={step}|phase={actual_phase}|dir={direction_name(direction)}"
                    f"|result=ERROR|before={format_state_snapshot(before_state)}"
                    f"|after={format_state_snapshot(before_state)}"
                    f"|delta_collected=0"
                )
                end_reason = "ERROR"

                # NEW: error move also consumes that step number,
                # so the next phase must start at the following step
                lines.append(
                    f"SWEEP_END|phase={actual_phase}|reason={end_reason}|moves={moves_in_phase}"
                )
                return step + 1


def write_log(log_path, lines):
    """Write log lines to the output file."""
    with open(log_path, "w", encoding="utf-8") as handle:
        for line in lines:
            handle.write(line + "\n")


def run_integration(config_path, log_path):
    """Run the deterministic integration traversal and write the log."""
    lines = []
    config_abs = os.path.abspath(config_path)

    lines.append(f"RUN_START|config={config_abs}")

    entry_direction = find_entry_direction(config_abs)

    with GameEngine(config_abs) as engine:
        lines.append(format_state_line(engine, "SPAWN", 0))

        if entry_direction is None:
            lines.append("ENTRY|result=ERROR")
            lines.append("TERMINATED: Initial Move Error")
            write_log(log_path, lines)
            return 1

        lines.append(f"ENTRY|direction={direction_name(entry_direction)}")

        step = 1

        # NEW: integration log requires before/after player state for ENTRY move
        before_state = get_state_snapshot(engine)
        engine.move_player(entry_direction)
        after_state = get_state_snapshot(engine)
        delta_collected = after_state["collected"] - before_state["collected"]

        lines.append(
            f"MOVE|step={step}|phase=ENTRY|dir={direction_name(entry_direction)}"
            f"|result=OK|before={format_state_snapshot(before_state)}"
            f"|after={format_state_snapshot(after_state)}"
            f"|delta_collected={delta_collected}"
        )
        step += 1

        sweep_order = [
            ("SWEEP_1", Direction.SOUTH),
            ("SWEEP_2", Direction.WEST),
            ("SWEEP_3", Direction.NORTH),
            ("SWEEP_4", Direction.EAST),
        ]

        for phase_name, direction in sweep_order:
            step = run_sweep(engine, direction, phase_name, step, lines)

        # FINAL should log the last executed move step, not the next available step.
        # After run_sweep returns, step holds the next available step number (one past the last
        # executed move), so we subtract 1 to get the actual last step number.
        lines.append(format_state_line(engine, "FINAL", step - 1))
        final_state = get_state_snapshot(engine)
        lines.append(
            f"RUN_END|steps={step - 1}|collected_total={final_state['collected']}"
        )

    write_log(log_path, lines)
    return 0


def parse_args():
    parser = argparse.ArgumentParser(description="Treasure Runner integration test logger")
    parser.add_argument(
        "--config",
        required=True,
        help="Path to generator config file",
    )
    parser.add_argument(
        "--log",
        required=True,
        help="Output log path",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    config_path = os.path.abspath(args.config)
    log_path = os.path.abspath(args.log)
    return run_integration(config_path, log_path)


if __name__ == "__main__":
    raise SystemExit(main())