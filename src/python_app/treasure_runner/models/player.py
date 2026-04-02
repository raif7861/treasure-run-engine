"""
High-level Player wrapper.

This wraps the low-level ctypes Player pointer and exposes
simple Python-friendly methods for reading player state.

The Player is owned by the GameEngine.
This wrapper must NOT destroy the underlying C player.
"""

import ctypes

from ..bindings.bindings import lib
from .exceptions import status_to_exception


class Player:
    """Python wrapper around a C Player pointer."""

    def __init__(self, ptr):
        """Store a borrowed C player pointer."""
        if not ptr:
            raise ValueError("ptr must not be null")
        self._ptr = ptr

    def get_room(self):
        """Return the player's current room id."""
        return lib.player_get_room(self._ptr)

    def get_room_id(self):
        """Compatibility alias for the player's current room id."""
        return self.get_room()

    def get_position(self):
        """Return the player's current (x, y) position."""
        x_out = ctypes.c_int()
        y_out = ctypes.c_int()

        status = lib.player_get_position(
            self._ptr,
            ctypes.byref(x_out),
            ctypes.byref(y_out),
        )
        if status != 0:
            raise status_to_exception(status, "failed to get player position")

        return (x_out.value, y_out.value)

    def get_collected_count(self):
        """Return the number of collected treasures."""
        return lib.player_get_collected_count(self._ptr)

    def has_collected_treasure(self, treasure_id):
        """Return True if the player has collected the given treasure id."""
        return bool(lib.player_has_collected_treasure(self._ptr, int(treasure_id)))

    def get_collected_treasures(self):
        """
        Return collected treasures as a list of dictionaries.

        The underlying treasure pointers are borrowed from the C layer.
        We only read their data here.
        """
        count_out = ctypes.c_int()

        treasure_array_ptr = lib.player_get_collected_treasures(
            self._ptr,
            ctypes.byref(count_out),
        )

        treasures = []
        count = count_out.value

        if not treasure_array_ptr:
            return treasures

        for idx in range(count):
            treasure_ptr = treasure_array_ptr[idx]
            if not treasure_ptr:
                continue

            treasure = treasure_ptr.contents

            name = ""
            if treasure.name:
                name = treasure.name.decode("utf-8")

            treasures.append(
                {
                    "id": treasure.id,
                    "name": name,
                    "starting_room_id": treasure.starting_room_id,
                    "initial_x": treasure.initial_x,
                    "initial_y": treasure.initial_y,
                    "x": treasure.x,
                    "y": treasure.y,
                    "collected": bool(treasure.collected),
                }
            )

        return treasures
        