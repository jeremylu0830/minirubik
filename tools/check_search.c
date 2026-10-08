/* Gate H3 for a search, checked on the host against the exact BFS distances
 * from solver.c, plus node counts for finding the worst case.
 *
 * The search under test lives in its own C file, included below through
 * SEARCH_SOURCE (a path relative to this file). If it relies on a separate
 * heuristic file, name that one in HEURISTIC_SOURCE and it is included
 * first. Both see everything in solver.c and must not define main. The
 * search file must define:
 *
 *   static void search_init(void);
 *       Build or load every table the search uses.
 *   static uint8_t search(const state_t *state, uint8_t moves[],
 *                         uint64_t *nodes);
 *       Write a solution into moves[] as solver.c move numbers
 *       (0 = R, 1 = R2, 2 = R', 3 = B, ..., 8 = D') and return its length.
 *       Add the number of nodes expanded to *nodes.
 *
 * Build from the repository root:
 *   cc -O2 -std=c99 -DHEURISTIC_SOURCE='"../heuristic.c"' \
 *      -DSEARCH_SOURCE='"../search.c"' tools/check_search.c -o check_search
 *
 * Run:
 *   ./check_search                       every state (gate H3, minutes)
 *   ./check_search tests/distance11.txt  only the states listed in a file
 *
 * A result passes when its length equals the exact distance, every move is
 * in range, and applying the moves reaches the solved state. Exit status is
 * 0 only if every checked state passes.
 */
#include <time.h>

#include "oracle.h"

#ifdef HEURISTIC_SOURCE
#include HEURISTIC_SOURCE
#endif
#ifndef SEARCH_SOURCE
#error "define SEARCH_SOURCE, e.g. -DSEARCH_SOURCE='\"../search.c\"'"
#endif
#include SEARCH_SOURCE

enum { MAX_MOVES = 32, WORST = 10, REPORTED = 10 };

typedef struct {
    uint64_t nodes;
    uint32_t rank;
    uint8_t distance;
} worst_t;

static worst_t worst[WORST];
static uint32_t count[12], wrong_length, bad_move, unsolved;
static uint64_t node_sum[12], node_max[12];

static void report(const char *what, const state_t *state, uint8_t d,
                   uint8_t length)
{
    if (wrong_length + bad_move + unsolved > REPORTED)
        return;
    printf("H3 %s: ", what);
    print_state(state);
    printf("  d = %u, returned length %u\n", d, length);
}

static void keep_if_worst(uint64_t nodes, uint32_t rank, uint8_t d)
{
    if (nodes <= worst[WORST - 1].nodes)
        return;
    int i = WORST - 1;
    for (; i > 0 && worst[i - 1].nodes < nodes; --i)
        worst[i] = worst[i - 1];
    worst[i] = (worst_t) {nodes, rank, d};
}

static void check_one(uint32_t rank, uint8_t d)
{
    state_t state, walk;
    uint8_t moves[MAX_MOVES];
    uint64_t nodes = 0;
    unrank_state(rank, &state);
    uint8_t length = search(&state, moves, &nodes);

    ++count[d];
    node_sum[d] += nodes;
    if (nodes > node_max[d])
        node_max[d] = nodes;
    keep_if_worst(nodes, rank, d);

    if (length != d) {
        ++wrong_length;
        report("wrong length", &state, d, length);
        return;
    }
    walk = state;
    for (uint8_t i = 0; i < length; ++i) {
        if (moves[i] >= MOVES) {
            ++bad_move;
            report("move out of range", &state, d, length);
            return;
        }
        walk = apply_move(walk, moves[i]);
    }
    if (rank_state(&walk) != 0) {
        ++unsolved;
        report("moves do not solve", &state, d, length);
    }
}

int main(int argc, char **argv)
{
    if (argc > 2) {
        fputs("usage: check_search [STATE_FILE]\n", stderr);
        return 2;
    }
    uint8_t *dist = exact_distances();
    if (!dist) {
        fputs("could not compute exact distances\n", stderr);
        return 1;
    }
    search_init();

    clock_t start = clock();
    uint32_t checked = 0;
    if (argc == 1) {
        for (uint32_t rank = 0; rank < STATES; ++rank, ++checked)
            check_one(rank, dist[rank]);
    } else {
        FILE *in = fopen(argv[1], "r");
        if (!in) {
            perror(argv[1]);
            return 1;
        }
        char line[64];
        while (fgets(line, sizeof line, in)) {
            line[strcspn(line, "\r\n")] = '\0';
            state_t state;
            if (!*line || line[0] == '#')
                continue;
            if (!parse_state(line, &state)) {
                fprintf(stderr, "invalid state: %s\n", line);
                return 1;
            }
            uint32_t rank = rank_state(&state);
            check_one(rank, dist[rank]);
            ++checked;
        }
        fclose(in);
    }
    double seconds = (double) (clock() - start) / CLOCKS_PER_SEC;

    uint32_t failed = wrong_length + bad_move + unsolved;
    printf("H3 %s: %u states checked, %u wrong length, %u move out of range, "
           "%u not solved  %s\n",
           argc == 1 ? "all states" : argv[1], checked, wrong_length,
           bad_move, unsolved, failed ? "FAIL" : "PASS");
    if (argc != 1)
        puts("   (a subset: gate H3 needs the run over every state)");
    printf("   search time %.1f s\n\n", seconds);

    puts(" d    states     mean nodes      max nodes");
    for (uint8_t d = 0; d < 12; ++d)
        if (count[d])
            printf("%2u  %8u  %13.1f  %13llu\n", d, count[d],
                   (double) node_sum[d] / count[d],
                   (unsigned long long) node_max[d]);

    puts("\nmost nodes expanded:");
    for (int i = 0; i < WORST && worst[i].nodes; ++i) {
        state_t state;
        unrank_state(worst[i].rank, &state);
        printf("  ");
        print_state(&state);
        printf("  d = %2u  nodes = %llu\n", worst[i].distance,
               (unsigned long long) worst[i].nodes);
    }

    free(dist);
    return failed != 0;
}
