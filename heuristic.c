static uint16_t perm_next[3][PERMUTATIONS];   /* 轉移表：p 轉一次之後的編號 */
static uint16_t orient_next[3][ORIENTATIONS]; /* 轉移表：o 轉一次之後的編號 */
static uint8_t dist_p[PERMUTATIONS];          /* 只看 p 的距離 */
static uint8_t dist_o[ORIENTATIONS];          /* 只看 o 的距離 */

static void heuristic_init(void)
{
    state_t state;

    for (uint16_t rank = 0; rank < PERMUTATIONS; ++rank) {
        unrank_state((uint32_t) rank * ORIENTATIONS, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            perm_next[face][rank] =
                (uint16_t) (rank_state(&next) / ORIENTATIONS);
        }
    }
    for (uint16_t rank = 0; rank < ORIENTATIONS; ++rank) {
        unrank_state(rank, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            orient_next[face][rank] =
                (uint16_t) (rank_state(&next) % ORIENTATIONS);
        }
    }

    /* TODO 2：在 5,040 個 p 編號上做 BFS，填 dist_p
     *   - 先把 dist_p 全部設成 255（還沒走到）
     *   - 起點：p 編號 0（解開），距離 0
     *   - 用一個佇列（static 陣列就好，最多 5,040 格）
     *   - 每取出一個 p，對 3 個面各轉 1、2、3 次（用 perm_next 連查），
     *     如果鄰居還是 255，就設成「目前距離 + 1」並放進佇列
     *   參考 solver.c:219-244，結構幾乎一樣 */
    memset(dist_p, UINT8_MAX, sizeof dist_p);
    dist_p[0] = 0;
    static uint16_t queue_p[PERMUTATIONS];
    queue_p[0] = 0;
    uint16_t head_p = 0, tail_p = 1;

    while (head_p < tail_p) {
        uint16_t here = queue_p[head_p++];
        uint8_t distance = (uint8_t) (dist_p[here] + 1U);

        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t next = here;
            for (uint8_t turn = 0; turn < 3; ++turn) {
                next = perm_next[face][next];
                if (dist_p[next] == UINT8_MAX) {
                    dist_p[next] = distance;
                    queue_p[tail_p++] = next;
                }
            }
        }
    }

    /* TODO 3：在 729 個 o 編號上做同樣的 BFS，填 dist_o */
    memset(dist_o, UINT8_MAX, sizeof dist_o);
    dist_o[0] = 0;
    static uint16_t queue_o[ORIENTATIONS];
    queue_o[0] = 0;
    uint16_t head_o = 0, tail_o = 1;

    while (head_o < tail_o) {
        uint16_t here = queue_o[head_o++];
        uint8_t distance = (uint8_t) (dist_o[here] + 1U);

        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t next = here;
            for (uint8_t turn = 0; turn < 3; ++turn) {
                next = orient_next[face][next];
                if (dist_o[next] == UINT8_MAX) {
                    dist_o[next] = distance;
                    queue_o[tail_o++] = next;
                }
            }
        }
    }
    
}

static void heuristic_tables(void)
{
    /* TODO 4：對兩張距離表各呼叫一次 check_table
     *   check_table("名字", 表, 大小, 解開那格的 index); */
    check_table("dist_p", dist_p, PERMUTATIONS, 0);
    check_table("dist_o", dist_o, ORIENTATIONS, 0);
}

static uint8_t heuristic(const state_t *state, uint32_t rank)
{
    (void) state;
    /* TODO 5：
     *   - 從 rank 拆出 p 編號和 o 編號（rank = p × 729 + o）
     *   - 各查一次表
     *   - 回傳兩個裡面比較大的 */
    uint16_t p = (uint16_t) (rank / ORIENTATIONS);
    uint16_t o = (uint16_t) (rank % ORIENTATIONS);
    uint8_t h_p = dist_p[p];
    uint8_t h_o = dist_o[o];
    return h_p > h_o ? h_p : h_o;
}
