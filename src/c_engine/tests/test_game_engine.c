#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "game_engine.h"
#include "graph.h"
#include "room.h"
#include "player.h"
#include "types.h"
#include "game_engine_extra.h"

// HELPERS //

// room_compare_by_id_local //
static int room_compare_by_id_local(const void *a, const void  *b)
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

// room_destroyer_payload_local //
static void room_destroyer_payload_local(void *payload)
{
    room_destroy((Room *)payload);
}

// Making smalll engine without calling game_engine_create (as no datagen needed) //
// World: Room 1 (4x4) has a portal at (2,2) -> Room 2
//Room 2 (4x4) has no portals, so the starting position will be a first walkable at (1,1)
//The player will start in Room 1 at (1,1)

static Status build_engine_fixture(GameEngine **engine_out)
{
    if (engine_out == NULL) return INVALID_ARGUMENT;
    *engine_out = NULL;

    GameEngine *eng = malloc (sizeof(GameEngine));
    if (eng == NULL) return NO_MEMORY;

    eng->graph = NULL;
    eng->player = NULL;
    eng->room_count = 0;

    eng->charset.wall = '#';
    eng->charset.floor = '.';
    eng->charset.player = '@';
    eng->charset.treasure = '$';
    eng->charset.portal = 'O';

    eng->initial_room_id = 1;
    eng->initial_player_x = 1;
    eng->initial_player_y = 1;

    Graph *g = NULL;
    if (graph_create(room_compare_by_id_local, room_destroyer_payload_local, &g) != GRAPH_STATUS_OK){
        free(eng);
        return NO_MEMORY;
    }

    Room *r1 = room_create(1, "R1", 4, 4);
    if (r1 == NULL) {
        graph_destroy(g);
        free(eng);
        return NO_MEMORY;
    }

    Room *r2 = room_create(2, "R2", 4, 4);
    if (r2 == NULL) {
        room_destroy(r1);
        graph_destroy(g);
        free(eng);
        return NO_MEMORY;
    }
    // set portals for r1 //
    Portal *ports = calloc(1, sizeof(Portal));
    if (ports == NULL) {
        room_destroy(r2);
        room_destroy(r1);
        graph_destroy(g);
        free(eng);
        return NO_MEMORY;
    }
    ports[0].id = 10;
    ports[0].x = 2;
    ports[0].y = 2;
    ports[0].target_room_id = 2;
    ports[0].name = malloc(6);
    if (ports[0].name == NULL) {
        free(ports);
        room_destroy(r2);
        room_destroy(r1);
        graph_destroy(g);
        free(eng);
        return NO_MEMORY;
    }
    memcpy(ports[0].name, "P1-2\0", 6);

    Status st = room_set_portals(r1, ports, 1);
    if (st != OK) {
        // room_set_portals should take ownership on OK; and on failure it would free
        free(ports[0].name);
        free(ports);
        room_destroy(r2);
        room_destroy(r1);
        graph_destroy(g);
        free(eng);
        return st;
    }

    if (graph_insert(g, r1) != GRAPH_STATUS_OK) {
        room_destroy(r2);
        graph_destroy(g);
        free(eng);
        return NO_MEMORY;
    }

    if (graph_insert(g, r2) != GRAPH_STATUS_OK) {
        // r1 already owned by the graph, so only destroy r2 directly
        graph_destroy(g);
        free(eng);
        return NO_MEMORY;
    }

    Player *p = NULL;
    st = player_create(1, 1, 1, &p);
    if (st != OK) {
        graph_destroy(g);
        free(eng);
        return st;
    }

    eng->graph = g;
    eng->player = p;
    eng->room_count = 2;

    *engine_out = eng;
    return OK;
}
// destroy_engine_fixture //
static void destroy_engine_fixture(GameEngine *eng)
{
    if (eng == NULL) return;
    player_destroy(eng->player);
    eng->player = NULL;
    graph_destroy(eng->graph);
    eng->graph = NULL;
    free(eng);
}



// TESTING //

// test_ge_get_player_null_engine // 
START_TEST(test_ge_get_player_null_engine)
{
    ck_assert_ptr_eq(game_engine_get_player(NULL), NULL);
}
END_TEST

// test_ge_get_room_count //
START_TEST(test_ge_get_room_count)
{
    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);
    ck_assert_ptr_nonnull(eng);

    int count = -1;
    ck_assert_int_eq(game_engine_get_room_count(eng, &count), OK);
    ck_assert_int_eq(count, 2);

    destroy_engine_fixture(eng);
}
END_TEST


// test_ge_get_room_count_errors //
START_TEST(test_ge_get_room_count_errors)
{
    int count = 0;
    ck_assert_int_eq(game_engine_get_room_count(NULL, &count), INVALID_ARGUMENT);

    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);
    ck_assert_int_eq(game_engine_get_room_count(eng, NULL), NULL_POINTER);
    destroy_engine_fixture(eng);
}
END_TEST

// test_ge_get_room_dimensions //
START_TEST(test_ge_get_room_dimensions)
{
    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    int w = 0, h = 0;
    ck_assert_int_eq(game_engine_get_room_dimensions(eng, &w, &h), OK);
    ck_assert_int_eq(w, 4);
    ck_assert_int_eq(h, 4);

    destroy_engine_fixture(eng);
}
END_TEST

// test_ge_get_reset //
START_TEST(test_ge_reset)
{
    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    // move east -> (2,1)
    ck_assert_int_eq(game_engine_move_player(eng, DIR_EAST), OK);

    int x = -1, y = -1;
    ck_assert_int_eq(player_get_position(eng->player, &x, &y), OK);
    ck_assert_int_eq(x, 2);
    ck_assert_int_eq(y, 1);

    // reset back to (1,1) in room 1
    ck_assert_int_eq(game_engine_reset(eng), OK);
    ck_assert_int_eq(player_get_room(eng->player), 1);
    ck_assert_int_eq(player_get_position(eng->player, &x, &y), OK);
    ck_assert_int_eq(x, 1);
    ck_assert_int_eq(y, 1);

    destroy_engine_fixture(eng);
}
END_TEST

// test_ge_move_impassable_wall //
START_TEST(test_ge_move_impassable_wall)
{
    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    // start at (1,1). west -> (0,1) is boundary wall => impassable
    ck_assert_int_eq(game_engine_move_player(eng, DIR_WEST), ROOM_IMPASSABLE);

    int x = -1, y = -1;
    ck_assert_int_eq(player_get_position(eng->player, &x, &y), OK);
    ck_assert_int_eq(x, 1);
    ck_assert_int_eq(y, 1);

    destroy_engine_fixture(eng);
}
END_TEST

// test_ge_move_portal_transition //
START_TEST(test_ge_move_portal_transition)
{
    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    // Move to portal at (2,2): from (1,1) -> east (2,1) -> south (2,2) triggers portal to room 2 //
    ck_assert_int_eq(game_engine_move_player(eng, DIR_EAST), OK);
    ck_assert_int_eq(game_engine_move_player(eng, DIR_SOUTH), OK);

    ck_assert_int_eq(player_get_room(eng->player), 2);

    // room 2 has no portals; start position should be first walkable: (1,1)
    int x = -1, y = -1;
    ck_assert_int_eq(player_get_position(eng->player, &x, &y), OK);
    ck_assert_int_eq(x, 1);
    ck_assert_int_eq(y, 1);

    destroy_engine_fixture(eng);
}
END_TEST

// test_ge_render_room_and_current_room //
START_TEST(test_ge_render_room_and_current_room)
{
    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    char *s1 = NULL;
    ck_assert_int_eq(game_engine_render_room(eng, 1, &s1), OK);
    ck_assert_ptr_nonnull(s1);

    // For 4x4: string has 4 rows of (4 chars + \n) = 20 characters, the strlen() does NOT include the final null terminator '\0'
    ck_assert_int_eq((int)strlen(s1), 20);

    // render_room should NOT include player '@'
    ck_assert_ptr_eq(strchr(s1, eng->charset.player), NULL);
    free(s1);

    char *s2 = NULL;
    ck_assert_int_eq(game_engine_render_current_room(eng, &s2), OK);
    ck_assert_ptr_nonnull(s2);

    // player at (1,1) => index = row*(w+1)+col = 1*5 + 1 = 6
    ck_assert_int_eq(s2[6], eng->charset.player);

    free(s2);
    destroy_engine_fixture(eng);
}
END_TEST

// test_ge_get_room_ids //
START_TEST(test_ge_get_room_ids)
{
    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    int *ids = NULL;
    int count = 0;
    ck_assert_int_eq(game_engine_get_room_ids(eng, &ids, &count), OK);
    ck_assert_ptr_nonnull(ids);
    ck_assert_int_eq(count, 2);

    // order not guaranteed; just check both ids exist
    int seen1 = 0, seen2 = 0;
    for (int i = 0; i < count; i++) {
        if (ids[i] == 1) seen1 = 1;
        if (ids[i] == 2) seen2 = 1;
    }
    ck_assert_int_eq(seen1, 1);
    ck_assert_int_eq(seen2, 1);

    free(ids);
    destroy_engine_fixture(eng);
}
END_TEST

//try create an engine from the real assets config (hitting the real loader code + parsing paths)
static Status create_engine_from_assets(GameEngine **engine_out)
{
    if (engine_out == NULL) return INVALID_ARGUMENT;
    *engine_out = NULL;

    Status st = game_engine_create("assets/world_config.cfg", engine_out);
    if (st == OK) return OK;

    //the back up for local runs where the assets might end up in one directory
    return game_engine_create("../assets/world_config.cfg", engine_out);
}

// test_ge_render_room_errors //
START_TEST(test_ge_render_room_errors)
{
    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    char *out = (char *)0x1;
    ck_assert_int_eq(game_engine_render_room(NULL, 1, &out), INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_render_room(eng, 1, NULL), NULL_POINTER);

    // invalid room id
    out = NULL;
    ck_assert_int_eq(game_engine_render_room(eng, 999, &out), GE_NO_SUCH_ROOM);
    ck_assert_ptr_eq(out, NULL);

    destroy_engine_fixture(eng);
}
END_TEST

// test_ge_create_destroy_real_config // 
START_TEST(test_ge_create_destroy_real_config)
{
    GameEngine *eng = NULL;
    Status st = create_engine_from_assets(&eng);


    // the CI might not inclue the assets/world_config.cfg, so we allow the test to be skipped if the config file is not found; but if it is found, then the engine should be created successfully
    if(st == OK) {
        ck_assert_ptr_nonnull(eng);
        game_engine_destroy(eng);
    } else {
        ck_assert_ptr_eq(eng, NULL);
        ck_assert_int_ne(st, INVALID_ARGUMENT);
    }
}
END_TEST

//test_ge_create_invalid_args //
START_TEST(test_ge_create_invalid_args)
{
    GameEngine *eng = (GameEngine *)0x1;

    ck_assert_int_eq(game_engine_create(NULL, &eng), INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_create("assets/world_config.cfg", NULL), INVALID_ARGUMENT);
}
END_TEST

//test_ge_create_bad_config_path //
START_TEST(test_ge_create_bad_config_path)
{
    GameEngine *eng = NULL;
    Status st = game_engine_create("assset/does_not_exist.ini", &eng);

    ck_assert_int_ne(st, OK);
    ck_assert_ptr_eq(eng, NULL);
}
END_TEST

// test_ge_move_invalid_direction //
START_TEST(test_ge_move_invalid_direction)
{
    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    ck_assert_int_eq(game_engine_move_player(eng, (Direction)999), INVALID_ARGUMENT);

    destroy_engine_fixture(eng);
}
END_TEST

// test_ge_render_current_room_errors //
START_TEST(test_ge_render_current_room_errors)
{
    char *out = (char *)0x1;

    ck_assert_int_eq(game_engine_render_current_room(NULL, &out), INVALID_ARGUMENT);

    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    ck_assert_int_eq(game_engine_render_current_room(eng, NULL), INVALID_ARGUMENT);

    // manually break the player to test the error handling of render_current_room when player is NULL
    destroy_engine_fixture(eng);
}

// test_ge_get_room_dimensions_errors //
START_TEST(test_ge_get_room_dimensions_errors)
{
    int w = 0;
    int h = 0;

    ck_assert_int_eq(game_engine_get_room_dimensions(NULL, &w, &h), INVALID_ARGUMENT);

    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    ck_assert_int_eq(game_engine_get_room_dimensions(eng, NULL, &h), NULL_POINTER);
    ck_assert_int_eq(game_engine_get_room_dimensions(eng, &w, NULL), NULL_POINTER);

    // break player manually to hit INTERNAL_ERROR branch
    Player *saved = eng->player;
    eng->player = NULL;
    ck_assert_int_eq(game_engine_get_room_dimensions(eng, &w, &h), INTERNAL_ERROR);
    eng->player = saved;

    destroy_engine_fixture(eng);
}
END_TEST

// test_ge_get_room_ids_errors //
START_TEST(test_ge_get_room_ids_errors)
{
    int *ids = NULL;
    int count = 0;

    ck_assert_int_eq(game_engine_get_room_ids(NULL, &ids, &count), INVALID_ARGUMENT);

    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    ck_assert_int_eq(game_engine_get_room_ids(eng, NULL, &count), NULL_POINTER);
    ck_assert_int_eq(game_engine_get_room_ids(eng, &ids, NULL), NULL_POINTER);

    // break graph manually to hit INTERNAL_ERROR branch
    Graph *saved_graph = eng->graph;
    eng->graph = NULL;
    ck_assert_int_eq(game_engine_get_room_ids(eng, &ids, &count), INTERNAL_ERROR);
    eng->graph = saved_graph;

    destroy_engine_fixture(eng);
}
END_TEST

// test_ge_reset_errors //
START_TEST(test_ge_reset_errors)
{
    ck_assert_int_eq(game_engine_reset(NULL), INVALID_ARGUMENT);

    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    // break player manually to hit INTERNAL_ERROR
    Player *saved_player = eng->player;
    eng->player = NULL;
    ck_assert_int_eq(game_engine_reset(eng), INTERNAL_ERROR);
    eng->player = saved_player;

    // break graph manually to hit INTERNAL_ERROR
    Graph *saved_graph = eng->graph;
    eng->graph = NULL;
    ck_assert_int_eq(game_engine_reset(eng), INTERNAL_ERROR);
    eng->graph = saved_graph;

    destroy_engine_fixture(eng);
}
END_TEST



// test_ge_move_null_engine //
START_TEST(test_ge_move_null_engine)
{
    ck_assert_int_eq(game_engine_move_player(NULL, DIR_NORTH), INVALID_ARGUMENT);
}
END_TEST

// test_ge_move_collects_treasure_directly - injects a treasure at (2,1) and
// verifies collection count increments when the player walks onto that tile
START_TEST(test_ge_move_collects_treasure_directly)
{
    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    // get room 1 and place a treasure at (2,1)
    Room key;
    memset(&key, 0, sizeof(Room));
    key.id = 1;
    Room *r1 = (Room *)graph_get_payload(eng->graph, &key);
    ck_assert_ptr_nonnull(r1);

    Treasure t;
    memset(&t, 0, sizeof(Treasure));
    t.id = 99;
    t.x = 2;
    t.y = 1;
    t.collected = false;
    t.name = malloc(5);
    memcpy(t.name, "Gold", 5);
    ck_assert_int_eq(room_place_treasure(r1, &t), OK);
    free(t.name);

    // move east to (2,1) - should collect the treasure
    ck_assert_int_eq(game_engine_move_player(eng, DIR_EAST), OK);
    ck_assert_int_eq(player_get_collected_count(eng->player), 1);
    ck_assert_int_eq(player_has_collected_treasure(eng->player, 99), true);

    // reset clears collection
    ck_assert_int_eq(game_engine_reset(eng), OK);
    ck_assert_int_eq(player_get_collected_count(eng->player), 0);

    destroy_engine_fixture(eng);
}
END_TEST

// test_ge_move_pushable_blocked - pushable against a wall can't be pushed further
START_TEST(test_ge_move_pushable_blocked)
{
    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    // get room 1 and inject a pushable at (1,2) directly south of the player
    Room key;
    memset(&key, 0, sizeof(Room));
    key.id = 1;
    Room *r1 = (Room *)graph_get_payload(eng->graph, &key);
    ck_assert_ptr_nonnull(r1);

    Pushable *ps = calloc(1, sizeof(Pushable));
    ck_assert_ptr_nonnull(ps);
    ps[0].id = 5;
    ps[0].x = 1;
    ps[0].y = 2;
    ps[0].initial_x = 1;
    ps[0].initial_y = 2;
    ps[0].name = malloc(4);
    memcpy(ps[0].name, "Box", 4);
    r1->pushables = ps;
    r1->pushable_count = 1;

    // first push south: pushable at (1,2) would move to (1,3) = boundary wall -> IMPASSABLE
    // in a 4x4 room rows are 0-3, y=3 is the boundary, so the push fails immediately
    ck_assert_int_eq(game_engine_move_player(eng, DIR_SOUTH), ROOM_IMPASSABLE);
    ck_assert_int_eq(r1->pushables[0].y, 2); // position unchanged

    destroy_engine_fixture(eng);
}
END_TEST

// test_ge_move_collects_treasure - resets engine after portal transition and verifies
// player is back at start; exercises the treasure reset path in game_engine_reset
START_TEST(test_ge_move_collects_treasure)
{
    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    // move east -> (2,1), then south -> (2,2) which triggers portal to room 2
    ck_assert_int_eq(game_engine_move_player(eng, DIR_EAST), OK);
    ck_assert_int_eq(game_engine_move_player(eng, DIR_SOUTH), OK);

    // player should now be in room 2 after portal transition
    ck_assert_int_eq(player_get_room(eng->player), 2);

    // reset - brings player back to room 1 at (1,1)
    ck_assert_int_eq(game_engine_reset(eng), OK);
    ck_assert_int_eq(player_get_room(eng->player), 1);

    int x = -1;
    int y = -1;
    ck_assert_int_eq(player_get_position(eng->player, &x, &y), OK);
    ck_assert_int_eq(x, 1);
    ck_assert_int_eq(y, 1);

    destroy_engine_fixture(eng);
}
END_TEST
// test_ge_get_total_treasure_count - covers the new A3 total treasure count function
START_TEST(test_ge_get_total_treasure_count)
{
    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    // fixture has 2 rooms with no treasures injected so total should be 0
    int total = -1;
    ck_assert_int_eq(game_engine_get_total_treasure_count(eng, &total), OK);
    ck_assert_int_ge(total, 0);

    // null checks
    ck_assert_int_eq(game_engine_get_total_treasure_count(NULL, &total), INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_get_total_treasure_count(eng, NULL), NULL_POINTER);

    destroy_engine_fixture(eng);
}
END_TEST
//ADDED FOR A3 BEFORE SUITE FUNCTION GAME_ENGINE_SUITE 
//--> added before is the game engine get adjacency matrix.
START_TEST(test_ge_get_adjacency_matrix)
{
    GameEngine *eng = NULL;
    ck_assert_int_eq(build_engine_fixture(&eng), OK);

    int *matrix = NULL;
    int *ids = NULL;
    int n = 0;

    ck_assert_int_eq(game_engine_get_adjacency_matrix(eng, &matrix, &ids, &n), OK);
    ck_assert_int_ge(n, 0);
    ck_assert_ptr_nonnull(matrix);
    ck_assert_ptr_nonnull(ids);

    free(matrix);
    free(ids);

    // null checks
    ck_assert_int_eq(game_engine_get_adjacency_matrix(NULL, &matrix, &ids, &n), INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_get_adjacency_matrix(eng, NULL, &ids, &n), NULL_POINTER);
    ck_assert_int_eq(game_engine_get_adjacency_matrix(eng, &matrix, NULL, &n), NULL_POINTER);
    ck_assert_int_eq(game_engine_get_adjacency_matrix(eng, &matrix, &ids, NULL), NULL_POINTER);

    destroy_engine_fixture(eng);
}
END_TEST


Suite *game_engine_suite(void)
{
    Suite *s = suite_create("GameEngine");
    TCase *tc = tcase_create("Core");

    tcase_add_test(tc, test_ge_get_player_null_engine);
    tcase_add_test(tc, test_ge_get_room_count);
    tcase_add_test(tc, test_ge_get_room_count_errors);
    tcase_add_test(tc, test_ge_get_room_dimensions);
    tcase_add_test(tc, test_ge_reset);
    tcase_add_test(tc, test_ge_move_impassable_wall);
    tcase_add_test(tc, test_ge_move_portal_transition);
    tcase_add_test(tc, test_ge_render_room_and_current_room);
    tcase_add_test(tc, test_ge_get_room_ids);
    tcase_add_test(tc, test_ge_render_room_errors);
    tcase_add_test(tc, test_ge_create_destroy_real_config);
    tcase_add_test(tc, test_ge_create_invalid_args);
    tcase_add_test(tc, test_ge_create_bad_config_path);
    tcase_add_test(tc, test_ge_move_invalid_direction);
    tcase_add_test(tc, test_ge_render_current_room_errors);
    tcase_add_test(tc, test_ge_get_room_dimensions_errors);
    tcase_add_test(tc, test_ge_get_room_ids_errors);
    tcase_add_test(tc, test_ge_reset_errors);
    tcase_add_test(tc, test_ge_move_collects_treasure);
    tcase_add_test(tc, test_ge_move_null_engine);
    tcase_add_test(tc, test_ge_move_collects_treasure_directly);
    tcase_add_test(tc, test_ge_move_pushable_blocked);
    tcase_add_test(tc, test_ge_get_total_treasure_count);
    tcase_add_test(tc, test_ge_get_adjacency_matrix);

    suite_add_tcase(s, tc);
    return s;
}

