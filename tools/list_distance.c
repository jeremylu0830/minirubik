/* List every state at a given HTM distance from solved, one 14-digit vector
 * per line in rank order. solver.c's BFS table is the oracle: the distance
 * of a state is the number of table moves that walk it back to rank 0.
 * The per-distance histogram goes to stderr so it can be checked against
 * report.md without touching the list on stdout.
 */
#define main solver_main
#include "../solver.c"
#undef main

int main(int argc, char **argv)
{
    char *end;
    long want = argc == 2 ? strtol(argv[1], &end, 10) : -1;
    if (argc != 2 || *end || want < 0 || want > 11) {
        fputs("usage: list_distance DISTANCE  (0 to 11)\n", stderr);
        return 2;
    }
    uint8_t diameter;
    uint8_t *table = build_table(&diameter);
    if (!table) {
        fputs("could not build complete state table\n", stderr);
        return 1;
    }
    uint32_t count[12] = {0};
    for (uint32_t rank = 0; rank < STATES; ++rank) {
        state_t state, walk;
        unrank_state(rank, &state);
        walk = state;
        uint8_t distance = 0;
        for (uint32_t r = rank; r; r = rank_state(&walk)) {
            walk = apply_move(walk, table[r]);
            if (++distance > 11) {
                fprintf(stderr, "rank %u does not reach solved\n", rank);
                return 1;
            }
        }
        ++count[distance];
        if (distance == want) {
            for (uint8_t i = 0; i < CUBIES; ++i)
                putchar('1' + state.p[i]);
            for (uint8_t i = 0; i < CUBIES; ++i)
                putchar('1' + state.o[i]);
            putchar('\n');
        }
    }
    free(table);
    for (uint8_t d = 0; d < 12; ++d)
        fprintf(stderr, "distance %2u: %7u\n", d, count[d]);
    return output_failed();
}
