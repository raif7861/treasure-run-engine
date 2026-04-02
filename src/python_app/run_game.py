import sys
import os
import argparse
import curses

"""
run_game.py - Entry point for Treasure Runner A3.
 
Collects command line arguments and launches the curses UI.
Uses the same argparse mechanism as run_integration.py from A2.
 
Usage like in A3 descirption:
    python3 run_game.py --config <path_to_ini> --profile <path_to_profile>
"""
# Adjust system path so treasure_runner package is importable
# when this script is run directly from the python/ directory
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
 
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.ui.game_ui import GameUI
 
def parse_args():
    #Parse command line arguments
    parser = argparse.ArgumentParser(description="Treasure Runner - A3")
    parser.add_argument(
        "--config",
        required=True,
        help="Path to the world generator .ini config file",
    )
    parser.add_argument(
        "--profile",
        required=True,
        help="Path to the player profile JSON file",
    )
    return parser.parse_args()

def main():
    #MAIN entry point - parse args and launch the game UI.
    args = parse_args()
 
    config_path = os.path.abspath(args.config)
    profile_path = os.path.abspath(args.profile)
 
    # Launch the game inside the curses wrapper for safe setup and cleanup
    curses.wrapper(run, config_path, profile_path)
 
 
def run(stdscr, config_path, profile_path):
    #Run the game with the given curses screen.
    with GameEngine(config_path) as engine:
        ui = GameUI(stdscr, engine, profile_path)
        ui.run()
 
 
if __name__ == "__main__":
    main()
 