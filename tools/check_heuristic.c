/* Gates H1 and H2 for a heuristic, checked on the host against the exact
 * BFS distances from solver.c.
 *
 * The heuristic under test lives in its own C file, included below through
 * HEURISTIC_SOURCE (a path relative to this file). It sees everything in
 * solver.c (state_t, quarter_turn, rank_state, unrank_state, STATES, ...)
 * and must define:
 *
 *   static void heuristic_init(void);
 *       Build or load every table the heuristic uses.
 *   static void heuristic_tables(void);
 *       Call check_table() once per table, for gate H2.
 *   static uint8_t heuristic(const state_t *state, uint32_t rank);
 *       A lower bound on the HTM distance of state; rank is
 *       rank_state(state), passed in so it need not be recomputed.
 *
 * Build and run from the repository root:
 *   cc -O2 -std=c99 -DHEURISTIC_SOURCE='"../path/to/heuristic.c"' \
 *      tools/check_heuristic.c -o check_heuristic
 *   ./check_heuristic
 *
 * Exit status is 0 only if H1 and H2 both pass.
 */
#include "oracle.h"

#ifndef HEURISTIC_SOURCE
#error "define HEURISTIC_SOURCE, e.g. -DHEURISTIC_SOURCE='\"../heuristic.c\"'"
#endif
#include HEURISTIC_SOURCE

int main(void)
{
    uint8_t *dist = exact_distances();
    if (!dist) {
        fputs("could not compute exact distances\n", stderr);
        return 1;
    }

    heuristic_init();
    heuristic_tables();

    uint32_t count[12] = {0}, exact[12] = {0}, violations = 0;
    uint64_t sum[12] = {0};
    uint8_t lo[12], hi[12] = {0};
    uint32_t spread[12][12] = {{0}};
    memset(lo, UNSET, sizeof lo);

    for (uint32_t rank = 0; rank < STATES; ++rank) {
        state_t state;
        unrank_state(rank, &state);
        uint8_t h = heuristic(&state, rank), d = dist[rank];
        if (h > d) {
            if (violations < 10) {
                printf("H1 violation: ");
                print_state(&state);
                printf("  h = %u > d = %u\n", h, d);
            }
            ++violations;
            continue;
        }
        ++count[d];
        sum[d] += h;
        exact[d] += h == d;
        spread[d][h]++;
        if (h < lo[d])
            lo[d] = h;
        if (h > hi[d])
            hi[d] = h;
    }
    printf("H1 h(s) <= d(s) over %u states: %u violations  %s\n\n", STATES,
           violations, violations ? "FAIL" : "PASS");

    puts(" d    states   min h   mean h   max h   h == d");
    for (uint8_t d = 0; d < 12; ++d)
        if (count[d])
            printf("%2u  %8u   %5u   %6.3f   %5u   %6.2f%%\n", d, count[d],
                   lo[d], (double) sum[d] / count[d], hi[d],
                   100.0 * exact[d] / count[d]);

    puts("\nh over the distance-11 states:");
    for (uint8_t h = 0; h < 12; ++h)
        if (spread[11][h])
            printf("  h = %2u: %u\n", h, spread[11][h]);

    free(dist);
    return violations || h2_failed;
}
