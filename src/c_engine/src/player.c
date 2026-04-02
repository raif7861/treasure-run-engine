#include <stdlib.h>
#include <stdbool.h>
#include "player.h"

Status player_create(int initial_room_id, int initial_x, int initial_y, Player **player_out)
{
    if (player_out == NULL) {
        return INVALID_ARGUMENT;
    }

    Player *p = malloc(sizeof(Player));
    if (p == NULL){
        *player_out = NULL;
        return NO_MEMORY;
    }

    p->room_id = initial_room_id;
    p->x = initial_x;
    p->y = initial_y;

    //A2 - initializing the treasure collection
    p->collected_treasures = NULL;
    p->collected_count = 0;

    *player_out = p;
    return OK;
}

void player_destroy(Player *p)
{
    if (p == NULL){
        return;
    }

    //a2 player doesnt OWN the treasures, only array
    free(p->collected_treasures);
    p->collected_treasures = NULL;
    p->collected_count = 0;

    free(p);
}

int player_get_room(const Player *p)
{
    if (p == NULL){
        return -1;
    }
    return p->room_id;
}

Status player_get_position(const Player *p, int *x_out, int *y_out)
{
    if (p == NULL || x_out == NULL || y_out == NULL){
        return INVALID_ARGUMENT;
    }
    *x_out = p->x;
    *y_out = p->y;
    return OK;
}

Status player_set_position(Player *p, int x, int y)
{
    if (p == NULL){
        return INVALID_ARGUMENT;
    }
    p->x = x;
    p->y = y;
    return OK;
}

Status player_move_to_room(Player *p, int new_room_id)
{
    if(p==NULL){
        return INVALID_ARGUMENT;
    }
    p->room_id = new_room_id;
    return OK;
}

Status player_reset_to_start(Player *p, int starting_room_id, int start_x, int start_y)
{
    if(p==NULL){
        return INVALID_ARGUMENT;
    }

    //A2 - clear the collected treasures only in the array
    free(p->collected_treasures);
    p->collected_treasures = NULL;
    p->collected_count = 0;

    p->room_id = starting_room_id;
    p->x = start_x;
    p->y = start_y;
    return OK;
}

// A2: TREASURE COLLECTION portion
Status player_try_collect(Player *p, Treasure *treasure)
{
    if (p == NULL || treasure == NULL){
        return NULL_POINTER;
    }

    //making sure its not already in the players array (by id)
    for (int i = 0; i < p->collected_count; i++) {
        if (p->collected_treasures[i] != NULL &&
            p->collected_treasures[i]->id == treasure->id) {
            return INVALID_ARGUMENT; //already collected in player's array
        }
    }
// Room may mark it collected first, and Player still needs to store it    
    Treasure **new_arr = realloc(
        p->collected_treasures,
        sizeof(Treasure *) * (p->collected_count + 1)
    );

    if (new_arr == NULL) {

        return NO_MEMORY;
    }
    p->collected_treasures = new_arr;
    p->collected_treasures[p->collected_count] = treasure;
    p->collected_count += 1;

    treasure->collected = true;
    return OK;
}

bool player_has_collected_treasure(const Player *p, int treasure_id)
{
    if (p == NULL || treasure_id < 0){
        return false;
    }

    for (int i = 0; i < p->collected_count; i++) {
        if (p->collected_treasures[i] != NULL && p->collected_treasures[i]->id == treasure_id) {
            return true;
        }
    }
    return false;
}

int player_get_collected_count(const Player *p)
{
    if (p == NULL){
        return 0;
        }
        return p->collected_count;
}

const Treasure * const *player_get_collected_treasures(const Player *p, int *count_out)
{
    if (p == NULL || count_out == NULL){
        return NULL;
    }
    *count_out = p->collected_count;
    // cast is okay - the internal stormage is treasure**
    return (const Treasure * const *)p->collected_treasures;
}