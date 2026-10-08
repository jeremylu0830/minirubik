/* Shared by the host-side gate checkers: solver.c as the exact oracle, the
 * H2 table check, and helpers for printing states.
 */
#ifndef ORACLE_H
#define ORACLE_H

#define main solver_main
#include "../solver.c"
#undef main

#define UNSET UINT8_MAX

static int h2_failed;

/* H2: every entry written, the solved entry is 0, and report the maximum. */
static void check_table(const char *name, const uint8_t *table, uint32_t size,
                        uint32_t solved)
{
    uint32_t unset = 0;
    uint8_t max = 0;
    for (uint32_t i = 0; i < size; ++i) {
        if (table[i] == UNSET)
            ++unset;
        else if (table[i] > max)
            max = table[i];
    }
    int ok = !unset && solved < size && table[solved] == 0;
    printf("H2 %-16s %8u entries  unset %u  solved[%u] = %u  max %u  %s\n",
           name, size, unset, solved, solved < size ? table[solved] : UNSET,
           max, ok ? "PASS" : "FAIL");
    h2_failed |= !ok;
}

/* Exact distance of every rank, by walking solver.c's table back to rank 0. */
static uint8_t *exact_distances(void)
{
    uint8_t diameter;
    uint8_t *table = build_table(&diameter);
    uint8_t *dist = malloc(STATES);
    if (!table || !dist) {
        free(table);
        free(dist);
        return NULL;
    }
    for (uint32_t rank = 0; rank < STATES; ++rank) {
        state_t walk;
        unrank_state(rank, &walk);
        uint8_t d = 0;
        for (uint32_t r = rank; r; r = rank_state(&walk)) {
            walk = apply_move(walk, table[r]);
            ++d;
        }
        dist[rank] = d;
    }
    free(table);
    return dist;
}

static void print_state(const state_t *state)
{
    for (uint8_t i = 0; i < CUBIES; ++i)
        putchar('1' + state->p[i]);
    for (uint8_t i = 0; i < CUBIES; ++i)
        putchar('1' + state->o[i]);
}

#endif
