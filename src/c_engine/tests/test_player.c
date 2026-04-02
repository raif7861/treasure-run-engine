//CI rerun after adding the player implementation
#include <check.h>
#include <stdlib.h>
#include "player.h"


// player_create first //
START_TEST(test_player_create_valid)
{
    Player *p = NULL;
    Status st = player_create(1, 2, 3, &p);

    ck_assert_int_eq(st, OK);
    ck_assert_ptr_nonnull(p);
    ck_assert_int_eq(player_get_room(p), 1);

    int x = 1, y = 1;
    ck_assert_int_eq(player_get_position(p, &x, &y), OK);
    ck_assert_int_eq(x, 2);
    ck_assert_int_eq(y, 3);

    player_destroy(p);
}
END_TEST

// test_player_create_null_out //
START_TEST(test_player_create_null_out)
{
    Status st = player_create(0, 0, 0, NULL);
    ck_assert_int_eq(st, INVALID_ARGUMENT);
}
END_TEST

// player_set_position //
START_TEST(test_player_set_position)
{
    Player *p= NULL;
    Status st = player_create(0, 0, 0, &p);
    ck_assert_int_eq(st, OK);
    ck_assert_ptr_nonnull(p);

    ck_assert_int_eq(player_set_position(p, 5, 6), OK);

    int x = 0, y = 0;
    ck_assert_int_eq(player_get_position(p, &x, &y), OK);

    ck_assert_int_eq(x, 5);
    ck_assert_int_eq(y, 6);

    player_destroy(p);
}
END_TEST

// player_move_to_room //

START_TEST(test_player_move_to_room)
{
    Player *p = NULL;
    player_create(0, 0, 0, &p);

    ck_assert_int_eq(player_move_to_room(p, 42), OK);
    ck_assert_int_eq(player_get_room(p), 42);

    player_destroy(p);
}
END_TEST

// player_reset_to_start //
START_TEST(test_player_reset_to_start)
{
    Player *p = NULL;
    player_create(5, 9, 9, &p);

    ck_assert_int_eq(player_reset_to_start(p, 1, 2, 3), OK);

    int x = 0, y = 0;
    player_get_position(p, &x, &y);

    ck_assert_int_eq(player_get_room(p), 1);
    ck_assert_int_eq(x, 2);
    ck_assert_int_eq(y, 3);

    player_destroy(p);
}
END_TEST




// test_player_null_guards - covers null checks in get_position, set_position,
// move_to_room, reset_to_start, get_collected_count
START_TEST(test_player_null_guards)
{
    int x = 0, y = 0;

    // get_position null checks
    ck_assert_int_ne(player_get_position(NULL, &x, &y), OK);

    Player *p = NULL;
    ck_assert_int_eq(player_create(0, 1, 2, &p), OK);
    ck_assert_int_ne(player_get_position(p, NULL, &y), OK);
    ck_assert_int_ne(player_get_position(p, &x, NULL), OK);

    // set_position null check
    ck_assert_int_ne(player_set_position(NULL, 1, 1), OK);

    // move_to_room null check
    ck_assert_int_ne(player_move_to_room(NULL, 1), OK);

    // reset_to_start null check
    ck_assert_int_ne(player_reset_to_start(NULL, 0, 0, 0), OK);

    // get_collected_count null check
    ck_assert_int_eq(player_get_collected_count(NULL), 0);

    player_destroy(p);
}
END_TEST

// test_player_get_collected_treasures - covers the returned list contents
START_TEST(test_player_get_collected_treasures)
{
    Player *p = NULL;
    ck_assert_int_eq(player_create(0, 0, 0, &p), OK);

    // empty at start
    int count = -1;
    const Treasure * const *list = player_get_collected_treasures(p, &count);
    ck_assert_int_eq(count, 0);

    // collect one, verify list has it
    Treasure t;
    memset(&t, 0, sizeof(Treasure));
    t.id = 5;
    t.collected = false;
    ck_assert_int_eq(player_try_collect(p, &t), OK);

    list = player_get_collected_treasures(p, &count);
    ck_assert_int_eq(count, 1);
    ck_assert_ptr_nonnull(list);
    ck_assert_int_eq(list[0]->id, 5);

    player_destroy(p);
}
END_TEST

Suite *player_suite(void)
{
    Suite *s = suite_create("Player");
    TCase *tc = tcase_create("Core");

    tcase_add_test(tc, test_player_create_valid);
    tcase_add_test(tc, test_player_create_null_out);
    tcase_add_test(tc, test_player_set_position);
    tcase_add_test(tc, test_player_move_to_room);
    tcase_add_test(tc, test_player_reset_to_start);
    tcase_add_test(tc, test_player_null_guards);
    tcase_add_test(tc, test_player_get_collected_treasures);

    suite_add_tcase(s, tc);
    return s;
}
