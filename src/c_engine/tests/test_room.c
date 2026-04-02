#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "room.h"

// helper to duplicate the C string without using strdup //

static char *test_dup(const char *s)
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

// test_room_create_clamps_dimensions // 
START_TEST(test_room_create_clamps_dimensions)
{
    Room *r = room_create(7, "R", 0, -5);
    ck_assert_ptr_nonnull(r);
    ck_assert_int_eq(room_get_width(r), 1);
    ck_assert_int_eq(room_get_height(r), 1);
    room_destroy(r);
}
END_TEST

// test_room_is_walkable_implicit_walls // 
START_TEST(test_room_is_walkable_implicit_walls)
{
    Room *r = room_create(0, "A", 4, 4);
    ck_assert_ptr_nonnull(r);

    // implicit wallas when the floor grid is NULL, so only the boundry is walls (the interior is walkable)
    ck_assert_int_eq(room_is_walkable(r, 0, 0), 0);
    ck_assert_int_eq(room_is_walkable(r, 3, 0), 0);
    ck_assert_int_eq(room_is_walkable(r, 0, 3), 0);
    ck_assert_int_eq(room_is_walkable(r, 3, 3), 0);

    ck_assert_int_eq(room_is_walkable(r, 1, 1), 1);
    ck_assert_int_eq(room_is_walkable(r, 2, 2), 1);

    //out of bounds assertions
    ck_assert_int_eq(room_is_walkable(r, -1, 0), 0);
    ck_assert_int_eq(room_is_walkable(r, 0, -1), 0);
    ck_assert_int_eq(room_is_walkable(r, 4, 0), 0);
    ck_assert_int_eq(room_is_walkable(r, 0, 4), 0);

    room_destroy(r);
}
END_TEST

// test_room_set_floor_grid_controls_walkable // 
START_TEST(test_room_set_floor_grid_controls_walkable)
{
    Room *r = room_create(0, "B", 3, 3);
    ck_assert_ptr_nonnull(r);

    // all false except centre true
    bool *grid = malloc(sizeof(bool) * 9);
    ck_assert_ptr_nonnull(grid);
    for (int i = 0; i < 9; i++) {
        grid[i] = false;
    }
    grid[1 * 3 + 1] = true;

    ck_assert_int_eq(room_set_floor_grid(r, grid), OK);

    ck_assert_int_eq(room_is_walkable(r, 1, 1), 1);
    ck_assert_int_eq(room_is_walkable(r, 0, 0), 0);
    ck_assert_int_eq(room_is_walkable(r, 2, 2), 0);

    room_destroy(r);
}
END_TEST

// test_room_treasure_and_portal_queries //
START_TEST(test_room_treasure_and_portal_queries)
{
    Room *r = room_create(10, "C", 5, 5);
    ck_assert_ptr_nonnull(r);

    // portals //
    Portal *ports = malloc(sizeof(Portal) * 1);
    ck_assert_ptr_nonnull(ports);
    memset(ports, 0, sizeof(Portal) * 1);

    ports[0].id = 99;
    ports[0].x = 2;
    ports[0].y = 1;
    ports[0].target_room_id = 123;
    ports[0].name = test_dup("P");
    ck_assert_ptr_nonnull(ports[0].name);

    ck_assert_int_eq(room_set_portals(r, ports, 1), OK);
    ck_assert_int_eq(room_get_portal_destination(r, 2, 1), 123);
    ck_assert_int_eq(room_get_portal_destination(r, 0, 0), -1);

    // treasures // 
    Treasure *trs = malloc(sizeof(Treasure) * 1);
    ck_assert_ptr_nonnull(trs);
    memset(trs, 0, sizeof(Treasure) * 1);

    trs[0].id = 77;
    trs[0].x = 1;
    trs[0].y = 3;
    trs[0].collected = false;
    trs[0].name = test_dup("T");
    ck_assert_ptr_nonnull(trs[0].name);

    ck_assert_int_eq(room_set_treasures(r, trs, 1), OK);
    ck_assert_int_eq(room_get_treasure_at(r, 1, 3), 77);
    ck_assert_int_eq(room_get_treasure_at(r, 2, 2), -1);

    room_destroy(r);
}
END_TEST

// test_room_classify_tile_and_render //
START_TEST(test_room_classify_tile_and_render)
{
    Room *r = room_create(5, "D", 4, 3);
    ck_assert_ptr_nonnull(r);

    //make all the tiles floor via explicit grid so the boundries can be floor too
    bool *grid = malloc(sizeof(bool) * (4 * 3));
    ck_assert_ptr_nonnull(grid);
    for (int i = 0; i < 12; i++) {
        grid[i] = true;
    }
    ck_assert_int_eq(room_set_floor_grid(r, grid), OK);

    //one treasure at  (1,1), one portal at (2,1)
    Treasure *trs = malloc(sizeof(Treasure) * 1);
    ck_assert_ptr_nonnull(trs);
    memset(trs, 0, sizeof(Treasure) * 1);
    trs[0].id = 1;
    trs[0].x = 1;
    trs[0].y = 1;
    trs[0].collected = false;
    trs[0].name = test_dup("Gold");
    ck_assert_ptr_nonnull(trs[0].name);
    ck_assert_int_eq(room_set_treasures(r, trs, 1), OK);

    Portal *ports = malloc(sizeof(Portal) * 1);
    ck_assert_ptr_nonnull(ports);
    memset(ports, 0, sizeof(Portal) * 1);
    ports[0].id = 2;
    ports[0].x = 2;
    ports[0].y = 1;
    ports[0].target_room_id = 42;
    ports[0].name = test_dup("Exit");
    ck_assert_ptr_nonnull(ports[0].name);
    ck_assert_int_eq(room_set_portals(r, ports, 1), OK);

    int out_id = -1;
    ck_assert_int_eq(room_classify_tile(r, 1, 1, &out_id), ROOM_TILE_TREASURE);
    ck_assert_int_eq(out_id, 1);

    out_id = -1;
    ck_assert_int_eq(room_classify_tile(r, 2, 1, &out_id), ROOM_TILE_PORTAL);
    ck_assert_int_eq(out_id, 42);

    Charset cs;
    cs.wall = '#';
    cs.floor = '.';
    cs.player = '@';
    cs.treasure = '$';
    cs.portal = 'O';

    char buffer[12]; // 4 width * 3 height
    ck_assert_int_eq(room_render(r, &cs, buffer, 4, 3), OK);

    //buffer index = y * width + x //
    ck_assert_int_eq(buffer[1 * 4 + 1], '$'); //treasure at (1,1)
    ck_assert_int_eq(buffer[1 * 4 + 2], 'O'); //portal at (2,1)

    room_destroy(r);
}
END_TEST

// test_room_get_start_position_prefers_first_portal //
START_TEST(test_room_get_start_position_prefers_first_portal)
{
    Room *r = room_create(9, "E", 4, 4);
    ck_assert_ptr_nonnull(r);

    Portal *ports = malloc(sizeof(Portal) * 1);
    ck_assert_ptr_nonnull(ports);
    memset(ports, 0, sizeof(Portal) * 1);
    ports[0].id = 1;
    ports[0].x = 2;
    ports[0].y = 2;
    ports[0].target_room_id = 99;
    ports[0].name = test_dup("StartPortal");
    ck_assert_ptr_nonnull(ports[0].name);

    ck_assert_int_eq(room_set_portals(r, ports, 1), OK);

    int x = -1, y = -1;
    ck_assert_int_eq(room_get_start_position(r, &x, &y), OK);
    ck_assert_int_eq(x, 2);
    ck_assert_int_eq(y, 2);

    room_destroy(r);
}
END_TEST

// test_room_get_start_position_finds_first_walkable //
START_TEST(test_room_get_start_position_finds_first_walkable)
{
    Room *r = room_create(9, "F", 4, 4);
    ck_assert_ptr_nonnull(r);
    
    // implicit walls, so first interior walkable is (1,1) //
    int x = -1, y = -1;
    ck_assert_int_eq(room_get_start_position(r, &x, &y), OK);
    ck_assert_int_eq(x, 1);
    ck_assert_int_eq(y, 1);

    room_destroy(r);
}
END_TEST


// test_room_null_guards - hits null arg branches across room API functions
START_TEST(test_room_null_guards)
{
    // room_is_walkable null room
    ck_assert_int_eq(room_is_walkable(NULL, 0, 0), 0);

    // room_get_portal_destination null room
    ck_assert_int_eq(room_get_portal_destination(NULL, 0, 0), -1);

    // room_get_treasure_at null room
    ck_assert_int_eq(room_get_treasure_at(NULL, 0, 0), -1);

    // room_get_start_position null args
    Room *r = room_create(1, "X", 4, 4);
    ck_assert_ptr_nonnull(r);
    int x = 0, y = 0;
    ck_assert_int_ne(room_get_start_position(NULL, &x, &y), OK);
    ck_assert_int_ne(room_get_start_position(r, NULL, &y), OK);
    ck_assert_int_ne(room_get_start_position(r, &x, NULL), OK);

    // null room returns ROOM_TILE_INVALID; out-of-bounds also returns ROOM_TILE_INVALID
    int out = 0;
    ck_assert_int_eq(room_classify_tile(NULL, 1, 1, &out), ROOM_TILE_INVALID);
    ck_assert_int_eq(room_classify_tile(r, -1, 0, &out), ROOM_TILE_INVALID);
    ck_assert_int_eq(room_classify_tile(r, 99, 0, &out), ROOM_TILE_INVALID);

    // null out_id is allowed - (0,0) is a boundary wall in a 4x4 room -> ROOM_TILE_WALL
    ck_assert_int_eq(room_classify_tile(r, 0, 0, NULL), ROOM_TILE_WALL);
    // valid interior tile with out_id - (1,1) is walkable floor
    ck_assert_int_eq(room_classify_tile(r, 1, 1, &out), ROOM_TILE_FLOOR);

    room_destroy(r);
}
END_TEST

// test_room_place_treasure //
START_TEST(test_room_place_treasure)
{
    Room *r = room_create(20, "G", 5, 5);
    ck_assert_ptr_nonnull(r);

    Treasure t;
    memset(&t, 0, sizeof(Treasure));
    t.id = 55;
    t.x = 2;
    t.y = 2;
    t.collected = false;
    t.name = test_dup("Gold");
    ck_assert_ptr_nonnull(t.name);

    ck_assert_int_eq(room_place_treasure(r, &t), OK);
    free(t.name); // room_place_treasure dup'd it; free our copy
    ck_assert_int_eq(r->treasure_count, 1);
    ck_assert_int_eq(room_get_treasure_at(r, 2, 2), 55);

    // null checks
    ck_assert_int_eq(room_place_treasure(NULL, &t), INVALID_ARGUMENT);
    ck_assert_int_eq(room_place_treasure(r, NULL), INVALID_ARGUMENT);

    room_destroy(r);
}
END_TEST

// test_room_pick_up_treasure //
START_TEST(test_room_pick_up_treasure)
{
    Room *r = room_create(21, "H", 5, 5);
    ck_assert_ptr_nonnull(r);

    Treasure *trs = calloc(1, sizeof(Treasure));
    ck_assert_ptr_nonnull(trs);
    trs[0].id = 77;
    trs[0].x = 2;
    trs[0].y = 2;
    trs[0].collected = false;
    trs[0].name = test_dup("Silver");
    ck_assert_int_eq(room_set_treasures(r, trs, 1), OK);

    // invalid arg checks
    ck_assert_int_eq(room_pick_up_treasure(NULL, 77, NULL), INVALID_ARGUMENT);
    Treasure *out = NULL;
    ck_assert_int_eq(room_pick_up_treasure(r, 77, NULL), INVALID_ARGUMENT);
    ck_assert_int_eq(room_pick_up_treasure(r, -1, &out), INVALID_ARGUMENT);

    // not found id
    ck_assert_int_eq(room_pick_up_treasure(r, 999, &out), ROOM_NOT_FOUND);

    // successful pickup
    ck_assert_int_eq(room_pick_up_treasure(r, 77, &out), OK);
    ck_assert_ptr_nonnull(out);
    ck_assert_int_eq(out->collected, true);

    // already collected - second pickup should fail
    ck_assert_int_eq(room_pick_up_treasure(r, 77, &out), INVALID_ARGUMENT);

    room_destroy(r);
}
END_TEST

// test_room_try_push_basic //
START_TEST(test_room_try_push_basic)
{
    Room *r = room_create(22, "I", 6, 6);
    ck_assert_ptr_nonnull(r);

    // set up a pushable at (2,2)
    Pushable *ps = calloc(1, sizeof(Pushable));
    ck_assert_ptr_nonnull(ps);
    ps[0].id = 1;
    ps[0].x = 2;
    ps[0].y = 2;
    ps[0].initial_x = 2;
    ps[0].initial_y = 2;
    ps[0].name = test_dup("Boulder");
    r->pushables = ps;
    r->pushable_count = 1;

    // push east: (2,2) -> (3,2), interior tile, should succeed
    ck_assert_int_eq(room_try_push(r, 0, DIR_EAST), OK);
    ck_assert_int_eq(r->pushables[0].x, 3);
    ck_assert_int_eq(r->pushables[0].y, 2);

    // push north: (3,2) -> (3,1), interior tile, should succeed
    ck_assert_int_eq(room_try_push(r, 0, DIR_NORTH), OK);
    ck_assert_int_eq(r->pushables[0].x, 3);
    ck_assert_int_eq(r->pushables[0].y, 1);

    // push north again: (3,1) -> (3,0) which is a boundary wall, should fail
    ck_assert_int_eq(room_try_push(r, 0, DIR_NORTH), ROOM_IMPASSABLE);
    // position must not have changed
    ck_assert_int_eq(r->pushables[0].y, 1);

    // null / out-of-range error checks
    ck_assert_int_eq(room_try_push(NULL, 0, DIR_EAST), INVALID_ARGUMENT);
    ck_assert_int_eq(room_try_push(r, -1, DIR_EAST), INVALID_ARGUMENT);
    ck_assert_int_eq(room_try_push(r, 99, DIR_EAST), INVALID_ARGUMENT);
    ck_assert_int_eq(room_try_push(r, 0, (Direction)999), INVALID_ARGUMENT);

    room_destroy(r);
}
END_TEST

// test_room_try_push_onto_portal_and_treasure //
START_TEST(test_room_try_push_onto_portal_and_treasure)
{
    Room *r = room_create(23, "J", 6, 6);
    ck_assert_ptr_nonnull(r);

    // portal at (4,2), treasure at (2,2), pushable at (3,2)
    Portal *ports = calloc(1, sizeof(Portal));
    ck_assert_ptr_nonnull(ports);
    ports[0].id = 1;
    ports[0].x = 4;
    ports[0].y = 2;
    ports[0].target_room_id = 99;
    ports[0].name = test_dup("P");
    ck_assert_int_eq(room_set_portals(r, ports, 1), OK);

    Treasure *trs = calloc(1, sizeof(Treasure));
    ck_assert_ptr_nonnull(trs);
    trs[0].id = 10;
    trs[0].x = 2;
    trs[0].y = 2;
    trs[0].collected = false;
    trs[0].name = test_dup("T");
    ck_assert_int_eq(room_set_treasures(r, trs, 1), OK);

    Pushable *ps = calloc(1, sizeof(Pushable));
    ck_assert_ptr_nonnull(ps);
    ps[0].id = 1;
    ps[0].x = 3;
    ps[0].y = 2;
    ps[0].initial_x = 3;
    ps[0].initial_y = 2;
    ps[0].name = test_dup("Boulder");
    r->pushables = ps;
    r->pushable_count = 1;

    // pushing east into portal tile (4,2) should be blocked
    ck_assert_int_eq(room_try_push(r, 0, DIR_EAST), ROOM_IMPASSABLE);
    ck_assert_int_eq(r->pushables[0].x, 3); // unchanged

    // pushing west into treasure tile (2,2) should be blocked
    ck_assert_int_eq(room_try_push(r, 0, DIR_WEST), ROOM_IMPASSABLE);
    ck_assert_int_eq(r->pushables[0].x, 3); // unchanged

    room_destroy(r);
}
END_TEST

// test_room_has_pushable_at - covers found and not-found branches
START_TEST(test_room_has_pushable_at)
{
    Room *r = room_create(30, "K", 5, 5);
    ck_assert_ptr_nonnull(r);

    // no pushables yet
    ck_assert_int_eq(room_has_pushable_at(r, 2, 2, NULL), false);
    ck_assert_int_eq(room_has_pushable_at(NULL, 2, 2, NULL), false);

    // inject a pushable directly
    Pushable *ps = calloc(1, sizeof(Pushable));
    ck_assert_ptr_nonnull(ps);
    ps[0].id = 7;
    ps[0].x = 2;
    ps[0].y = 2;
    ps[0].initial_x = 2;
    ps[0].initial_y = 2;
    ps[0].name = malloc(5);
    memcpy(ps[0].name, "Rock", 5);
    r->pushables = ps;
    r->pushable_count = 1;

    // found - with and without idx_out
    int idx = -1;
    ck_assert_int_eq(room_has_pushable_at(r, 2, 2, &idx), true);
    ck_assert_int_eq(idx, 0);
    ck_assert_int_eq(room_has_pushable_at(r, 2, 2, NULL), true);

    // not at a different tile
    ck_assert_int_eq(room_has_pushable_at(r, 3, 3, NULL), false);

    room_destroy(r);
}
END_TEST

Suite *room_suite(void)
{
    Suite *s = suite_create("Room");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_room_create_clamps_dimensions);
    tcase_add_test(tc_core, test_room_is_walkable_implicit_walls);
    tcase_add_test(tc_core, test_room_set_floor_grid_controls_walkable);
    tcase_add_test(tc_core, test_room_treasure_and_portal_queries);
    tcase_add_test(tc_core, test_room_classify_tile_and_render);
    tcase_add_test(tc_core, test_room_get_start_position_prefers_first_portal);
    tcase_add_test(tc_core, test_room_get_start_position_finds_first_walkable);
    tcase_add_test(tc_core, test_room_place_treasure);
    tcase_add_test(tc_core, test_room_pick_up_treasure);
    tcase_add_test(tc_core, test_room_try_push_basic);
    tcase_add_test(tc_core, test_room_try_push_onto_portal_and_treasure);
    tcase_add_test(tc_core, test_room_null_guards);
    tcase_add_test(tc_core, test_room_has_pushable_at);

    suite_add_tcase(s, tc_core);
    return s;
}
