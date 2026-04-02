#ifndef DATAGEN_H
#define DATAGEN_H

#include <stdbool.h>
#include <stdint.h>

/* =============================================================
 * DATAGEN - World Data Generator (C API)
 * =============================================================
 *
 * This generator produces *model-level* room data. It does NOT
 * create tile grids, ASCII maps, or rendering structures.
 *
 * Students must:
 *   - iterate rooms using get_next_room()
 *   - deep-copy all referenced arrays into their own data model
 *   - construct their own tile representation as required
 *
 * IMPORTANT:
 *   All pointer fields inside returned Room structures reference
 *   generator-owned memory. That memory becomes invalid after
 *   stop_datagen() is called.
 *
 * Order:
 *   Rooms are returned in an arbitrary *deterministic* order.
 *   Do NOT assume rooms are sorted by ID, position, or adjacency.
 *
 * Connectivity:
 *   - Each portal whose neighbor_id >= 0 refers to a valid room.
 *   - neighbor_id == -1 indicates “no neighbor”.
 *   - Rooms will never list themselves as neighbors.
 *   - The room graph may contain cycles.
 *
 * ID semantics:
 *   - Portals: room-local IDs
 *   - Pushables: room-local IDs
 *   - Treasures: globally unique IDs (stable across all rooms)
 *
 * =============================================================
 */


/* ---------------------------------------------
 * Portal types
 * --------------------------------------------- */
typedef enum {
    DG_PORTAL_DOOR = 0,
    DG_PORTAL_STAIR_UP,
    DG_PORTAL_STAIR_DOWN,
    DG_PORTAL_TELEPORT,
    DG_PORTAL_CUSTOM
} DG_PortalType;


/* ---------------------------------------------
 * Portal (room-local identity)
 *
 * - (x,y) is tile coordinate inside the room
 * - neighbor_id = -1 → no connected room
 * - neighbor_id >= 0 → valid room ID
 * - neighbor_id will NEVER equal this room's ID
 *
 --------------------------------------------- */
typedef struct {
    int id;              /* unique within this room */
    int x;
    int y;
    DG_PortalType type;
    int neighbor_id;     /* -1 or ID of another room */
    int required_switch_id; /* -1 if always active, otherwise switch index */
} DG_Portal;


/* ---------------------------------------------
 * Pushable objects (room-local identity)
 --------------------------------------------- */
typedef struct {
    int id;              /* unique within this room */
    int type;            /* student-defined meaning */
    uint32_t flags;      /* reserved for future gameplay rules */
    int x;
    int y;
    char *name;
} DG_Pushable;


/* ---------------------------------------------
 * Switch tiles (room-local identity)
 *
 * A switch unlocks a specific portal while pressed.
 * Pressing is achieved by moving a pushable onto the tile.
 --------------------------------------------- */
typedef struct {
    int id;              /* unique within this room */
    int x;
    int y;
    int portal_id;       /* portal index this switch controls */
} DG_Switch;


/* ---------------------------------------------
 * Treasure objects (GLOBAL identity)
 *
 * global_id is unique across ALL rooms.
 * These are movable entities that travel between rooms.
 --------------------------------------------- */
typedef struct {
    int global_id;       /* globally unique */
    int type;
    int value;
    int x;
    int y;
    char *name;
} DG_Treasure;


/* ---------------------------------------------
 * Room structure (model-level only)
 --------------------------------------------- */
typedef struct {
    int id;              /* 0..room_count-1 or arbitrary; stable */

    int width;           /* tile dimensions */
    int height;

    bool *floor_grid;    /* len = width*height; true=floor, false=wall */

    DG_Portal   *portals;
    int          portal_count;

    DG_Pushable *pushables;
    int          pushable_count;

    DG_Treasure *treasures;
    int          treasure_count;

    DG_Switch   *switches;
    int          switch_count;

} DG_Room;


/* ---------------------------------------------
 * Generator error codes
 * Returned by start_datagen().
 --------------------------------------------- */
enum {
    DG_OK           = 0,
    DG_ERR_CONFIG   = -1,
    DG_ERR_OOM      = -2,
    DG_ERR_INTERNAL = -3
};


/* =============================================================
 * PUBLIC API
 * =============================================================
 */

/**
 * start_datagen
 * -------------
 * Initialize the generator using parameters in an INI config file.
 *
 * Parameters:
 *   config_path: Path to INI configuration file
 *
 * Returns:
 *   DG_OK on success
 *   DG_ERR_CONFIG if config file is missing/invalid
 *   DG_ERR_OOM if memory allocation fails
 *   DG_ERR_INTERNAL on other errors
 *
 * On success:
 *   - The generator loads and allocates all internal data.
 *   - Rooms may be iterated with has_more_rooms() / get_next_room().
 *   - Iteration order is arbitrary but deterministic.
 *
 * Must be paired with stop_datagen() to free resources.
 */
int start_datagen(const char *config_path);


/**
 * has_more_rooms
 * --------------
 * Returns true if additional rooms are available via get_next_room().
 */
bool has_more_rooms(void);


/**
 * get_next_room
 * -------------
 * Returns the next room in iteration order.
 *
 * The returned Room is a shallow copy:
 *   - pointers inside the Room refer to internal generator memory
 *   - that memory becomes invalid after stop_datagen()
 *
 * Students must deep-copy arrays to preserve data.
 */
DG_Room get_next_room(void);


/**
 * get_room_by_index
 * -----------------
 * Direct access to internal room array (read-only).
 *
 * Returns:
 *   pointer to internal Room (invalid after stop_datagen)
 *   NULL if index out of range
 */
const DG_Room *get_room_by_index(int index);


/**
 * stop_datagen
 * ------------
 * Frees all memory owned by the generator.
 * Invalidates all pointers previously returned by the API.
 */
void stop_datagen(void);


/**
 * dg_version
 * ----------
 * Returns a version string for debugging.
 */
const char *dg_version(void);


/**
 * dg_get_charset
 * --------------
 * Returns the Charset configuration loaded from the INI file.
 * Must be called between start_datagen() and stop_datagen().
 * Returns NULL if charset is not available.
 */
typedef struct {
    char wall;
    char floor;
    char player;
    char pushable;
    char treasure;
    char portal;
    char switch_off;
    char switch_on;
} DG_Charset;

const DG_Charset *dg_get_charset(void);


#endif /* DATAGEN_H */
