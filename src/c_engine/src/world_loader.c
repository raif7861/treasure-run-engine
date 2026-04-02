#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "world_loader.h"
#include "datagen.h"
#include "graph.h"
#include "room.h"

//comapring two rooms by the ID which is the graph keys - graph uses that to find the nodes

static int room_compare_by_id(const void *a, const void *b)
{
    const Room *ra = (const Room *)a;
    const Room *rb = (const Room *)b;

    if (ra == NULL && rb == NULL) return 0;
    if (ra == NULL) return -1;
    if (rb == NULL) return 1;

    if (ra->id < rb->id) return -1;
    if (ra->id > rb->id) return 1;
    return 0;
}

//Graph destroyer callback - graph frees the payloads with the function
static void room_destroyer_payload(void *payload)
{
    room_destroy((Room *)payload);
}

static char *dup_cstr(const char *s)
{
    if (s==NULL){
        return NULL;
    }
    size_t n = strlen(s);
    char *copy = malloc(n + 1);
    if (copy == NULL){
        return NULL;
    }
    memcpy(copy, s, n + 1);
    return copy;
}

//portal name helper (since portal structure  needs name)
static char *make_portal_name(int portal_id)
{
    char buf[64];
    (void)snprintf(buf, sizeof(buf), "Portal %d", portal_id);
    return dup_cstr(buf);
}

static Status copy_charset_from_datagen(Charset *charset_out)
{
    const DG_Charset *dg_cs = dg_get_charset();
    if (dg_cs == NULL) {
        return WL_ERR_DATAGEN;
    }

    charset_out->wall = dg_cs->wall;
    charset_out->floor = dg_cs->floor;
    charset_out->player = dg_cs->player;
    charset_out->pushable = dg_cs->pushable; //new
    charset_out->treasure = dg_cs->treasure;
    charset_out->portal = dg_cs->portal;
    charset_out->switch_off = dg_cs->switch_off; // now need for A2
    charset_out->switch_on = dg_cs->switch_on; // now need for A2

    return OK;
}

static Status init_room_floor_grid(Room *r, const DG_Room *dg_room)
{
    if (dg_room->floor_grid == NULL){
        //NULL will mean implicit boundry walls
        return room_set_floor_grid(r, NULL);
    }

    int area = dg_room->width * dg_room->height;
    bool *grid = NULL;

    if (area > 0) {
        grid = malloc(sizeof(bool) * (size_t) area);
        if (grid == NULL) {
            return NO_MEMORY;
        }
        memcpy(grid, dg_room->floor_grid, sizeof(bool) * (size_t)area);
    }

    Status st = room_set_floor_grid(r, grid);
    if (st != OK) {
        free(grid);
    }
    return st;
}

static void free_portal_array(Portal *ports, int portal_count)
{
    if (ports == NULL) return;
    for (int ii = 0; ii < portal_count; ii++) {
        free(ports[ii].name);
        ports[ii].name = NULL;
    }
    free(ports);
}

static Status init_room_portals(Room *r, const DG_Room *dg_room)
{
    if (dg_room->portal_count <= 0) {
        return room_set_portals(r, NULL, 0);
    }

    Portal *ports = calloc((size_t)dg_room->portal_count, sizeof(Portal));
    if (ports == NULL) {
        return NO_MEMORY;
    }

    for (int ii = 0; ii < dg_room->portal_count; ii++) {
        ports[ii].id = dg_room->portals[ii].id;
        ports[ii].x = dg_room->portals[ii].x;
        ports[ii].y = dg_room->portals[ii].y;
        //dg uses the neighbors id and the portal struct uses the target room id
        ports[ii].target_room_id = dg_room->portals[ii].neighbor_id; //have to keep it since the offical field name in the DG_Portal is set to neighbor

        // new added - portal gating info from datagen
        ports[ii].required_switch_id = dg_room->portals[ii].required_switch_id;
        ports[ii].gated =(ports[ii].required_switch_id != -1); // how datagen sets when switch is required or not

        ports[ii].name = make_portal_name(ports[ii].id);
        if (ports[ii].name == NULL) {
            free_portal_array(ports, dg_room->portal_count);
            return NO_MEMORY;
        }
    }

    Status st = room_set_portals(r, ports, dg_room->portal_count);
    if (st != OK) {
        free_portal_array(ports, dg_room->portal_count);
    }
    return st;
}

static void free_treasure_array(Treasure *trs, int treasure_count)
{
    if (trs == NULL) return;
    for (int ii = 0; ii < treasure_count; ii++) {
        free(trs[ii].name);
        trs[ii].name = NULL;
    }
    free(trs);
}

static Status init_room_treasures(Room *r, const DG_Room *dg_room)
{
    if (dg_room->treasure_count <= 0) {
        return room_set_treasures(r, NULL, 0);
    }

    Treasure *trs = calloc((size_t)dg_room->treasure_count, sizeof(Treasure));
    if (trs == NULL) {
        return NO_MEMORY;
    }

    for (int ii = 0; ii < dg_room->treasure_count; ii++) {
        trs[ii].id = dg_room->treasures[ii].global_id;

        trs[ii].starting_room_id = r->id;
        trs[ii].initial_x = dg_room->treasures[ii].x;
        trs[ii].initial_y = dg_room->treasures[ii].y;

        trs[ii].x = dg_room->treasures[ii].x;
        trs[ii].y = dg_room->treasures[ii].y;

        trs[ii].collected = false;
        trs[ii].name = dup_cstr(dg_room->treasures[ii].name);

        if (dg_room->treasures[ii].name != NULL && trs[ii].name == NULL) {
            free_treasure_array(trs, dg_room->treasure_count);
            return NO_MEMORY;
        }
    }
    Status st = room_set_treasures(r, trs, dg_room->treasure_count);
    if (st != OK) {
        free_treasure_array(trs, dg_room->treasure_count);
    }
    return st;
}

//pushables support new helper functions
static void free_pushable_array(Pushable *pushables, int pushable_count)
{
    if (pushables == NULL) return;
    for (int ii = 0; ii < pushable_count; ii++) {
        free(pushables[ii].name);
        pushables[ii].name = NULL;
    }
    free(pushables);
}

// pushables support new helper functions
static Status init_room_pushables(Room *r, const DG_Room *dg_room)
{
    if (dg_room->pushable_count <= 0) {
        r->pushables = NULL;
        r->pushable_count = 0;
        return OK;
    }

    Pushable *ps = calloc((size_t)dg_room->pushable_count, sizeof(Pushable));
    if (ps == NULL) return NO_MEMORY;

    for (int ii = 0; ii < dg_room->pushable_count; ii++) {
        ps[ii].id = dg_room->pushables[ii].id;
        ps[ii].name = dup_cstr(dg_room->pushables[ii].name);
        if (dg_room->pushables[ii].name != NULL && ps[ii].name == NULL) {
            free_pushable_array(ps, dg_room->pushable_count);
            return NO_MEMORY;
        }

        ps[ii].initial_x = dg_room->pushables[ii].x;
        ps[ii].initial_y = dg_room->pushables[ii].y;
        ps[ii].x = dg_room->pushables[ii].x;
        ps[ii].y = dg_room->pushables[ii].y;

    }
    
    r->pushables = ps;
    r->pushable_count = dg_room->pushable_count;
    return OK;
}

// new added - switches support new helper functions
static Status init_room_switches(Room *r, const DG_Room *dg_room)
{
    if (dg_room->switch_count <= 0) {
        r->switches = NULL;
        r->switch_count = 0;
        return OK;
    }

    Switch *sw = calloc((size_t)dg_room->switch_count, sizeof(Switch));
    if (sw == NULL) return NO_MEMORY;

    for (int ii = 0; ii < dg_room->switch_count; ii++) {
        sw[ii].id = dg_room->switches[ii].id;
        sw[ii].x = dg_room->switches[ii].x;
        sw[ii].y = dg_room->switches[ii].y;
        sw[ii].portal_id = dg_room->switches[ii].portal_id;
    }

    r->switches = sw;
    r->switch_count = dg_room->switch_count;
    return OK;
}


static Status connect_portal_edge(Graph *g)
{
    const void *const *payloads = NULL;
    int payload_count = 0;

    if (graph_get_all_payloads(g, &payloads, &payload_count) != GRAPH_STATUS_OK) {
        return INTERNAL_ERROR;
    }

    for (int ii = 0; ii < payload_count; ii++) {
        Room *src = (Room *)payloads[ii];
        if (src == NULL) {
            continue;
        }

        for (int pi = 0; pi < src->portal_count; pi++) {
            int dest_id = src->portals[pi].target_room_id;

            Room key;
            memset(&key, 0, sizeof(key));
            key.id = dest_id;

            Room *dest = (Room *)graph_get_payload(g, &key);
            if (dest != NULL) {
                (void)graph_connect(g, src, dest);
            }
        }
    }
    return OK;
}

// build_room_from_dg: allocates and fully initialises one Room from a DG_Room.
// extracted to reduce the cognitive complexity of loader_load_world.
static Status build_room_from_dg(const DG_Room *dg_room, Room **room_out)
{
    if (dg_room == NULL || room_out == NULL) {
        return INVALID_ARGUMENT;
    }
    *room_out = NULL;

    Room *r = room_create(dg_room->id, NULL, dg_room->width, dg_room->height);
    if (r == NULL) {
        return NO_MEMORY;
    }

    Status st = init_room_floor_grid(r, dg_room);
    if (st != OK) {
        room_destroy(r);
        return st;
    }

    st = init_room_portals(r, dg_room);
    if (st != OK) {
        room_destroy(r);
        return st;
    }

    st = init_room_treasures(r, dg_room);
    if (st != OK) {
        room_destroy(r);
        return st;
    }

    st = init_room_pushables(r, dg_room);
    if (st != OK) {
        room_destroy(r);
        return st;
    }

    st = init_room_switches(r, dg_room);
    if (st != OK) {
        room_destroy(r);
        return st;
    }

    *room_out = r;
    return OK;
}

Status loader_load_world(const char *config_file, Graph **graph_out, Room **first_room_out, int *num_rooms_out, Charset *charset_out)

{
    if(config_file == NULL || graph_out == NULL || first_room_out == NULL || num_rooms_out == NULL || charset_out == NULL){
        return INVALID_ARGUMENT;
    }

    *graph_out = NULL;
    *first_room_out = NULL;
    *num_rooms_out = 0;

    //now start dartagen loading
    int dg_rc = start_datagen(config_file);

    if (dg_rc != DG_OK) {
        stop_datagen();
        //change return value if the header wanted other
        if (dg_rc == DG_ERR_CONFIG) return WL_ERR_CONFIG;
        if (dg_rc == DG_ERR_OOM) return NO_MEMORY;
        return INTERNAL_ERROR;
    }

    Status st = copy_charset_from_datagen(charset_out);
    if (st != OK) {
        stop_datagen();
        return st;
    }

    //graph creation
    Graph *g = NULL;
    if (graph_create(room_compare_by_id, room_destroyer_payload, &g) != GRAPH_STATUS_OK){
        stop_datagen();
        return NO_MEMORY;
    }

    Room *first_room = NULL;
    int room_count = 0;

    //load rooms from datagen and deep copy for ownership 
    while (has_more_rooms()) {
        DG_Room dg_room = get_next_room();

        Room *r = NULL;
        st = build_room_from_dg(&dg_room, &r);
        if (st != OK) {
            graph_destroy(g);
            stop_datagen();
            return st;
        }

        if (graph_insert(g, r) != GRAPH_STATUS_OK) {
            room_destroy(r);
            graph_destroy(g);
            stop_datagen();
            return NO_MEMORY;
        }

        if (first_room == NULL) {
            first_room = r;
        }
        room_count++;
    }

    st = connect_portal_edge(g);
    if (st != OK) {
        graph_destroy(g);
        stop_datagen();
        return st;
    }

    stop_datagen();

    *graph_out = g;
    *first_room_out = first_room;
    *num_rooms_out = room_count;

    return OK;
}