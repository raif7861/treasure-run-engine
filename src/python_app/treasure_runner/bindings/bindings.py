"""
 Low-level ctypes bindings

This module provides direct ctypes access to the C library functions.
It handles:
  - Loading the shared library
  - Defining C enums and structures
  - Wrapping C function signatures
  - Managing error codes from the C layer

This is a thin layer - no error handling or convenience wrappers.
All error handling is done in the models layer.
"""

import ctypes
import os
from enum import IntEnum
from pathlib import Path


# ============================================================
# Enums matching C definitions
# ============================================================

class Direction(IntEnum):
    """Movement directions (matches DIR_* in types.h)."""
    NORTH = 0
    SOUTH = 1
    EAST = 2
    WEST = 3


class Status(IntEnum):
    """Status codes for room and player operations."""
    OK = 0
    INVALID_ARGUMENT = 1
    NULL_POINTER = 2
    NO_MEMORY = 3
    BOUNDS_EXCEEDED = 4
    INTERNAL_ERROR = 5
    ROOM_IMPASSABLE = 6
    ROOM_NO_PORTAL = 7
    ROOM_NOT_FOUND = 8
    GE_NO_SUCH_ROOM = 9
    WL_ERR_CONFIG = 10
    WL_ERR_DATAGEN = 11

# Backwards compatibility for existing imports
GameEngineStatus = Status


# ============================================================
# C Structures - Opaque types only
# ============================================================

# Treasure is used by player_get_collected_treasures
class Treasure(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_int),
        ("name", ctypes.c_char_p),
        ("starting_room_id", ctypes.c_int),
        ("initial_x", ctypes.c_int),
        ("initial_y", ctypes.c_int),
        ("x", ctypes.c_int),
        ("y", ctypes.c_int),
        ("collected", ctypes.c_bool),
    ]

# ============================================================
# Opaque pointer types for GameEngine and Player
# ============================================================

GameEngine = ctypes.c_void_p
Player = ctypes.c_void_p


# ============================================================
# Library Loading
# ============================================================

def _find_library():
    """Locate libbackend.so under the project dist directory."""
    # Optional override via env
    env_path = os.getenv("TREASURE_RUNNER_DIST")
    candidates = []

    if env_path:
        candidates.append(Path(env_path) / "libbackend.so")
        candidates.append(Path(env_path) / "libpuzzlegen.so")

    # Project-relative: ../../dist relative to this file
    here = Path(__file__).resolve()
    repo_root = here.parent.parent.parent.parent
    candidates.append(repo_root / "dist" / "libbackend.so")
    candidates.append(repo_root / "dist" / "libpuzzlegen.so")
    # ADDED for local development without dist build - look directly in c/lib
    candidates.append(repo_root / "c" / "lib" / "libbackend.so")
    candidates.append(repo_root / "c" / "lib" / "libpuzzlegen.so")

    backend_path = None
    puzzlegen_path = None

    for path in candidates:
        if path.exists():
            if path.name == "libbackend.so" and backend_path is None:
                backend_path = path
            if path.name == "libpuzzlegen.so" and puzzlegen_path is None:
                puzzlegen_path = path

    if backend_path is not None:
        # Ensure puzzlegen is loaded first if present to satisfy dependencies.
        if puzzlegen_path is not None:
            ctypes.CDLL(str(puzzlegen_path))
        return str(backend_path)

    tried = "\n".join(str(p) for p in candidates)
    raise RuntimeError(f"libbackend.so not found. Paths tried:\n{tried}")


# Load the library
_LIB_PATH = _find_library()
lib = ctypes.CDLL(_LIB_PATH)


# ============================================================
# C Function Signatures
# ============================================================

# ============================================================
#Game Engine function signatures
# ============================================================
lib.game_engine_create.argtypes = [ctypes.c_char_p, ctypes.POINTER(GameEngine)]
lib.game_engine_create.restype = ctypes.c_int

lib.game_engine_destroy.argtypes = [GameEngine]
lib.game_engine_destroy.restype = None

lib.game_engine_get_player.argtypes = [GameEngine]
lib.game_engine_get_player.restype = Player

lib.game_engine_move_player.argtypes = [GameEngine, ctypes.c_int]
lib.game_engine_move_player.restype = ctypes.c_int

lib.game_engine_render_current_room.argtypes = [GameEngine, ctypes.POINTER(ctypes.c_char_p)]
lib.game_engine_render_current_room.restype = ctypes.c_int

lib.game_engine_render_room.argtypes = [
    GameEngine,
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_char_p),
]

lib.game_engine_render_room.restype = ctypes.c_int

lib.game_engine_get_room_count.argtypes = [GameEngine, ctypes.POINTER(ctypes.c_int)]
lib.game_engine_get_room_count.restype = ctypes.c_int

lib.game_engine_get_room_dimensions.argtypes = [GameEngine, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
lib.game_engine_get_room_dimensions.restype = ctypes.c_int

lib.game_engine_get_room_ids.argtypes = [GameEngine, ctypes.POINTER(ctypes.POINTER(ctypes.c_int)), ctypes.POINTER(ctypes.c_int)]
lib.game_engine_get_room_ids.restype = ctypes.c_int

lib.game_engine_reset.argtypes = [GameEngine]
lib.game_engine_reset.restype = ctypes.c_int

lib.game_engine_free_string.argtypes = [ctypes.c_void_p]
lib.game_engine_free_string.restype = None


# ============================================================
# C Function Signatures - Player
# ============================================================
lib.player_get_room.argtypes = [Player]
lib.player_get_room.restype = ctypes.c_int

lib.player_get_position.argtypes = [Player, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
lib.player_get_position.restype = ctypes.c_int

lib.player_get_collected_count.argtypes = [Player]
lib.player_get_collected_count.restype = ctypes.c_int

lib.player_has_collected_treasure.argtypes = [Player, ctypes.c_int]
lib.player_has_collected_treasure.restype = ctypes.c_bool

lib.player_get_collected_treasures.argtypes = [Player, ctypes.POINTER(ctypes.c_int)]
lib.player_get_collected_treasures.restype = ctypes.POINTER(ctypes.POINTER(Treasure))

lib.player_reset_to_start.argtypes = [Player, ctypes.c_int, ctypes.c_int, ctypes.c_int]
lib.player_reset_to_start.restype = ctypes.c_int

# ============================================================
# C Function Signatures - Room
# ============================================================

# Room is opaque - no direct room accessors exposed to Python
Room = ctypes.c_void_p


# ============================================================
# Memory Management
# ============================================================

lib.destroy_treasure.argtypes = [ctypes.POINTER(Treasure)]
lib.destroy_treasure.restype = None

# game_engine_get_total_treasure_count - added for A3 collect-all-treasure feature
lib.game_engine_get_total_treasure_count.argtypes = [GameEngine, ctypes.POINTER(ctypes.c_int)]
lib.game_engine_get_total_treasure_count.restype = ctypes.c_int


# game_engine_get_adjacency_matrix - added for A3 enhanced UI minimap feature
lib.game_engine_get_adjacency_matrix.argtypes = [
    GameEngine,
    ctypes.POINTER(ctypes.POINTER(ctypes.c_int)),
    ctypes.POINTER(ctypes.POINTER(ctypes.c_int)),
    ctypes.POINTER(ctypes.c_int),
]
lib.game_engine_get_adjacency_matrix.restype = ctypes.c_int
