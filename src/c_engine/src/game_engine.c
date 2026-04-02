#include <stdlib.h>
#include <string.h>
#include "game_engine.h"
#include "graph.h"
#include "room.h"
#include "world_loader.h"
#include "player.h"

// quick add for game engine of just freeing string
void game_engine_free_string(void *ptr)
{
    free(ptr);
}

//---------------------------
//helper for local file
//---------------------------
//find room by mutable id for A2 - since we need to modify the room when player picks up treasure or pushables, we need mutable access to the room in the graph.
static Room *find_room_by_id_mut(Graph *g, int room_id)
{
    if (g == NULL){
        return NULL;
    }

    Room key;
    memset(&key, 0, sizeof(Room));
    key.id = room_id;

    return (Room *)graph_get_payload(g, &key);
}

static int find_portal_index_at(const Room *r, int x, int y)
{
    if (r == NULL) return -1;

    for (int i = 0; i < r->portal_count; i++) {
        if (r->portals[i].x == x && r->portals[i].y == y) {
            return i;
        }
    }
    return -1;
}

//portal gating rule:
// if the portal is gated, then it will only unlock when a pushable is sitting on the required switch tile.
static bool portal_is_unlocked(const Room *r, int portal_idx)
{
    if (r == NULL) return false;
    if (portal_idx < 0 || portal_idx >= r->portal_count) return false;

    const Portal *p = &r->portals[portal_idx];
    if (!p->gated) {
        return true; //not gated, always unlocked
    }
    // gated portal, check if the required switch is activated
    int sid = p->required_switch_id;
    if (sid < 0 || sid >= r->switch_count || r->switches == NULL) {
        return false; //invalid switch id, treat as locked
    }

    int sx = r->switches[sid].x;
    int sy = r->switches[sid].y;

    //unlocked only if a pushable is on the switch
    return room_has_pushable_at(r, sx, sy, NULL);
}

// find_paired_portal_spawn: find the spawn tile in dest_room for a portal transition.
// Paired portals share the same id across rooms - so the player lands on the portal
// in dest_room whose id matches the portal they just stepped through in the source room.
// We skip any portal that sits on the same tile as the source portal (self-loop guard).
// Falls back to room_get_start_position if no matching portal id is found.
static Status find_paired_portal_spawn(const Room *dest_room, int src_portal_id,
                                       int src_x, int src_y, int *x_out, int *y_out)
{
    if (dest_room == NULL || x_out == NULL || y_out == NULL) {
        return INVALID_ARGUMENT;
    }

    // paired portals share the same id - find the matching one in the destination room,
    // but skip the portal we just came through (same tile = self-loop)
    for (int i = 0; i < dest_room->portal_count; i++) {
        if (dest_room->portals[i].id == src_portal_id
            && !(dest_room->portals[i].x == src_x && dest_room->portals[i].y == src_y)) {
            *x_out = dest_room->portals[i].x;
            *y_out = dest_room->portals[i].y;
            return OK;
        }
    }

    // no matching portal id found (or only self), use the standard start position as fallback
    return room_get_start_position(dest_room, x_out, y_out);
}

// helper: handle portal transition after the player has stepped onto a portal tile.
// moves the player into the destination room, spawning on the matching paired portal tile.
// separated from game_engine_move_player to keep its cognitive complexity below threshold.
static Status handle_portal_transition(GameEngine *eng, Room *r, int portal_idx)
{
    if (!portal_is_unlocked(r, portal_idx)) {
        //portal exists but its locked
        return ROOM_IMPASSABLE;
    }

    int src_portal_id = r->portals[portal_idx].id;
    int src_x = r->portals[portal_idx].x;
    int src_y = r->portals[portal_idx].y;
    int dest = r->portals[portal_idx].target_room_id;
    Room *dest_room = find_room_by_id_mut(eng->graph, dest);
    if (dest_room == NULL) {
        return GE_NO_SUCH_ROOM; //portal leads to non-existent room, shouldnt happen in a valid config
    }

    int start_x = 0;
    int start_y = 0;
    // spawn on the portal in dest_room whose id matches the portal we came through,
    // skipping self-loop portals that sit on the same tile
    Status st = find_paired_portal_spawn(dest_room, src_portal_id, src_x, src_y, &start_x, &start_y);
    if (st != OK) {
        return st; //failed to get start position in destination room, shouldnt happen in a valid config
    }

    st = player_move_to_room(eng->player, dest);
    if (st != OK) {
        return st;
    }

    st = player_set_position(eng->player, start_x, start_y);
    if (st != OK) {
        return st; //failed to set player position in destination room, shouldnt happen
    }

    return OK;
}

static const Room *find_room_by_id(const Graph *g, int room_id)
{
    if(g == NULL){
        return NULL;
    }

    //graph get payload using the graph compare_fn (for room id compare)
    Room key;
    memset(&key, 0, sizeof(Room));
    key.id = room_id;

    return (const Room *)graph_get_payload(g, &key);
}

//cover a flattened framebuffer (no newlines) int the printable string with '\n'
//caller now owns *str_out and then must free it

static Status format_frambuffer_to_string(const char *buf, int width, int height, char **str_out)
{
    if (buf == NULL || str_out == NULL || width <= 0 || height <= 0){
        return INVALID_ARGUMENT;
    }

    //each row has the width chars + '\n', and one final '\0'
    size_t out_len = (size_t)(height * (width + 1)) + 1;
    char *out = malloc(out_len);
    if (out == NULL){
        return NO_MEMORY;
    }

    size_t k = 0;
    for(int row = 0;row < height; row++) {
        for (int col = 0; col < width; col++) {
            out[k++] = buf[row * width + col];
        }
        out[k++] = '\n';
    }
    out[k] = '\0';

    *str_out = out;
    return OK;
}


//---------------------------
// api implementation 
//---------------------------

Status game_engine_create(const char *config_file_path, GameEngine **engine_out)
{
    if (config_file_path == NULL || engine_out == NULL){
        return INVALID_ARGUMENT;
    }

    *engine_out = NULL;

    GameEngine *eng = malloc(sizeof(GameEngine));
    if (eng == NULL) {
        return NO_MEMORY;
    }

    //initialize safe defaults
    eng->graph = NULL;
    eng->player = NULL;
    //added room_count initalization
    eng->room_count = 0;

    // eng->charset.wall = '#';
    // eng->charset.floor = '.';
    // eng->charset.player = '@';
    // eng->charset.treasure = '$';
    // eng->charset.portal = 'O';
    // eng->charset.pushable = '*';

    eng->initial_room_id = -1;
    eng->initial_player_x = 0;
    eng->initial_player_y = 0;

    Room *first_room = NULL;
    eng->room_count = 0;

    Status st = loader_load_world(config_file_path, &eng->graph, &first_room, &eng->room_count, &eng->charset);

    //important - that the bad config HAS TO return no-OK
    if (st != OK) {
        free(eng);
        return st;
    }


    if (first_room==NULL){
        graph_destroy(eng->graph);
        free(eng);
        return INTERNAL_ERROR;
    }

    int start_x =0;
    int start_y = 0;
    st = room_get_start_position(first_room, &start_x, &start_y);
    if (st != OK) {
        graph_destroy(eng->graph);
        free(eng);
        return st;
    }
    
    Player *p = NULL;
    st = player_create(first_room->id, start_x, start_y, &p);
    if (st != OK) {
        graph_destroy(eng->graph);
        free(eng);
        return st;
    }
    eng->player = p;

    eng->initial_room_id = first_room->id;
    eng->initial_player_x = start_x;
    eng->initial_player_y = start_y;
    
    *engine_out = eng;
    return OK;
}

void game_engine_destroy(GameEngine *eng)
{
    if (eng == NULL) {
        return;
    }
// implemented destroy player and graph
    player_destroy(eng->player);
    eng->player = NULL;

    graph_destroy(eng->graph);
    eng->graph = NULL;

    free(eng);
}

const Player *game_engine_get_player(const GameEngine *eng)
{
    if (eng == NULL){
        return NULL;
    }
    return eng->player;
}

Status game_engine_get_room_count(const GameEngine *eng, int *count_out)
{
    if (eng == NULL){
        return INVALID_ARGUMENT;
    }

    if (count_out == NULL){
        return NULL_POINTER;
    }

    *count_out = eng->room_count;
    return OK;

}

Status game_engine_get_room_dimensions(const GameEngine *eng,int *width_out, int *height_out)
{
    if (eng == NULL){
        return INVALID_ARGUMENT;
    }

    if (width_out == NULL || height_out == NULL){
        return NULL_POINTER;
    }

    if(eng->player == NULL){ 
        return INTERNAL_ERROR;
    }
    
    //added implementation to get current room from player
    int room_id = player_get_room(eng->player);
    const Room *r = find_room_by_id(eng->graph, room_id);
    if (r== NULL) {
        return GE_NO_SUCH_ROOM;
    }

    *width_out = room_get_width(r);
    *height_out = room_get_height(r);
    return OK;
}

Status game_engine_get_room_ids(const GameEngine *eng, int **ids_out, int *count_out)
{
    if (eng == NULL){
        return INVALID_ARGUMENT;
    }

    if(ids_out == NULL || count_out == NULL){
        return NULL_POINTER;
    }

    if (eng->graph ==NULL){
        return INTERNAL_ERROR;
    }

    const void *const *payloads = NULL;
    int count = 0;

    GraphStatus gs = graph_get_all_payloads(eng->graph, &payloads, &count);
    if (gs != GRAPH_STATUS_OK){
        return INTERNAL_ERROR;
    }

    int *ids = malloc(sizeof(int) * (size_t)count);
    if (ids == NULL){
        return NO_MEMORY;
    }

    for (int i = 0; i < count; i++) {
        const Room *r = (const Room *)payloads[i];
        ids[i] = (r ==NULL) ? -1 : r->id;
    }

    *ids_out = ids;
    *count_out = count;
    return OK;
}

Status game_engine_reset(GameEngine *eng)
{
    if(eng == NULL) {
        return INVALID_ARGUMENT;
    }

    if(eng->player == NULL || eng->graph == NULL){
        return INTERNAL_ERROR;
    }

    //reset all the rooms ( treasures + pushables)
    const void  *const *payloads = NULL;
    int count = 0;

    GraphStatus gs = graph_get_all_payloads(eng->graph, &payloads, &count);
    if (gs != GRAPH_STATUS_OK){
        return INTERNAL_ERROR;
    }

    for(int i = 0; i < count; i++) {
        Room *r = (Room *)payloads[i];
        if (r == NULL) {
            continue;
        }

        //Treasures back to inital state
        for (int t = 0; t < r->treasure_count; t++) {
            r->treasures[t].collected = false;
            r->treasures[t].x = r->treasures[t].initial_x;
            r->treasures[t].y = r->treasures[t].initial_y;
        }

        //pushables back to the initial state
        for (int p = 0; p < r->pushable_count; p++) {
            r->pushables[p].x = r->pushables[p].initial_x;
            r->pushables[p].y = r->pushables[p].initial_y;
        }
    }
    //reset player (clearing the collected list + moves to start)
    return player_reset_to_start(eng->player, eng->initial_room_id, eng->initial_player_x, eng->initial_player_y);
}
//starts from here flow
Status game_engine_move_player(GameEngine *eng, Direction dir)
{
    //(void)dir; // used in implementation later
    
    if (eng == NULL){
        return INVALID_ARGUMENT;
    }

    if(eng->player == NULL || eng->graph == NULL){
        return INTERNAL_ERROR;
    }

    int x = 0;
    int y = 0;
    Status st = player_get_position(eng->player, &x, &y);
    if (st != OK){
        return st;
    }

    int room_id = player_get_room(eng->player);
    Room *r = find_room_by_id_mut(eng->graph, room_id);
    if (r == NULL){
        return GE_NO_SUCH_ROOM;
    }

    int dx = 0;
    int dy = 0;
    switch (dir) {
        case DIR_NORTH: dy = -1; break;
        case DIR_SOUTH: dy = 1; break;
        case DIR_EAST: dx = 1; break;
        case DIR_WEST: dx = -1; break;
        default:
            return INVALID_ARGUMENT;
    }

    int new_x = x + dx;
    int new_y = y + dy;

    // 1. if there is a pushable, attempt to push it.
    int pidx = -1;
    if (room_has_pushable_at(r, new_x, new_y, &pidx)) {
        st = room_try_push(r, pidx, dir);
        if (st != OK) {
            return ROOM_IMPASSABLE; // can't push, so can't move
        }
        //after pushing, now the tile should be free   
    }

    //2) now check walkability for the player (walls, bounds, etc)
    if (!room_is_walkable(r, new_x, new_y)) {
        return ROOM_IMPASSABLE;
    }
    
    //move player onto the tile
    st = player_set_position(eng->player, new_x, new_y);
    if (st != OK) {
        return st;
    }

    // 3) treasure collection (if treasure exsist here)
    int tid = room_get_treasure_at(r, new_x, new_y);
    if (tid != -1) {
        Treasure *picked = NULL;

        // NEW: let the room remove the treasure from the tile first
        st = room_pick_up_treasure(r, tid, &picked);
        if (st != OK) {
            return st;
        }

        // then let the player store that collected treasure
        st = player_try_collect(eng->player, picked);
        if (st != OK) {
            return st; //failed to collect treasure, shouldnt happen
        }
    }

    // 4) portal transition (with the gating support)
    int portal_idx = find_portal_index_at(r, new_x, new_y);
    if (portal_idx != -1) {
        return handle_portal_transition(eng, r, portal_idx);
    }
    return OK;
}

Status game_engine_render_room(const GameEngine *eng, int room_id, char **str_out)
{
    if (eng == NULL){
        return INVALID_ARGUMENT;
    }
    if (str_out == NULL){
        return NULL_POINTER;
    }


    *str_out = NULL;

    const Room *r = find_room_by_id(eng ->graph, room_id);
    if (r == NULL){
        return GE_NO_SUCH_ROOM;
    }

    int w = room_get_width(r);
    int h = room_get_height(r);
    if (w <= 0 || h <= 0){
        return INTERNAL_ERROR;
    }

    char *buf = malloc ((size_t)w * (size_t)h);
    if (buf ==NULL){
        return NO_MEMORY;
    }

    Status st = room_render(r, &eng->charset, buf, w, h);
    if (st !=OK){
        free(buf);
        return st;
    }

    st = format_frambuffer_to_string(buf, w, h, str_out);
    free (buf);
    return st;
}

Status game_engine_render_current_room(const GameEngine *eng, char **str_out)
{
    if (eng == NULL || str_out == NULL){
        return INVALID_ARGUMENT;
    }

    if (eng->player == NULL){
        return INTERNAL_ERROR;
    }

    *str_out = NULL;

    int room_id = player_get_room(eng->player);
    const Room *r = find_room_by_id(eng->graph, room_id);
    if (r == NULL){
        return GE_NO_SUCH_ROOM;
    }

    int w = room_get_width(r);
    int h = room_get_height(r);
    if (w <= 0 || h <= 0){
        return INTERNAL_ERROR;
    }

    size_t area = (size_t)w * (size_t)h;
    char *buf = malloc (area);
    
    if (buf == NULL){
        return NO_MEMORY;
    }

    Status st = room_render(r, &eng->charset, buf, w, h);
    if (st != OK){
        free(buf);
        return st;
    }

    int px = 0;
    int py = 0;
    st = player_get_position(eng->player, &px, &py);
    if (st == OK){
        if (px >= 0 && py >= 0 && px < w && py < h){
            buf[py * w + px] = eng->charset.player;
        }
    }

    st = format_frambuffer_to_string(buf, w, h, str_out);
    free (buf);
    return st;
}

// game_engine_get_total_treasure_count - added for A3 collect-all-treasure feature
// counts all treasures across every room in the graph regardless of collected state
#include "game_engine_extra.h"

Status game_engine_get_total_treasure_count(const GameEngine *eng, int *count_out)
{
    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }
    if (count_out == NULL) {
        return NULL_POINTER;
    }
    if (eng->graph == NULL) {
        return INTERNAL_ERROR;
    }

    const void *const *payloads = NULL;
    int room_count = 0;
    GraphStatus gs = graph_get_all_payloads(eng->graph, &payloads, &room_count);
    if (gs != GRAPH_STATUS_OK) {
        return INTERNAL_ERROR;
    }

    int total = 0;
    for (int i = 0; i < room_count; i++) {
        const Room *r = (const Room *)payloads[i];
        if (r != NULL) {
            total += r->treasure_count;
        }
    }
    *count_out = total;
    return OK;
}
    //game_engine_get_adjacency_matrix - added for A3s enhanced UI minimap feature 
    //and it builds a flattende n*n adjacency matrix from room portals connections
    
Status game_engine_get_adjacency_matrix(const GameEngine *eng,
                                        int **matrix_out,
                                        int **ids_out,
                                        int *n_out)

{
    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }
    if (matrix_out == NULL || ids_out == NULL || n_out == NULL) {
        return NULL_POINTER;
    }
    if (eng->graph == NULL) {
        return INTERNAL_ERROR;
    }

    const void *const *payloads = NULL;
    int count = 0;
    GraphStatus gs = graph_get_all_payloads(eng->graph, &payloads, &count);
    if (gs != GRAPH_STATUS_OK) {
        return INTERNAL_ERROR;
    }

    // allocate ids array - one entry per room
    int *ids = calloc((size_t)count, sizeof(int));
    if (ids == NULL) {
        return NO_MEMORY;
    }

    // allocate flattened n*n matrix initialized to 0
    int *matrix = calloc((size_t)count * (size_t)count, sizeof(int));
    if (matrix == NULL) {
        free(ids);
        return NO_MEMORY;
    }

    // fill ids array
    for (int i = 0; i < count; i++) {
        const Room *r = (const Room *)payloads[i];
        ids[i] = (r != NULL) ? r->id : -1;
    }

    // fill adjacency matrix using portal connections
    for (int i = 0; i < count; i++) {
        const Room *src = (const Room *)payloads[i];
        if (src == NULL) continue;
        for (int pi = 0; pi < src->portal_count; pi++) {
            int dest_id = src->portals[pi].target_room_id;
            for (int j = 0; j < count; j++) {
                if (ids[j] == dest_id) {
                    matrix[i * count + j] = 1;
                    break;
                }
            }
        }
    }

    *matrix_out = matrix;
    *ids_out = ids;
    *n_out = count;
    return OK;
}
