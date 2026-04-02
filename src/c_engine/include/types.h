#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

/* ============================================================
 * Global Types
 *
 * This header defines shared enums and plain data structures
 * used across multiple modules.
 *
 * Rules:
 *   • No function declarations
 *   • No ownership-transferring APIs
 *   • Structs are simple data containers only
 * ============================================================ */


/* ============================================================
 * Status Codes
 *
 * Unified status codes returned by all subsystems.
 * Each module uses a subset of these values.
 * ============================================================ */

typedef enum {
    /* Success */
    OK = 0,

    /* Generic errors */
    INVALID_ARGUMENT,
    NULL_POINTER,
    NO_MEMORY,
    BOUNDS_EXCEEDED,
    INTERNAL_ERROR,

    /* Room-specific errors */
    ROOM_IMPASSABLE,
    ROOM_NO_PORTAL,
    ROOM_NOT_FOUND,

    /* GameEngine-specific errors */
    GE_NO_SUCH_ROOM,

    
    /* WorldLoader-specific errors */
    WL_ERR_CONFIG,
    WL_ERR_DATAGEN
} Status;


/* ============================================================
 * Direction
 *
 * Cardinal movement directions used by the GameEngine.
 * ============================================================ */

typedef enum {
    DIR_NORTH = 0,
    DIR_SOUTH,
    DIR_EAST,
    DIR_WEST
} Direction;


/* ============================================================
 * Portal
 *
 * Static link from one room to another.
 * Portals never move and never change destination.
 *
 * Some portals may be gated by switches.
 * ============================================================ */

typedef struct {
    int id;                 /* Unique within the room */
    char *name;             /* Portal name (owned by room) */
    int x;                  /* Tile position */
    int y;                  /* Tile position */
    int target_room_id;     /* Destination room ID */
    bool gated;             /* True if portal requires a switch */
    int required_switch_id; /* Switch index that unlocks this portal (-1 if none) */
} Portal;


/* ============================================================
 * Treasure
 *
 * Collectible object with persistent identity.
 *
 * NOTE: In A1, treasures may be visual-only; the collected field
 * exists for later assignments when treasures move to the player.
 * ============================================================ */

typedef struct {
    int id;                 /* Globally unique */
    char *name;             /* Treasure name */
    int starting_room_id;   /* For reset */
    int initial_x;          /* Initial position */
    int initial_y;
    int x;                  /* Current position */
    int y;
    bool collected;         /* True if collected */
} Treasure;


/* ============================================================
 * Pushable
 *
 * Movable object whose state resets between runs.
 * ============================================================ */

typedef struct {
    int id;                 /* Unique within a room */
    char *name;             /* Pushable name */
    int initial_x;          /* Initial position */
    int initial_y;
    int x;                  /* Current position */
    int y;
} Pushable;


/* ============================================================
 * Switch
 *
 * Pressure plate that can unlock a portal.
 * Activated when a pushable rests on it.
 * ============================================================ */
typedef struct {
    int id;                 /* Unique within the room */
    int x;                  /* Tile position */
    int y;                  /* Tile position */
    int portal_id;          /* Portal index that this switch unlocks */
} Switch;


/* ============================================================
 * Charset
 *
 * Rendering characters used for ASCII display.
 *
 * NOTE: Some fields (like pushable/switch markers) may be used
 * in later assignments depending on feature scope.
 * ============================================================ */

typedef struct {
    char wall;
    char floor;
    char player;
    char pushable;
    char treasure;
    char portal;
    char switch_off;
    char switch_on;
} Charset;


/* Forward declaration for opaque Room in public APIs */
typedef struct Room Room;

#endif /* TYPES_H */
