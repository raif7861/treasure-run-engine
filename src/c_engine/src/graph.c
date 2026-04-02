#include "graph.h"

#include <stdlib.h>
#include <string.h>

#define INITIAL_NODE_CAPACITY 8
#define INITIAL_neighbor_CAPACITY 4

static const void *const K_EMPTY_PAYLOAD_LIST[1] = { NULL };
static const void *const K_EMPTY_NEIGHBOR_LIST[1] = { NULL };

typedef struct Node {
    void *payload;
    void **neighbors;
    int neighbor_count;
    int neighbor_capacity;
} Node;

struct Graph {
    GraphCompareFn compare;
    GraphDestroyFn destroy;
    Node *nodes;
    void **payloads;
    int node_count;
    int node_capacity;
    int payload_capacity;
    int edge_count;
};

/* --------------------------------------------------
 * Private helper prototypes
 * -------------------------------------------------- */
static bool graph_get_node_index_internal(const Graph *g,
                                          const void *payload,
                                          int *index_out);

static bool graph_get_node_internal(const Graph *g,
                                    const void *payload,
                                    Node **node_out);

static GraphStatus graph_ensure_node_capacity(Graph *g, int min_capacity);
static GraphStatus graph_node_ensure_neighbor_capacity(Node *node);
static bool graph_node_remove_neighbor(Node *node,
                                       const void *payload,
                                       GraphCompareFn compare);
static bool graph_has_cycle_from(const Graph *g, int start_idx, int *states);

/* --------------------------------------------------
 * Public API
 * -------------------------------------------------- */

GraphStatus graph_create(GraphCompareFn compare_fn,
                         GraphDestroyFn destroy_fn,
                         Graph **graph_out)
{
    if (!compare_fn || !graph_out) {
        return GRAPH_STATUS_NULL_ARGUMENT;
    }

    Graph *g = calloc(1, sizeof(Graph));
    if (!g) {
        return GRAPH_STATUS_NO_MEMORY;
    }

    g->compare = compare_fn;
    g->destroy = destroy_fn;
    *graph_out = g;
    return GRAPH_STATUS_OK;
}

void graph_destroy(Graph *g)
{
    if (!g) {
        return;
    }

    for (int i = 0; i < g->node_count; ++i) {
        Node *node = &g->nodes[i];

        if (g->destroy && node->payload) {
            g->destroy(node->payload);
        }

        free(node->neighbors);
    }

    free(g->nodes);
    free(g->payloads);
    free(g);
}

GraphStatus graph_insert(Graph *g, void *payload)
{
    if (!g || !payload) {
        return GRAPH_STATUS_NULL_ARGUMENT;
    }

    int existing_index = 0;
    if (graph_get_node_index_internal(g, payload, &existing_index)) {
        return GRAPH_STATUS_DUPLICATE_PAYLOAD;
    }

    GraphStatus status =
        graph_ensure_node_capacity(g, g->node_count + 1);
    if (status != GRAPH_STATUS_OK) {
        return status;
    }

    Node *node = &g->nodes[g->node_count];
    node->payload = payload;
    node->neighbors = NULL;
    node->neighbor_count = 0;
    node->neighbor_capacity = 0;

    g->payloads[g->node_count] = payload;
    g->node_count += 1;

    return GRAPH_STATUS_OK;
}

GraphStatus graph_connect(Graph *g,
                          const void *from,
                          const void *to)
{
    if (!g || !from || !to) {
        return GRAPH_STATUS_NULL_ARGUMENT;
    }

    Node *from_node = NULL;
    Node *to_node = NULL;

    if (!graph_get_node_internal(g, from, &from_node) ||
        !graph_get_node_internal(g, to, &to_node)) {
        return GRAPH_STATUS_NOT_FOUND;
    }

    for (int i = 0; i < from_node->neighbor_count; ++i) {
        if (g->compare(from_node->neighbors[i], to) == 0) {
            return GRAPH_STATUS_DUPLICATE_EDGE;
        }
    }

    GraphStatus status =
        graph_node_ensure_neighbor_capacity(from_node);
    if (status != GRAPH_STATUS_OK) {
        return status;
    }

    from_node->neighbors[from_node->neighbor_count] = (void *)to;
    from_node->neighbor_count += 1;
    g->edge_count += 1;

    return GRAPH_STATUS_OK;
}

GraphStatus graph_get_neighbors(const Graph *g,
                                const void *payload,
                                const void * const **neighbors_out,
                                int *count_out)
{
    if (!g || !payload || !neighbors_out || !count_out) {
        return GRAPH_STATUS_NULL_ARGUMENT;
    }

    Node *node = NULL;
    if (!graph_get_node_internal(g, payload, &node)) {
        return GRAPH_STATUS_NOT_FOUND;
    }

    if (node->neighbor_count > 0) {
        *neighbors_out = (const void * const *)node->neighbors;
    } else {
        *neighbors_out = K_EMPTY_NEIGHBOR_LIST;
    }

    *count_out = node->neighbor_count;
    return GRAPH_STATUS_OK;
}

int graph_size(const Graph *g)
{
    if (!g) {
        return 0;
    }
    return g->node_count;
}

bool graph_contains(const Graph *g, const void *payload)
{
    int idx = 0;
    return graph_get_node_index_internal(g, payload, &idx);
}

int graph_outdegree(const Graph *g, const void *payload)
{
    Node *node = NULL;
    if (!graph_get_node_internal(g, payload, &node)) {
        return 0;
    }
    return node->neighbor_count;
}

bool graph_has_edge(const Graph *g,
                    const void *from,
                    const void *to)
{
    Node *node = NULL;
    if (!graph_get_node_internal(g, from, &node)) {
        return false;
    }

    for (int i = 0; i < node->neighbor_count; ++i) {
        if (g->compare(node->neighbors[i], to) == 0) {
            return true;
        }
    }

    return false;
}

int graph_indegree(const Graph *g, const void *payload)
{
    if (!g || !payload) {
        return 0;
    }

    int count = 0;

    for (int i = 0; i < g->node_count; ++i) {
        if (g->compare(g->nodes[i].payload, payload) == 0) {
            continue;
        }

        for (int j = 0; j < g->nodes[i].neighbor_count; ++j) {
            if (g->compare(g->nodes[i].neighbors[j], payload) == 0) {
                count += 1;
            }
        }
    }

    return count;
}

int graph_edge_count(const Graph *g)
{
    if (!g) {
        return 0;
    }
    return g->edge_count;
}

GraphStatus graph_get_all_payloads(const Graph *g,
                                   const void * const **payloads_out,
                                   int *count_out)
{
    if (!g || !payloads_out || !count_out) {
        return GRAPH_STATUS_NULL_ARGUMENT;
    }

    if (g->node_count > 0) {
        *payloads_out = (const void * const *)g->payloads;
    } else {
        *payloads_out = K_EMPTY_PAYLOAD_LIST;
    }

    *count_out = g->node_count;
    return GRAPH_STATUS_OK;
}

bool graph_reachable(const Graph *g, const void *from, const void *to)
{
    int start = 0;
    int target = 0;

    if (!g || !from || !to) {
        return false;
    }

    if (!graph_get_node_index_internal(g, from, &start) ||
        !graph_get_node_index_internal(g, to, &target)) {
        return false;
    }

    if (start == target) {
        return true;
    }

    bool *visited = calloc(g->node_count, sizeof(bool));
    int *stack = malloc(sizeof(int) * g->node_count);
    if (!visited || !stack) {
        free(visited);
        free(stack);
        return false;
    }

    int top = 0;
    stack[top++] = start;
    visited[start] = true;

    while (top > 0) {
        int current = stack[--top];
        Node *node = &g->nodes[current];

        for (int i = 0; i < node->neighbor_count; ++i) {
            int neighbor_idx = 0;
            if (!graph_get_node_index_internal(g, node->neighbors[i], &neighbor_idx)) {
                continue;
            }

            if (neighbor_idx == target) {
                free(stack);
                free(visited);
                return true;
            }

            if (!visited[neighbor_idx]) {
                visited[neighbor_idx] = true;
                stack[top++] = neighbor_idx;
            }
        }
    }

    free(stack);
    free(visited);
    return false;
}

bool graph_has_cycle(const Graph *g)
{
    if (!g || g->node_count == 0) {
        return false;
    }

    int *states = calloc(g->node_count, sizeof(int));
    if (!states) {
        return false;
    }

    bool found = false;

    for (int i = 0; i < g->node_count; ++i) {
        if (states[i] != 0) {
            continue;
        }

        if (graph_has_cycle_from(g, i, states)) {
            found = true;
            break;
        }
    }

    free(states);
    return found;
}

bool graph_is_connected(const Graph *g)
{
    if (!g || g->node_count <= 1) {
        return true;
    }

    bool *visited = calloc(g->node_count, sizeof(bool));
    int *stack = malloc(sizeof(int) * g->node_count);

    if (!visited || !stack) {
        free(visited);
        free(stack);
        return false;
    }

    int top = 0;
    stack[top++] = 0;
    visited[0] = true;

    while (top > 0) {
        int current = stack[--top];
        Node *node = &g->nodes[current];

        for (int i = 0; i < node->neighbor_count; ++i) {
            int neighbor_idx = 0;
            if (!graph_get_node_index_internal(g, node->neighbors[i], &neighbor_idx)) {
                continue;
            }

            if (visited[neighbor_idx]) {
                continue;
            }

            visited[neighbor_idx] = true;
            stack[top++] = neighbor_idx;
        }
    }

    bool all_visited = true;
    for (int i = 0; i < g->node_count; ++i) {
        if (!visited[i]) {
            all_visited = false;
            break;
        }
    }

    free(stack);
    free(visited);
    return all_visited;
}

GraphStatus graph_disconnect(Graph *g,
                             const void *from,
                             const void *to)
{
    Node *from_node = NULL;
    Node *to_node = NULL;

    if (!graph_get_node_internal(g, from, &from_node) ||
        !graph_get_node_internal(g, to, &to_node)) {
        return GRAPH_STATUS_NOT_FOUND;
    }

    if (!graph_node_remove_neighbor(from_node, to, g->compare)) {
        return GRAPH_STATUS_NOT_FOUND;
    }

    g->edge_count -= 1;
    return GRAPH_STATUS_OK;
}

GraphStatus graph_remove(Graph *g, const void *payload)
{
    int idx = 0;
    if (!graph_get_node_index_internal(g, payload, &idx)) {
        return GRAPH_STATUS_NOT_FOUND;
    }

    Node *target = &g->nodes[idx];

    int outgoing = target->neighbor_count;
    free(target->neighbors);

    int incoming = 0;
    for (int i = 0; i < g->node_count; ++i) {
        if (i == idx) {
            continue;
        }

        if (graph_node_remove_neighbor(&g->nodes[i],
                                       payload,
                                       g->compare)) {
            incoming += 1;
        }
    }

    g->edge_count -= (outgoing + incoming);

    if (g->destroy) {
        g->destroy((void *)payload);
    }

    if (idx != g->node_count - 1) {
        memmove(&g->nodes[idx],
                &g->nodes[idx + 1],
                sizeof(Node) * (g->node_count - idx - 1));

        memmove(&g->payloads[idx],
                &g->payloads[idx + 1],
                sizeof(void *) * (g->node_count - idx - 1));
    }

    g->node_count -= 1;
    return GRAPH_STATUS_OK;
}

const void *graph_get_payload(const Graph *g, const void *key)
{
    Node *node = NULL;
    if (!graph_get_node_internal(g, key, &node)) {
        return NULL;
    }
    return node->payload;
}

/* --------------------------------------------------
 * Private helper definitions
 * -------------------------------------------------- */

static bool graph_get_node_index_internal(const Graph *g,
                                          const void *payload,
                                          int *index_out)
{
    if (!g || !payload || !index_out) {
        return false;
    }

    for (int i = 0; i < g->node_count; ++i) {
        if (g->compare(g->nodes[i].payload, payload) == 0) {
            *index_out = i;
            return true;
        }
    }

    return false;
}

static bool graph_get_node_internal(const Graph *g,
                                    const void *payload,
                                    Node **node_out)
{
    int idx = 0;

    if (!graph_get_node_index_internal(g, payload, &idx)) {
        return false;
    }

    *node_out = &g->nodes[idx];
    return true;
}

static GraphStatus graph_ensure_node_capacity(Graph *g, int min_capacity)
{
    if (g->node_capacity >= min_capacity) {
        return GRAPH_STATUS_OK;
    }

    int target = (g->node_capacity > 0)
                 ? g->node_capacity
                 : INITIAL_NODE_CAPACITY;

    while (target < min_capacity) {
        target *= 2;
    }

    Node *new_nodes = malloc(sizeof(Node) * target);
    void **new_payloads = malloc(sizeof(void *) * target);

    if (!new_nodes || !new_payloads) {
        free(new_nodes);
        free(new_payloads);
        return GRAPH_STATUS_NO_MEMORY;
    }

    memcpy(new_nodes, g->nodes, sizeof(Node) * g->node_count);
    memcpy(new_payloads, g->payloads, sizeof(void *) * g->node_count);

    free(g->nodes);
    free(g->payloads);

    g->nodes = new_nodes;
    g->payloads = new_payloads;
    g->node_capacity = target;
    g->payload_capacity = target;

    return GRAPH_STATUS_OK;
}

static GraphStatus graph_node_ensure_neighbor_capacity(Node *node)
{
    if (node->neighbor_capacity > node->neighbor_count) {
        return GRAPH_STATUS_OK;
    }

    int target = (node->neighbor_capacity > 0)
                 ? node->neighbor_capacity * 2
                 : INITIAL_neighbor_CAPACITY;

    void **new_neighbors =
        realloc(node->neighbors, sizeof(void *) * target);

    if (!new_neighbors) {
        return GRAPH_STATUS_NO_MEMORY;
    }

    node->neighbors = new_neighbors;
    node->neighbor_capacity = target;
    return GRAPH_STATUS_OK;
}

static bool graph_node_remove_neighbor(Node *node,
                                       const void *payload,
                                       GraphCompareFn compare)
{
    if (!node || !payload) {
        return false;
    }

    for (int i = 0; i < node->neighbor_count; ++i) {
        if (compare(node->neighbors[i], payload) == 0) {
            if (i < node->neighbor_count - 1) {
                memmove(&node->neighbors[i],
                        &node->neighbors[i + 1],
                        sizeof(void *) * (node->neighbor_count - i - 1));
            }
            node->neighbor_count -= 1;
            return true;
        }
    }

    return false;
}

typedef struct {
    int node_idx;
    int next_neighbor;
} GraphDfsFrame;

static bool graph_has_cycle_from(const Graph *g, int start_idx, int *states)
{
    GraphDfsFrame *stack =
        malloc(sizeof(GraphDfsFrame) * g->node_count);
    if (!stack) {
        return false;
    }

    int top = 0;
    stack[top++] = (GraphDfsFrame){ .node_idx = start_idx, .next_neighbor = 0 };
    states[start_idx] = 1; /* visiting */

    while (top > 0) {
        GraphDfsFrame *frame = &stack[top - 1];
        Node *node = &g->nodes[frame->node_idx];

        if (frame->next_neighbor >= node->neighbor_count) {
            states[frame->node_idx] = 2; /* visited */
            top -= 1;
            continue;
        }

        void *neighbor_payload = node->neighbors[frame->next_neighbor];
        frame->next_neighbor += 1;

        int neighbor_idx = 0;
        if (!graph_get_node_index_internal(g, neighbor_payload, &neighbor_idx)) {
            continue;
        }

        if (states[neighbor_idx] == 1) {
            free(stack);
            return true;
        }

        if (states[neighbor_idx] == 0) {
            states[neighbor_idx] = 1; /* visiting */
            stack[top++] =
                (GraphDfsFrame){ .node_idx = neighbor_idx, .next_neighbor = 0 };
        }
    }

    free(stack);
    return false;
}
