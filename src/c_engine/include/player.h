#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"   /* Status, Direction, Treasure */

/* ============================================================
 * Player (First-Class Entity)
 *
 * Represents the player's persistent state across the world.
 *
 * Responsibilities:
 *   • Track current room ID and position
 *   • Track collected Treasure objects
 *   • Support reset to initial state
 *
 * Design notes:
 *   • Player never stores Room pointers (IDs only)
 *   • All movement logic lives in GameEngine
 *   • Player does not own treasures (rooms retain ownership)
 * ============================================================ */

typedef struct Player {
    int room_id;                 /* Current room ID */
    int x;                       /* X position within room */
    int y;                       /* Y position within room */

    Treasure **collected_treasures; /* Borrowed pointers */
    int collected_count;
} Player;


/* ============================================================
 * Lifecycle
 * ============================================================ */

/*
 * Create a new player at the given room and position. The array of treasures should be NULL and 
 * treasure count should be 0.
 *
 * Preconditions:
 *   player_out must not be NULL.
 *
 * Returns:
 *   OK on success (player_out set)
 *   INVALID_ARGUMENT if player_out is NULL
 *   NO_MEMORY on allocation failure
 */
Status player_create(int initial_room_id, int initial_x, int initial_y, Player **player_out);

/*
 * Destroy the player and free all owned memory.
 * 
 * The player does not own the treasures, so when freeing the player, just free the array - 
 * do not free the individual treasures (the rooms will do that)
 *
 * Must be safe to call on NULL.
 */
void player_destroy(Player *p);


/* ============================================================
 * Position and Room State
 * ============================================================ */

/*
 * Get the ID of the room the player is currently in.
 *
 * Returns:
 *   Room ID on success
 *   -1 if p is NULL
 */
int player_get_room(const Player *p);

/*
 * Get the player's current position.
 *
 * Returns:
 *   OK on success (x_out and y_out are set)
 *   INVALID_ARGUMENT if any pointer is NULL
 */
Status player_get_position(const Player *p, int *x_out, int *y_out);

/*
 * Set the player's position within the current room (no validation).
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if player is NULL
 */
Status player_set_position(Player *p, int x, int y);

/*
 * Transition the player to a different room (position unchanged).
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if player is NULL
 */
Status player_move_to_room(Player *p, int new_room_id);


/* ============================================================
 * Reset
 * ============================================================ */

/*
 * Reset the player to an initial state:
 *   • Clears collected treasures
 *   • Resets room ID and position
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if player is NULL
 */
Status player_reset_to_start(Player *p,
                             int starting_room_id,
                             int start_x,
                             int start_y);



// **********************************************************                                
// ********************** ADDED FOR A2 **********************
// **********************************************************


/* ============================================================
 * Treasure Collection
 * ============================================================ */

/*
 * Attempt to collect a treasure and borrow its pointer.
 *
 * Preconditions:
 *   p and treasure are not NULL
 *   treasure has not been collected: treasure->collected is false
 *   and this treasure is not in the p->collected_treasures array
 *
 * Returns:
 *   OK on success
 *   NULL_POINTER if p or treasure is NULL
 *   INVALID_ARGUMENT if treasure already collected
 *   NO_MEMORY on allocation failure
 *
 * Postconditions:
 *   p->collected_treasures has been expanded to accommodate the new treasure
 *   treasure ptr has been correctly added to p->collected_treasures array
 *   and array length was updated
 *   treasure->collected status has been correctly updated
 *
 * Ownership:
 *   The room retains ownership of the Treasure. Player must NOT free it.
 */
Status player_try_collect(Player *p, Treasure *treasure);

/* Check whether the player has collected a given treasure ID. 
 *
 * Preconditions:
 *   p is not NULL 
 *   treasure_id is not negative
 *
 * Returns:
 *   true is treasure has been collected
 *   false otherwise, or on invalid arguments
 */
bool player_has_collected_treasure(const Player *p, int treasure_id);

/* Return the number of collected treasures.
 *
 * Preconditions:
 *   p is not NULL 
 *
 * Returns:
 *   the number of collected treasures
 *   0 if p is NULL
 */
int player_get_collected_count(const Player *p);

/*
 * Retrieve the array of collected treasures.
 *
 * Preconditions:
 *   p and count_out are not NULL
 *
 * Returns:
 *   Pointer to internal array of collected treasures (read-only)
 *   NULL if p or count_out is NULL
 *
 * Ownership:
 *   Caller must NOT free or modify returned pointers.
 *
 * Postconditions:
 *   p has not been modified
 *   count_out has been set to the correct number of treasures
 */
const Treasure * const *
player_get_collected_treasures(const Player *p, int *count_out);






#endif /* PLAYER_H */
