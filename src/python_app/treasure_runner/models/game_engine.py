"""
High-level GameEngine wrapper.

This wraps the low-level ctypes bindings and exposes a Pythonic API
for interacting with the Treasure Runner engine.
"""

import ctypes

from ..bindings.bindings import GameEngine as CGameEngine
from ..bindings.bindings import lib
from .exceptions import status_to_exception
from .player import Player


class GameEngine:
    """Python wrapper around the C GameEngine."""

    def __init__(self, config_path):
        """
        Create a new engine from a config path.

        config_path:
            path to the .ini config file
        """
        if config_path is None:
            raise ValueError("config_path must not be None")

        self._eng = CGameEngine()
        self._player = None

        config_bytes = str(config_path).encode("utf-8")
        status = lib.game_engine_create(config_bytes, ctypes.byref(self._eng))
        if status != 0:
            raise status_to_exception(status, "failed to create game engine")

        player_ptr = lib.game_engine_get_player(self._eng)
        if not player_ptr:
            lib.game_engine_destroy(self._eng)
            self._eng = CGameEngine()
            raise RuntimeError("game engine returned null player pointer")

        self._player = Player(player_ptr)

    @property
    def player(self):
        """Return the Player wrapper for this engine."""
        return self._player

    def destroy(self):
        """Destroy the underlying C engine if it still exists."""
        if self._eng:
            lib.game_engine_destroy(self._eng)
            self._eng = CGameEngine()
            self._player = None

    def close(self):
        """Compatibility alias for destroy()."""
        self.destroy()

    def __del__(self):
        """Best-effort cleanup."""
        try:
            self.destroy()
        except Exception:
            pass

    def __enter__(self):
        """Support with-statement usage."""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Clean up on context exit."""
        self.destroy()
        return False

    def get_player(self):
        """Return a Player wrapper for the engine's current player."""
        if self._player is None:
            raise RuntimeError("game engine has no player")
        return self._player

    def move_player(self, direction):
        """
        Move the player in the given direction.

        direction may be:
        - a Direction enum value
        - an int
        """
        status = lib.game_engine_move_player(self._eng, int(direction))
        if status != 0:
            raise status_to_exception(status, "failed to move player")

    def render_current_room(self):
        """Render the current room and return it as a Python string."""
        str_out = ctypes.c_char_p()

        status = lib.game_engine_render_current_room(
            self._eng,
            ctypes.byref(str_out),
        )
        if status != 0:
            raise status_to_exception(status, "failed to render current room")

        try:
            if not str_out.value:
                return ""
            return str_out.value.decode("utf-8")
        finally:
            if str_out:
                lib.game_engine_free_string(ctypes.cast(str_out, ctypes.c_void_p))

    def render_room(self, room_id):
        """Render a room by id and return it as a Python string."""
        str_out = ctypes.c_char_p()

        status = lib.game_engine_render_room(
            self._eng,
            int(room_id),
            ctypes.byref(str_out),
        )
        if status != 0:
            raise status_to_exception(status, "failed to render room")

        try:
            if not str_out.value:
                return ""
            return str_out.value.decode("utf-8")
        finally:
            if str_out:
                lib.game_engine_free_string(ctypes.cast(str_out, ctypes.c_void_p))

    def get_room_count(self):
        """Return the total number of rooms."""
        count_out = ctypes.c_int()

        status = lib.game_engine_get_room_count(
            self._eng,
            ctypes.byref(count_out),
        )
        if status != 0:
            raise status_to_exception(status, "failed to get room count")

        return count_out.value

    def get_room_dimensions(self):
        """Return the current room dimensions as (width, height)."""
        width_out = ctypes.c_int()
        height_out = ctypes.c_int()

        status = lib.game_engine_get_room_dimensions(
            self._eng,
            ctypes.byref(width_out),
            ctypes.byref(height_out),
        )
        if status != 0:
            raise status_to_exception(status, "failed to get room dimensions")

        return (width_out.value, height_out.value)

    def get_room_ids(self):
        """
        Return all room ids as a Python list.

        The C layer allocates the ids array.
        We copy it into Python immediately.
        """
        ids_ptr = ctypes.POINTER(ctypes.c_int)()
        count_out = ctypes.c_int()

        status = lib.game_engine_get_room_ids(
            self._eng,
            ctypes.byref(ids_ptr),
            ctypes.byref(count_out),
        )
        if status != 0:
            raise status_to_exception(status, "failed to get room ids")

        count = count_out.value
        room_ids = []

        try:
            if ids_ptr:
                for idx in range(count):
                    room_ids.append(ids_ptr[idx])
        finally:
            if ids_ptr:
                lib.game_engine_free_string(ctypes.cast(ids_ptr, ctypes.c_void_p))

        return room_ids

    def reset(self):
        """Resets the engine to its initial state."""
        status = lib.game_engine_reset(self._eng)
        if status != 0:
            raise status_to_exception(status, "failed to reset game engine")

    def get_total_treasure_count(self):
        """Return the total number of treasures in the entire world."""
        count_out = ctypes.c_int()
        status = lib.game_engine_get_total_treasure_count(
            self._eng,
            ctypes.byref(count_out),
        )
        if status != 0:
            raise status_to_exception(status, "failed to get total treasure count")
        return count_out.value
    def get_adjacency_matrix(self):
        """Return adjacency matrix and room ids for minimap rendering.

        Returns (matrix, ids, n) where matrix is a flat list of n*n ints,
        ids is a list of n room ids, and n is the number of rooms.
        Caller must not modify the lists - they are copied from C memory.
        """
        matrix_ptr = ctypes.POINTER(ctypes.c_int)()
        ids_ptr = ctypes.POINTER(ctypes.c_int)()
        n_out = ctypes.c_int()

        status = lib.game_engine_get_adjacency_matrix(
            self._eng,
            ctypes.byref(matrix_ptr),
            ctypes.byref(ids_ptr),
            ctypes.byref(n_out),
        )
        if status != 0:
            raise status_to_exception(status, "failed to get adjacency matrix")

        num_rooms = n_out.value
        try:
            matrix = [matrix_ptr[i] for i in range(num_rooms * num_rooms)]
            ids = [ids_ptr[i] for i in range(num_rooms)]
        finally:
            lib.game_engine_free_string(ctypes.cast(matrix_ptr, ctypes.c_void_p))
            lib.game_engine_free_string(ctypes.cast(ids_ptr, ctypes.c_void_p))

        return matrix, ids, num_rooms
