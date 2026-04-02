#include <stdlib.h>
#include <string.h>
#include "room.h"

static char *dup_string(const char *s)
{
    if (s == NULL) {
        return NULL;
    }
    size_t n = strlen(s);
    char *copy = malloc(n + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, s, n + 1);
    return copy;
}

static void free_portals(Portal *portals, int portal_count)
{
    if(portals == NULL){
        return;
    }
    for (int i=0; i < portal_count; i++){
        free(portals[i].name);
        portals[i].name = NULL;
    }
    free(portals);
}
static void free_treasures(Treasure *treasures, int treasure_count)
{
    if (treasures == NULL){
        return;
    }
    
    for (int i = 0;i<treasure_count; i++) {
        free(treasures[i].name);
        treasures[i].name = NULL;
    }
    free(treasures);
}

//free pushables new
static void free_pushables(Pushable *pushables, int pushable_count)
{
    if (pushables == NULL){
        return;
    }
    
    for (int i = 0;i<pushable_count; i++) {
        free(pushables[i].name);
        pushables[i].name = NULL;
    }
    free(pushables);
}

//free switches new
static void free_switches(Switch *switches, int switch_count)
{
    (void)switch_count; //not used but keeping for consistency
    if (switches == NULL){
        return;
    }
    free(switches);
}


Room *room_create(int id, const char *name, int width, int height)
{
    if (width <1) width = 1;
    if (height < 1) height = 1;

    Room *r = malloc (sizeof(Room));
    if (r == NULL){
        return NULL;
    }

    r->id = id;
    r->name = dup_string(name);
    
    if(name !=NULL && r->name == NULL){
        free(r);
        return NULL;
    }

    r->width = width;
    r->height = height;

    r->floor_grid = NULL;

    r->neighbors = NULL;
    r->neighbor_count = 0;

    r->portals = NULL;
    r->portal_count = 0;

    r->treasures = NULL;
    r->treasure_count = 0;

    r->pushables = NULL;
    r->pushable_count = 0;

    r->switches = NULL;
    r->switch_count = 0;

    return r;
}

int room_get_width(const Room *r)
{
    if (r == NULL){
        return 0;
    }
    return r->width;
}


int room_get_height(const Room *r)
{
    if(r == NULL) {
        return 0;
    }
    return r->height;
}

Status room_set_floor_grid(Room *r, bool *floor_grid)
{
    if (r==NULL) {
        return INVALID_ARGUMENT;
    }

    if (r-> floor_grid != NULL) {
        free(r->floor_grid);
        r->floor_grid = NULL;
    }

    r->floor_grid = floor_grid; //transfer ownership
    return OK;
}

Status room_set_portals(Room *r, Portal *portals, int portal_count)
{
    if(r == NULL){
        return INVALID_ARGUMENT;
    }

    if (portal_count < 0) {
        return INVALID_ARGUMENT;
    }
    if (portal_count > 0 && portals == NULL) {
        return INVALID_ARGUMENT;
    }

    if (r->portals != NULL) {
        free_portals(r->portals, r->portal_count);
        r->portals = NULL;
        r->portal_count = 0;
    }

    r->portals = portals; //transfer ownership
    r->portal_count = portal_count;
    return OK;
}

Status room_set_treasures(Room *r, Treasure *treasures, int treasure_count)
{
    if (r == NULL){
        return INVALID_ARGUMENT;
    }

    if (treasure_count < 0) {
        return INVALID_ARGUMENT;
    }
    if (treasure_count > 0 && treasures == NULL) {
        return INVALID_ARGUMENT;
    }

    if (r->treasures != NULL) {
        free_treasures(r->treasures, r->treasure_count);
        r->treasures = NULL;
        r->treasure_count = 0;
    }

    r->treasures = treasures; //transfer ownership
    r->treasure_count = treasure_count;
    return OK;
}

Status room_place_treasure(Room *r, const Treasure *treasure)
{
    if (r == NULL || treasure == NULL){
        return INVALID_ARGUMENT;
    }

    Treasure *new_arr = realloc (r->treasures, sizeof(Treasure) * (r->treasure_count + 1));
    if (new_arr == NULL){
        return NO_MEMORY;
    }
    r->treasures = new_arr;

    Treasure t = *treasure;
    t.name = dup_string(treasure->name);
    if (treasure->name != NULL && t.name == NULL){
        return NO_MEMORY;
    }

    r->treasures[r->treasure_count] = t;
    r->treasure_count += 1;

    return OK;
}

int room_get_treasure_at(const Room *r, int x, int y)
{
    if (r == NULL){
        return -1;
    }

    for (int i = 0; i < r->treasure_count; i++){
        if(!r->treasures[i].collected &&
           r->treasures[i].x == x &&
           r->treasures[i].y == y){
            return r->treasures[i].id;
        }
    }
    return -1;
}

int room_get_portal_destination(const Room *r, int x, int y)
{
    if( r == NULL){
        return -1;
    }

    for (int i = 0; i < r->portal_count; i++){
        if (r->portals[i].x == x && r->portals[i].y == y){
            return r->portals[i].target_room_id;
        }
    }
    return -1;
}

//room get id for pick up treasure

int room_get_id(const Room *r)
{
    if (r == NULL){
        return -1;
    }
    return r->id;
}

//room_pick up treasure new
Status room_pick_up_treasure(Room *r, int treasure_id, Treasure **treasure_out)
{
    if (r == NULL || treasure_out == NULL || treasure_id < 0){
        return INVALID_ARGUMENT;
    }

    for (int i = 0; i < r->treasure_count; i++){
        if (r->treasures[i].id == treasure_id) {
            if (r->treasures[i].collected) {
                return INVALID_ARGUMENT; //already collected
            }
            r->treasures[i].collected = true;
            *treasure_out = &r->treasures[i];
            return OK;
        }
    }
    return ROOM_NOT_FOUND;
}
//destroy treasure
void destroy_treasure(Treasure *t)
{
    if (t==NULL){
        return;
    }
    free(t->name);
    t->name = NULL;
    free(t);
}

//new added - pushables support (room has pushable at)
bool room_has_pushable_at(const Room *r, int x, int y, int *pushable_idx_out)
{
    if (r == NULL) {
        return false;
    }

    for(int i = 0; i < r->pushable_count; i++) {
        if(r->pushables[i].x == x && r->pushables[i].y == y) {
            if (pushable_idx_out != NULL) {
                *pushable_idx_out = i;
            }
            return true;
        }
    }
    return false;
}

// helper checking only the walls/bounds/floor grid. ignoring the pushables
static bool base_is_walkable_ignoring_pushables(const Room *r, int x, int y)
{
    if (r == NULL) return false;
    if (x<0 || y < 0 || x >= r->width || y >= r->height) return false;

    if (r->floor_grid == NULL) {
        //implicit boundary walls
        if (x == 0 || y == 0 || x == r->width - 1 || y == r->height - 1) {
            return false;
        }
        return true;
    }
    return r->floor_grid[y * r->width + x];
}

//room try push new
Status room_try_push(Room *r, int pushable_idx, Direction dir)
{
    if (r == NULL){
        return INVALID_ARGUMENT;
    }
    if (pushable_idx < 0 || pushable_idx >= r->pushable_count){
        return INVALID_ARGUMENT;
    }

    int dx = 0;
    int dy = 0;

    //MUST MATCH with the direction definitions in types.h
    switch (dir) {
        case DIR_NORTH: dy = -1; break;
        case DIR_SOUTH: dy = 1; break;
        case DIR_EAST: dx = 1; break;
        case DIR_WEST: dx = -1; break;
        default:
            return INVALID_ARGUMENT;
    }

    int cur_x = r->pushables[pushable_idx].x;
    int cur_y = r->pushables[pushable_idx].y;

    int new_x = cur_x + dx;
    int new_y = cur_y + dy;

    //must be a floor tile and not wall or out of bounds
    if (!base_is_walkable_ignoring_pushables(r, new_x, new_y)) {
        return ROOM_IMPASSABLE;
    }

    //it cant push into another pushable
    if(room_has_pushable_at(r, new_x, new_y, NULL)) {
        return ROOM_IMPASSABLE;
    }

    //not allow pushing onto the portals or the uncollected treasure
    if (room_get_portal_destination(r, new_x, new_y) != -1) {
        return ROOM_IMPASSABLE;
    }
    if (room_get_treasure_at(r, new_x, new_y) != -1) {
        return ROOM_IMPASSABLE;
    }

    r->pushables[pushable_idx].x = new_x;
    r->pushables[pushable_idx].y = new_y;

    return OK;
}

bool room_is_walkable(const Room *r, int x, int y)
{
    if(!base_is_walkable_ignoring_pushables(r, x, y)) {
        return false;
    }

    if(room_has_pushable_at(r, x, y, NULL)) {
        return false;
    }

    return true;
}

RoomTileType room_classify_tile(const Room *r, int x, int y, int *out_id)
{
    if (r == NULL){
        return ROOM_TILE_INVALID;
    }
    if (x < 0 || y < 0 || x >= r->width || y >= r->height){
        return ROOM_TILE_INVALID;
    }

    int pidx = -1; //pushable idx
    if(room_has_pushable_at(r, x, y, &pidx)) {
        if (out_id != NULL) {
            *out_id = pidx;
        }
        return ROOM_TILE_PUSHABLE;
    }


    int tid = room_get_treasure_at(r, x, y);
    if (tid != -1) {
        if (out_id != NULL){
            *out_id = tid;
        }
        return ROOM_TILE_TREASURE;
    }

    int dest = room_get_portal_destination(r, x, y);
    if (dest != -1) {
        if (out_id != NULL){
            *out_id = dest;
        }
        return ROOM_TILE_PORTAL;
    }

    if(room_is_walkable(r, x, y)){
        return ROOM_TILE_FLOOR;
    }

    return ROOM_TILE_WALL;
}

static char base_tile_char_at(const Room *r, const Charset *charset, int row, int col)
{
    if (r->floor_grid == NULL) {
        if (col == 0 || row == 0 || col == r->width - 1 || row == r->height - 1){
            return charset->wall;
        }
        return (char) charset->floor;
    }

    bool is_floor = r->floor_grid[row * r->width + col];
    return (char)(is_floor ? charset->floor : charset->wall);
}

static void overlay_treasures(const Room *r, const Charset *charset, char *buffer)
{
    for (int ti = 0; ti < r->treasure_count; ti++) {
        if (!r->treasures[ti].collected) {
            int tx = r->treasures[ti].x;
            int ty = r->treasures[ti].y;
            
            if (tx >= 0 && ty >= 0 && tx < r->width && ty < r->height) {
                buffer[ty * r->width + tx] = (char)charset->treasure;
            }
        }
    }
}

static void overlay_portals(const Room *r, const Charset *charset, char *buffer)
{
    for (int pi = 0; pi < r->portal_count; pi++) {
        int px = r->portals[pi].x;
        int py = r->portals[pi].y;
        
        if (px >= 0 && py >= 0 && px < r->width && py < r->height) {
            buffer[py * r->width + px] = (char)charset->portal;
        }
    }
}

Status room_render(const Room *r, const Charset *charset, char *buffer, int buffer_width, int buffer_height)
{
    if (r == NULL || charset == NULL || buffer == NULL){
        return INVALID_ARGUMENT;
    }

    if (buffer_width != r->width || buffer_height != r->height){
        return INVALID_ARGUMENT;
    }

    for(int row =0; row < r->height; row++) {
        for(int col = 0; col < r->width; col++) {
            buffer[row * r->width + col] = base_tile_char_at(r, charset, row, col);
        }
    }

    overlay_treasures(r, charset, buffer);
    overlay_portals(r, charset, buffer);

    return OK;
}

Status room_get_start_position(const Room *r, int *x_out, int *y_out)
{
    if(r == NULL || x_out == NULL || y_out == NULL){
        return INVALID_ARGUMENT;
    }

    if (r->portal_count > 0 && r->portals !=NULL){
        *x_out = r->portals[0].x;
        *y_out = r->portals[0].y;
        return OK;
    }

    for (int row = 1; row < r->height - 1; row++) {
        for (int col = 1; col < r->width - 1; col++) {
            if (room_is_walkable(r, col, row)){
                *x_out = col;
                *y_out = row;
                return OK;
            }
        }
    }
    return ROOM_NOT_FOUND;
}

void room_destroy(Room *r)
{
    if (r==NULL) {
        return;
    }

    free (r->name);
    r->name = NULL;

    free(r->floor_grid);
    r->floor_grid = NULL;

    free_portals(r->portals, r->portal_count);
    r->portals = NULL;
    r->portal_count = 0;

    free_treasures(r->treasures, r->treasure_count);
    r->treasures = NULL;
    r->treasure_count = 0;

    free_pushables(r->pushables, r->pushable_count);
    r->pushables = NULL;
    r->pushable_count = 0;

    free_switches(r->switches, r->switch_count);
    r->switches = NULL;
    r->switch_count = 0;

    free(r);
}    