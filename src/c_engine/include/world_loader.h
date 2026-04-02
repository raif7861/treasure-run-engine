#ifndef WORLD_LOADER_H
#define WORLD_LOADER_H

#include "types.h"   /* Room, Charset */


typedef struct Graph Graph;

/* ============================================================
 * World Loader
 *
 * Responsibilities:
 *   • Consume datagen library output (generator-owned memory)
 *   • Deep-copy room data into student-owned Room structs
 *   • Build a connectivity Graph from room neighbor relationships
 *   • Cache the Charset provided by datagen
 *
 * The loader bridges the gap between datagen's temporary output
 * and the student's persistent data structures.
 *
 * IMPORTANT:
 *   All datagen pointers are temporary.
 *   The loader performs deep copies so that room data remains
 *   valid after datagen is shut down.
 * ============================================================ */

/* ============================================================
 * loader_load_world
 * ============================================================
 *
 * Load a world by:
 *   1. Passing the config file to datagen
 *   2. Iterating through datagen output
 *   3. Deep-copying datagen data to add to Room structs
 *   4. Building a connectivity graph of Room structs and returning it to the game engine
 *   5. Passing the charset back to the game engine
 *
 * Parameters:
 *   config_file:
 *     Path to the datagen configuration file.
 *
 * Outputs:
 *   graph_out:
 *     On success, receives a pointer to a newly created Graph.
 *
 *   first_room_out:
 *     On success, receives a pointer to the first room inserted
 *     into the graph.
 *
 *   num_rooms_out:
 *     On success, receives the total number of rooms loaded.
 *
 *   charset_out:
 *     On success, receives the loaded charset.
 *
 * Returns:
 *   OK on success
 *   WL_ERR_CONFIG if the config path is invalid
 *   WL_ERR_DATAGEN if datagen fails
 *   NO_MEMORY on allocation failure
 *
 * Ownership:
 *   - The caller owns the returned Graph.
 *   - All Room structs are owned by the Graph as payloads.
 *   - Destroying the Graph frees all Rooms.
 */
Status loader_load_world(const char *config_file,
                         Graph **graph_out,
                         Room **first_room_out,
                         int  *num_rooms_out,
                         Charset *charset_out);


#endif /* WORLD_LOADER_H */
