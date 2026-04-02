#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "types.h"   /* Direction, Status */

typedef struct Graph Graph;
typedef struct Player Player;

/* ============================================================
 * GameEngine - Main Game Controller
 *
 * The GameEngine orchestrates the entire game state:
 *   • Owns the connectivity Graph (world topology)
 *   • Indirectly owns all Rooms via the Graph payloads
 *   • Owns the Player entity
 *   • Owns the Charset used for rendering
 *   • Implements player movement, interaction, and room transitions
 *
 * All internal state is opaque to Python.
 * Only this public API is exposed via ctypes in later assignments.
 *
 * The Graph and Room structures are never exposed directly.
 * ============================================================ */

/* ============================================================
 * Opaque GameEngine type
 *
 * Python receives a pointer to this structure but cannot
 * inspect or modify its contents.
 * ============================================================ */

typedef struct GameEngine {
    Graph  *graph;              /* Owns all Room payloads */
    Player *player;             /* Current player state */
    Charset charset;            /* Rendering characters */

    int initial_room_id;
    int initial_player_x;
    int initial_player_y;

    int room_count;             /* Cached number of rooms */
} GameEngine;

/* ============================================================
 * Creation & Destruction
 * ============================================================ */

/*
 * game_engine_create
 * ------------------
 * Build the entire game state by:
 *   1. Loading the world using the world loader
 *   2. Creating a connectivity graph of rooms
 *   3. Loading the rendering charset
 *   4. Creating and placing the player in the starting room
 *
 * Parameters:
 *   config_path:
 *     Path to the world configuration file (must not be NULL).
 *
 *   engine_out:
 *     On success, receives an owning pointer to a newly created GameEngine.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are NULL
 *   WL_ERR_DATAGEN if world loading fails
 *   NO_MEMORY on allocation failure
 *
 * Postconditions:
 *   On success, caller owns the engine and must call game_engine_destroy().
 */
Status game_engine_create(const char *config_file_path, GameEngine **engine_out);

/*
 * game_engine_destroy
 * -------------------
 * Destroy the game engine and all owned state.
 *
 * This frees:
 *   • The player
 *   • The connectivity graph
 *   • All rooms owned by the graph
 */
void game_engine_destroy(GameEngine *eng);

/* ============================================================
 * Player Access
 * ============================================================ */

/*
 * Retrieve the current player.
 *
 * The returned pointer is owned by the engine and remains valid
 * for the lifetime of the engine.
 *
 * Returns:
 *   Pointer to the player on success
 *   NULL if eng is NULL
 */
const Player *game_engine_get_player(const GameEngine *eng);

/* ============================================================
 * Player Movement & Interaction
 * ============================================================ */

/*
 * Attempt to move the player in the given direction.
 *
 * Movement logic:
 *   • Walls block movement
 *   • Pushables may be pushed
 *   • Treasures may be collected
 *   • Portals trigger room transitions
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are invalid
 *   ROOM_IMPASSABLE if movement is blocked
 *   GE_NO_SUCH_ROOM if a referenced room does not exist
 *   INTERNAL_ERROR on invariant failure
 */
Status game_engine_move_player(GameEngine *eng, Direction dir);

/* ============================================================
 * Metadata Queries
 * ============================================================ */

 /*
 * Retrieve the total number of rooms in the world.
 *
 * Returns:
 *   OK on success (count_out is set)
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if count_out is NULL
 */
Status game_engine_get_room_count(const GameEngine *eng,
                                  int *count_out);

/*
 * Retrieve the width and height of the player's current room.
 *
 * Returns:
 *   OK on success (width_out and height_out are set)
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if width_out or height_out are NULL
 *   INTERNAL_ERROR if player or room is invalid
 *   GE_NO_SUCH_ROOM if the current room cannot be found
 */
Status game_engine_get_room_dimensions(const GameEngine *eng,
                                       int *width_out,
                                       int *height_out);



/* ============================================================
 * Resetting / Restarting
 * ============================================================ */

/*
 * Reset the game to its initial state.
 *
 * Effects:
 *   • Player returns to starting room and position
 *   • Collected treasures become available again (Player and Room are updated accordingly)
 *   • Pushables return to their initial positions
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if eng is NULL
 *   INTERNAL_ERROR if reset cannot complete (this covers all internal engine state errors)
 */
Status game_engine_reset(GameEngine *eng);

/* ============================================================
 * Room Rendering
 * ============================================================ */

/*
 * Render the player's current room as a string.
 *
 * The output includes:
 *   • Walls and floor
 *   • Portals
 *   • Treasures
 *   • Pushables
 *   • Player character
 *
 * The output is formatted as a multi-line string with linefeeds after
 * each row, suitable for printing to the console.
 *
 * Parameters:
 *   eng:
 *     The game engine.
 *
 *   str_out:
 *     On success, receives a newly allocated string.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are invalid
 *   INTERNAL_ERROR on rendering failure
 *
 * Ownership:
 *   The caller must free() the returned string.
 */
Status game_engine_render_current_room(const GameEngine *eng,
                                       char **str_out);

/*
 * Render a room by ID into a newly allocated string (no player overlay).
 *
 * The output includes:
 *   - Walls and floor
 *   - Portals
 *   - Treasures
 *   - Pushables 
 *   - Does NOT include player character
 *
 * Returns:
 *   OK on success (str_out receives newly allocated string)
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if str_out is NULL
 *   GE_NO_SUCH_ROOM if room_id is invalid
 *
 * Ownership:
 *   The caller must free() the returned string.
 */
Status game_engine_render_room(const GameEngine *eng,
                               int room_id,
                               char **str_out);

/*
 * Retrieve all room IDs in the loaded world.
 *
 * Returns:
 *   OK on success (ids_out and count_out are set)
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if ids_out or count_out are NULL
 *   INTERNAL_ERROR if graph is invalid
 *   NO_MEMORY on allocation failure
 *
 * Ownership:
 *   The caller owns the returned array and must free() it.
 */
Status game_engine_get_room_ids(const GameEngine *eng,
                                int **ids_out,
                                int *count_out);


// **********************************************************                                
// ********************** ADDED FOR A2 **********************
// **********************************************************

/* ============================================================
 * Memory Utilities
 * ============================================================ */

/*
 * Free a heap buffer allocated by the game engine (e.g., render string).
 *
 * This is required for C/Python interoperability in A2 and A3
 *
 */
void game_engine_free_string(void *ptr);

#endif /* GAME_ENGINE_H */
