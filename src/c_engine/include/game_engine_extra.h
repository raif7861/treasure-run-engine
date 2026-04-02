#ifndef GAME_ENGINE_EXTRA_H
#define GAME_ENGINE_EXTRA_H

#include "game_engine.h"

/*
 * game_engine_get_total_treasure_count
 * -----------------------------------
 * Returns the total number of treasures across all rooms in the world.
 * Used by the Python UI to show collection progress (example - 3/10 collected).
 *
 * Parameters:
 *   eng       - the game engine (must not be NULL)
 *   count_out - receives the total treasure count on success
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if count_out is NULL
 *   INTERNAL_ERROR if graph is invalid
 */
Status game_engine_get_total_treasure_count(const GameEngine *eng,
                                            int *count_out);


/*
 * game_engine_get_adjacency_matrix
 * ---------------------------------
 * Returns an adjacency matrix representing room connections.
 * Used by the Python UI to draw the minimap.
 * matrix_out is a flattened n*n array where matrix[i*n+j]=1
 * means room i connects to room j via a portal.
 *
 * Caller must free() both matrix_out and ids_out.
 */
Status game_engine_get_adjacency_matrix(const GameEngine *eng,
                                        int **matrix_out,
                                        int **ids_out,
                                        int *n_out);
#endif /* GAME_ENGINE_EXTRA_H */