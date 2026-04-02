#ifndef ROOM_H
#define ROOM_H

#include <stdio.h>
#include <stdbool.h>
#include "types.h"

/* ============================================================
 * Room (Data Model + API)
 *
 * The structure definition is exposed here for white-box tests,
 * but external modules should treat Room as opaque and access it
 * only through the room_* functions.
 * ============================================================ */
struct Room {
    int id;                     /* Unique room ID */
    char *name;                 /* Room name */

    int width;                  /* Tile width */
    int height;                 /* Tile height */

    /* Layout: true = floor, false = wall */
    bool *floor_grid;           /* width * height */

    /* Neighbor room IDs */
    int *neighbors;             /* neighbor_count entries */
    int neighbor_count;

    /* Static entities */
    Portal *portals;            /* portal_count entries */
    int portal_count;

    /* Mutable entities */
    Treasure *treasures;        /* treasure_count entries */
    int treasure_count;

    Pushable *pushables;        /* pushable_count entries */
    int pushable_count;

    /* Switches that gate portals */
    Switch *switches;           /* switch_count entries */
    int switch_count;
};

/* ============================================================
 * Room (Mutable World Entity)
 *
 * A Room represents a single puzzle space.
 *
 * Properties:
 *   • Fixed dimensions and layout
 *   • Static portals
 *   • Dynamic pushables
 *   • Dynamic treasure (still owned by the Room when collected)
 *
 * Design constraints:
 *   • Rooms never store Player pointers
 *   • Rooms never store pointers to other Rooms
 *   • Room connections are expressed via room IDs only
 *
 * Ownership:
 *   • Rooms are created and owned by the world loader / game engine
 *   • All memory inside a Room is freed by room_destroy
 * ============================================================ */

/* ============================================================
 * Creation
 * ============================================================ */

/*
 * Create a new Room with the given identity and dimensions.
 * - You need to allocate the Room and the name. 
 * - The room name must be a copy of the name argument.
 * - All the other pointers must be set to NULL. They will be initialized 
 *   using the relevant setter functions, which are described below
 *
 * Parameters:
 *   id:
 *     Unique room ID.
 *   name:
 *     Room name (room owns the memory).
 *   width, height:
 *     Room dimensions
 *
 * Preconditions:
 *   No preconditions. name may be NULL.
 *
 * Postconditions:
 *   On success, returns a newly allocated Room with a correctly set name.
 *   All other pointers in the Room must be initialized to NULL.
 *
 * Returns:
 *   Newly allocated Room on success
 *   NULL on allocation failure
 */
Room *room_create(int id, const char *name, int width, int height);




/* ============================================================
 * Basic Queries
 * ============================================================ */


/*
 * Retrieve the room width in tiles.
 *
 * Returns:
 *   width on success
 *   0 if r is NULL
 */
int room_get_width(const Room *r);

/*
 * Retrieve the room height in tiles.
 *
 * Returns:
 *   height on success
 *   0 if r is NULL
 */
int room_get_height(const Room *r);


/* ============================================================
 * Room Data Setters
 * ============================================================ */

/*
 * Set the floor grid (ownership transfers to the room). 
 *
 * Parameters:
 *   floor_grid:
 *     Array of width * height booleans (true = floor).
 *     Pass NULL to indicate implicit boundary walls.
 *
 * Preconditions:
 *   r must not be NULL.
 *   If floor_grid is non-NULL, it must be width * height entries.
 *
 * Postconditions:
 *   On success, ownership of floor_grid transfers to the room.
 *   If the floor grid in the room was previously initialized:
 *   - It must be overwritten by the new grid.
 *   - The old grid must be freed.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room is NULL
 */
Status room_set_floor_grid(Room *r, bool *floor_grid);

/*
 * Set portals (ownership transfers to the room).
 *
 * Preconditions:
 *   r must not be NULL.
 *   If portal_count > 0, portals must not be NULL.
 *
 * Postconditions:
 *   On success, ownership of portals transfers to the room.
 *   If the portal array in the room was previously initialized:
 *   - It must be overwritten by the new portal array.
 *   - The new portal count must be correct.
 *   - The old portal array must be freed, along with old portal names.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room is NULL or parameters are inconsistent
 */
Status room_set_portals(Room *r, Portal *portals, int portal_count);

/*
 * Set treasures (ownership transfers to the room).
 *
 * Preconditions:
 *   r must not be NULL.
 *   If treasure_count > 0, treasures must not be NULL.
 *
 * Postconditions:
 *   On success, ownership of treasures transfers to the room.
 *   If the treasure array in the room was previously initialized:
 *   - It must be overwritten by the new treasure array.
 *   - The new treasure count must be correct.
 *   - The old treasure array must be freed, along with old treasure names.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room is NULL or parameters are inconsistent
 */
Status room_set_treasures(Room *r, Treasure *treasures, int treasure_count);


/* ============================================================
 * Treasure Management
 * ============================================================ */

/*
 * Add a treasure into the room after reset.
 *
 * Preconditions:
 *   r and treasure must not be NULL.
 *
 * Postconditions:
 *   On success, ownership of the new treasure transfers to the room.
 *   The treasures array is expanded to accommodate the new treasure.
 *   The existing treasures in the room must not be affected by the addition.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room or treasure is NULL
 *   NO_MEMORY on allocation failure
 *
 * Ownership:
 *   The room takes ownership of the Treasure data (no copy of name).
 *   Caller must not free the Treasure struct after passing it in.
 */
Status room_place_treasure(Room *r, const Treasure *treasure);

/*
 * Check if a non-collected treasure exists at a given position.
 *
 * Returns:
 *   >= 0 : treasure ID if found
 *   -1   : no treasure at position or room is NULL
 */
int room_get_treasure_at(const Room *r, int x, int y);

/* ============================================================
 * Portals
 * ============================================================ */

/*
 * Check if a portal exists at (x,y).
 *
 * Returns:
 *   >= 0 : destination room ID if found
 *   -1   : no portal at position or room is NULL
 */
int room_get_portal_destination(const Room *r, int x, int y);


/* ============================================================
 * Walkability & Tile Classification
 * ============================================================ */

/*
 * Determine whether a tile is walkable.
 *
 * A tile is not walkable if:
 *   • Out of bounds
 *   • Wall (including interior walls)
 *   • Occupied by a pushable
 *
 * Returns:
 *   true if walkable
 *   false otherwise or if room is NULL
 *
 * A2 NOTE: The collected treasures are treated as floor tiles
 */
bool room_is_walkable(const Room *r, int x, int y);

/*
 * RoomTileType - Runtime tile classification
 *
 * Used by GameEngine movement logic to determine what exists at a position.
 *
 * IMPORTANT:
 *   This is NOT a storage type. Rooms do not store a grid of RoomTileType.
 *   Instead, room_classify_tile() computes the type on-demand by checking:
 *     1. Is there a treasure at this position?
 *     2. Is there a portal at this position?
 *     3. Is this position walkable?
 *     4. Otherwise, it's a wall or invalid.
 *
 * This design uses sparse storage (separate arrays for entities) rather
 * than a dense tile grid, which is more memory-efficient for large rooms
 * with few entities.
 */
typedef enum {
    ROOM_TILE_INVALID = 0,
    ROOM_TILE_WALL,
    ROOM_TILE_FLOOR,
    ROOM_TILE_PUSHABLE,
    ROOM_TILE_TREASURE,
    ROOM_TILE_PORTAL
} RoomTileType;

/*
 * Classify a tile and optionally return an associated ID:
 *
 * If out_id is non-NULL, it is set as follows:
 *   • treasure: out_id receives treasure ID
 *   • portal: out_id receives destination room ID
 *   • pushable: out_id receives the pushable index
 *
 * Returns:
 *   ROOM_TILE_INVALID for out-of-bounds or if room is NULL
 *   ROOM_TILE_WALL for walls
 *   ROOM_TILE_FLOOR for empty walkable tiles
 *   FOR A2: collected treasures are walkable floor tiles   
 *
 *   ROOM_TILE_TREASURE for uncollected treasure
 *   ROOM_TILE_PORTAL for portals
 *   ROOM_TILE_PUSHABLE for pushables
 */
RoomTileType room_classify_tile(const Room *r,
                                int x,
                                int y,
                                int *out_id);


/* ============================================================
 * Rendering
 * ============================================================ */

/*
 * Render the room into a pre-allocated buffer.
 *
 * Produces a "framebuffer" - a flat character array with NO linefeeds.
 * This allows easy overlay of dynamic entities (like the player) by the
 * game engine before formatting the final display string.
 *
 * Rendering is layered:
 *   1. Base layer: Use floor_grid to render walls and floors
 *      - floor_grid[i] == true  → render charset->floor
 *      - floor_grid[i] == false → render charset->wall
 *      - If floor_grid is NULL, assume perimeter walls with open interior
 *   2. Overlay treasures at their positions (if not collected)
 *   3. Overlay portals at their positions
 *
 * The buffer uses simple 2D-to-1D indexing: buffer[y * width + x].
 * Does NOT render the player - that's done by the game engine.
 *
 * A2 NOTE: the collected treasures are treated and displayed as floor tiles
 *
 * Parameters:
 *   r:
 *     The room to render.
 *   charset:
 *     Character set for rendering.
 *   buffer:
 *     Pre-allocated buffer of exactly width * height bytes (no linefeeds).
 *   buffer_width, buffer_height:
 *     Must match room dimensions.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room, charset, or buffer is NULL
 *   INVALID_ARGUMENT if buffer dimensions don't match room dimensions
 */
Status room_render(const Room *r,
                   const Charset *charset,
                   char *buffer,
                   int buffer_width,
                   int buffer_height);


/* ============================================================
 * Entry Position
 * ============================================================ */

/*
 * Determine a valid starting position in the room.
 *
 * Preference:
 *   1. First portal location
 *   2. Any interior walkable tile
 *
 * Returns:
 *   OK on success (x_out and y_out are set)
 *   INVALID_ARGUMENT if room or output pointers are NULL
 *   ROOM_NOT_FOUND if no valid starting position exists
 */
Status room_get_start_position(const Room *r,
                               int *x_out,
                               int *y_out);


/* ============================================================
 * Destruction & Debugging
 * ============================================================ */

/*
 * Free all memory owned by the room.
 *
 * NOTE for A2: room retains ownership of the treasures that were originally in the room  
 * and this function must free them when the room is destroyed
 *
 * Safe to call on NULL.
 */
void room_destroy(Room *r);


// **********************************************************                                
// ********************** ADDED FOR A2 **********************
// **********************************************************

/* ============================================================
 * Additional Room and Treasure functions
 * ============================================================ */


/*
 * Retrieve the Unique room ID.
 *
 * Returns:
 *   ID on success
 *   -1 if r is NULL
 */

int room_get_id(const Room *r);

/*
 * Remove a treasure from the room by ID.
 *
 * Preconditions:
 *   r and treasure_out are not NULL 
 *   treasure_id is not negative
 *
 * Parameters:
 *   treasure_out: receives a pointer to the room-owned Treasure on success
 *
 * Returns:
 *   OK on success (treasure_out is set)
 *   INVALID_ARGUMENT if r or treasure_out is NULL, or treasure already collected
 *   ROOM_NOT_FOUND if treasure_id does not exist in the room
 *
 * Postconditions:
 *   If the treasure was found, its .collected status was set to true
 *   No other Room data has been modified in any way
 *
 * Ownership:
 *   The room retains ownership of the Treasure. Callers must NOT free it.
 */
Status room_pick_up_treasure(Room *r, int treasure_id, Treasure **treasure_out);


/*
 * Free a heap-allocated Treasure (if one was ever created).
 *
 * Note:
 *   This is not room-prefixed because treasure lifetimes may need to be
 *   managed outside the Room API.
 *   Room-owned treasures should NOT be freed with this function.
 *   This function might be useful in A3.
 */
void destroy_treasure(Treasure *t);

/* ============================================================
 * Pushables
 * ============================================================ */

/*
 * Check whether a pushable exists at (x,y).
 *
 * Preconditions:
 *   r is not NULL 
 *
 * Returns:
 *   true if a pushable exists at the specified coordinates
 *   false if it does not, or on invalid arguments
 *
 * Postconditions:
 *   If pushable_idx_out is non-NULL, it receives the pushable index used 
 *   internally by the room.
 */
bool room_has_pushable_at(const Room *r,
                          int x,
                          int y,
                          int *pushable_idx_out);

/*
 * Attempt to push a pushable in the given direction.
 *
 * Preconditions:
 *   r is not NULL 
 *   pushable_idx is not negative and is less than r->pushable_count
 *   dir is a valid member of the enum type Direction
 *
 * Returns:
 *   OK on success
 *   ROOM_IMPASSABLE if blocked
 *   INVALID_ARGUMENT if arguments are invalid
 *
 * Postconditions:
 *   If push was possible, the pushable's x and y coordinates in r->pushables
 *   have been correctly updated - i.e. the obstacle was pushed
 */
Status room_try_push(Room *r,
                     int pushable_idx,
                     Direction dir);




#endif /* ROOM_H */