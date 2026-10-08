enum { NO_FACE = 3 };          /* 起點沒有「上一步」，用 3 表示 */

static uint8_t path[12];       /* path[g] = 第 g 步轉的 move 編號 */
static uint64_t *node_count;   /* 指向框架給的計數器 */

/* 從 (p, o) 開始，已經走了 g 步，看能不能在 bound 步內解開。
 * 找到回傳 1，找不到回傳 0。 */
static int dfs(uint16_t p, uint16_t o, uint8_t g, uint8_t bound,
               uint8_t last_face)
{
    /* TODO A：計數器 +1（這是一個節點） */
    *node_count += 1;

    /* TODO B：終點檢查
     *   p 和 o 都是 0 → 解開了，return 1 */
    if (p == 0 && o == 0) {
        return 1;
    }
    /* TODO C：試每個 move
     *   for face = 0..2：
     *       如果 face == last_face，跳過（同一面不連續轉）
     *       next_p = p, next_o = o
     *       for turn = 0..2：                    ← 和你 BFS 裡的寫法一樣
     *           next_p、next_o 各查一次轉移表     （連查，所以是 R、R2、R'）
     *           h = 兩張距離表查出來取 max
     *           如果 g + 1 + h > bound → continue（剪掉）
     *           path[g] = 這個 move 的編號（face × 3 + turn）
     *           如果 dfs(next_p, next_o, g + 1, bound, face) 回傳 1
     *               → return 1（一路往上傳）
     */
    for(uint8_t face = 0 ;face < 3;face++){
        if(face == last_face) continue;
        uint16_t next_p = p;
        uint16_t next_o = o;
        for(uint8_t turn = 0;turn<3;turn++){
            next_p = perm_next[face][next_p];
            next_o = orient_next[face][next_o];
            
            uint16_t h = (dist_p[next_p] > dist_o[next_o]) ? dist_p[next_p] : dist_o[next_o];

            if(g+1+h>bound) continue;

            path[g] = face * 3 + turn;
            if(dfs(next_p, next_o, g + 1, bound, face) == 1)
                return 1;
        }   
    }
    return 0;   /* 全部試完都沒找到 */
}

static void search_init(void)
{
    heuristic_init();    /* 建轉移表和距離表 */
}

static uint8_t search(const state_t *state, uint8_t moves[], uint64_t *nodes)
{
    /* TODO D：
     *   1. 從 state 算 rank，拆成 p 編號和 o 編號
     *   2. node_count = nodes
     *   3. bound = 起點的 h（查表取 max）
     *   4. 重複：
     *        如果 dfs(p, o, 0, bound, NO_FACE) 回傳 1：
     *            把 path[0] ~ path[bound - 1] 複製到 moves[]
     *            return bound
     *        否則 bound + 1
     */

     uint32_t rank = rank_state(state);
     uint16_t p = (uint16_t) (rank / ORIENTATIONS);
     uint16_t o = (uint16_t) (rank % ORIENTATIONS);

    node_count = nodes;
    uint8_t bound = (dist_p[p] > dist_o[o]) ? dist_p[p] : dist_o[o];

    while(1){
        if(dfs(p, o, 0, bound, NO_FACE)){
            for(uint8_t i = 0 ; i<bound;i++){
                moves[i] = path[i];
            }
            return bound;
        }else{
            bound += 1;
        }
    }
}