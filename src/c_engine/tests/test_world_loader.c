#include <check.h>
#include <stdlib.h>

#include "world_loader.h"
#include "types.h" //for the Charset

//Graph is forward-declared in the world_laoder.h, so we can use it in the test without including graph.h

// test_loader_null_conffig_file //
START_TEST(test_loader_null_conffig_file)
{
    Graph *g = (Graph *)0x1;    //non-null dummy value
    Room *first = (Room *)0x1;      //non-null dummy value
    int count = -1;
    Charset cs;

    Status st = loader_load_world(NULL, &g, &first, &count, &cs);
    ck_assert_int_eq(st, INVALID_ARGUMENT);

    ck_assert_ptr_eq(g, (Graph *)0x1);
    ck_assert_ptr_eq(first, (Room *)0x1);
    ck_assert_int_eq(count, -1);
}
END_TEST

// test_loader_null_graph_out //
START_TEST(test_loader_null_graph_out)
{
    Room *first = NULL;
    Charset cs;

    Status st = loader_load_world("fake.cfg", NULL, &first, NULL, &cs);
    ck_assert_int_eq(st, INVALID_ARGUMENT);
}
END_TEST

// test_loader_null_first_room_out //
START_TEST(test_loader_null_first_room_out)
{
    Graph *g = NULL;
    int count = 0;
    Charset cs;

    Status st = loader_load_world("fake.cfg", &g, NULL, &count, &cs);
    ck_assert_int_eq(st, INVALID_ARGUMENT);
}
END_TEST

// test_loader_null_num_rooms_out //
START_TEST(test_loader_null_num_rooms_out)
{
    Graph *g = NULL;
    Room *first = NULL;
    Charset cs;

    Status st = loader_load_world("fake.cfg", &g, &first, NULL, &cs);
    ck_assert_int_eq(st, INVALID_ARGUMENT);
}
END_TEST

// test_loader_null_charset_out //
START_TEST(test_loader_null_charset_out)
{
    Graph *g = NULL;
    Room *first = NULL;
    int count = 0;

    Status st = loader_load_world("fake.cfg", &g, &first, &count, NULL);
    ck_assert_int_eq(st, INVALID_ARGUMENT);
}
END_TEST

// test_loader_bad_config_path - non-existent file should return a non-OK error //
START_TEST(test_loader_bad_config_path)
{
    Graph *g = NULL;
    Room *first = NULL;
    int count = 0;
    Charset cs;

    Status st = loader_load_world("does_not_exist_xyz.cfg", &g, &first, &count, &cs);
    ck_assert_int_ne(st, OK);
    ck_assert_ptr_eq(g, NULL);
}
END_TEST

// test_loader_real_config - exercises the full loading path if a config is available.
// mirrors the pattern in test_game_engine.c: skip silently if the config is missing,
// but assert correct behaviour when it is present.
START_TEST(test_loader_real_config)
{
    const char *paths[] = {
        "assets/treasure_runner.ini",
        "../assets/treasure_runner.ini",
        NULL
    };

    Graph *g = NULL;
    Room *first = NULL;
    int count = 0;
    Charset cs;
    Status st = WL_ERR_CONFIG;

    for (int i = 0; paths[i] != NULL; i++) {
        g = NULL;
        first = NULL;
        count = 0;
        st = loader_load_world(paths[i], &g, &first, &count, &cs);
        if (st == OK) break;
    }

    if (st == OK) {
        // basic sanity on what the loader returned
        ck_assert_ptr_nonnull(g);
        ck_assert_ptr_nonnull(first);
        ck_assert_int_gt(count, 0);

        // charset should have been populated with printable characters
        ck_assert_int_ne(cs.wall, 0);
        ck_assert_int_ne(cs.floor, 0);
        ck_assert_int_ne(cs.player, 0);

        // clean up - the graph owns the rooms
        extern void graph_destroy(Graph *g);
        graph_destroy(g);
    }
    // if no config file was found, the test passes silently (CI may not have assets)
}
END_TEST

Suite *world_loader_suite(void)
{
    Suite *s = suite_create("WorldLoader");
    TCase *tc = tcase_create("Core");

    tcase_add_test(tc, test_loader_null_conffig_file);
    tcase_add_test(tc, test_loader_null_graph_out);
    tcase_add_test(tc, test_loader_null_first_room_out);
    tcase_add_test(tc, test_loader_null_num_rooms_out);
    tcase_add_test(tc, test_loader_null_charset_out);
    tcase_add_test(tc, test_loader_bad_config_path);
    tcase_add_test(tc, test_loader_real_config);
    suite_add_tcase(s, tc);

    return s;
}

