.data

arr_p:  .zero 24      # .half 陣列
arr_o:  .zero 24      # .half 陣列
arr_face:  .zero 12      # .byte 陣列
arr_turn:  .zero 12      # .byte 陣列
input:  .string "25314672313211"

.text
    la    t0, input
    addi  t4, t0, 7        # t4 = &input[7]（固定不變，搬到最外面）
    li    t6, 7
    li    a2, 0            # a2 = i
    li    a1, 0            # a1 = p
loop_i:
    beq a2,t6,done_i   # ① 如果 i == 7（a2 == t6），跳到 done_i

    # ===== 2a：算 smaller → a0 =====
    add   t1, t0, a2       # t1 = &input[i]
    lbu   t2, 0(t1)        # t2 = input[i]
    li    a0, 0            # a0 = smaller
    addi  t3, t1, 1        # t3 = &input[j]，j 從 i+1 開始
loop_j:
    beq t3, t4, done_j   # ① 如果 t3 == t4，跳到 done
    lbu   t5, 0(t3)        # t5 = input[j]
    bgeu t5,t2,next   # ② 如果 t5 >= t2（不是比較小），跳到 next
    addi a0,a0,1   # ③ a0 = a0 + 1
next:
    addi  t3, t3, 1
    j     loop_j
done_j:

    # ===== a1 = a1 × (7 − i) =====
    sub   a3, t6, a2       # a3 = 7 − i
    # ② 照上面的乘法例子，算出 a1 × a3，結果放回 a1
    #    提示：需要一個暫存器放「結果」，乘完再搬回 a1
    li a4, 0
    li a5,0 
mul_loop:
    beq a3,a4, mul_done
    add a5,a1,a5
    addi a4,a4,1
    j mul_loop
mul_done:
    mv a1,a5
    # ===== a1 = a1 + smaller =====
    add a1,a1,a0   # ③
    

    addi  a2, a2, 1        # i++
    j     loop_i
done_i:
    mv s0,a1
    la    t0, input
    addi  t0, t0, 7     # ① 跳到方向數字的開頭
    li    t1, 6         # ② 要讀幾個
    li    a1, 0
loop_o:
    lbu   t2, 0(t0)
    addi  t2, t2, -49
    slli t3, a1 ,1   # ③ t3 = a1 << 1
    add a1, t3, a1    # ④ a1 = t3 + a1     （現在 a1 = 原本的 o × 3）
    add a1, a1, t2    # ⑤ a1 = a1 + t2
    addi  t0, t0, 1
    addi  t1, t1, -1
    bnez  t1, loop_o
    mv s1,a1

    # ① 讀 dist_p[p]，放到 t1
    la t0, dist_p
    add t0,t0,s0
    lbu t1,0(t0)
    # ② 讀 dist_o[o]，放到 t2
    la t0, dist_o
    add t0,t0,s1
    lbu t2,0(t0)
    # ③ s2 = max(t1, t2)
    mv s2,t2
    bgeu t2,t1,find_h
    mv s2,t1
find_h:
    bnez s0, start_check
    bnez s1, start_check
    li a0, 0
    j end
start_check:
    la    t0, arr_p
    sh    s0, 0(t0)

    la t1,arr_o
    sh s1, 0(t1)
    
    
    li a5, 3 #常數
    li t4, 1
    li t5,10080
    li t6,1458
    
    
    mv s8, s2 #bound
    li a1, 0 #counter for test hueristic
    

restart:
    li s3,0
    li s7,0 #g
    la t1, perm_next
    la t2, orient_next
    li a7, 3
loop_face:
    beq s3,a5,go_up
    beq s3,a7,next_face
    li s4,-1
    mv s5,s0
    mv s6,s1
    

loop_turn:
    addi s4,s4,1
    beq s4,a5,next_face 

    slli  t0, s5, 1
    add t3 , t1, t0
    lhu s5 , 0(t3)
    
    slli  t0, s6, 1
    add t3 , t2, t0
    lhu s6, 0(t3)

    la t3,dist_p
    la t4,dist_o

    add t3,t3,s5
    add t4,t4,s6

    lbu t0,0(t3)
    lbu t3,0(t4)
    mv a4,t0
    bgeu t0,t3,find_h_prime
    mv a4,t3

find_h_prime:
    add a4,a4,s7
    addi a4,a4,1
    bltu s8,a4,loop_turn

    bnez s6, go_down
    bnez s5, go_down
    slli t0,s3,1
    add t0,t0,s3
    add a2,t0,s4 #answer

    la t3, arr_face
    lbu t0,0(t3)
    slli t1,t0,1
    add t1,t1,t0

    la t3, arr_turn
    lbu t0,0(t3)
    add a1,t1,t0
    addi a0,s7,1


    j end

go_down:
    la t0, arr_face
    add t0, t0, s7
    sb s3,0(t0)
    la t0, arr_turn
    add t0, t0, s7
    sb s4,0(t0)

    la t0, arr_p
    slli t3, s7, 1
    add t0 ,t0,t3
    sh s5,2(t0)
    la t0, arr_o
    add t0 ,t0,t3
    sh s6,2(t0)

    mv a7, s3
    addi s7,s7,1

    mv s0,s5
    mv s1,s6

    la t1, perm_next
    la t2, orient_next
    li s3,0
    j loop_face
go_up:
    beqz s7, add_bound
    addi s7, s7, -1

    la t0,arr_face
    add t0,t0,s7
    lbu s3, 0(t0)
    la t0,arr_turn
    add t0,t0,s7
    lbu s4, 0(t0)

    slli t1,s7,1
    la t0,arr_p
    add t0,t0,t1
    lhu s0, 0(t0)
    lhu s5, 2(t0)
    la t0,arr_o
    add t0,t0,t1
    lhu s1, 0(t0)
    lhu s6, 2(t0)

    la t1,perm_next
    la t2,orient_next

    beqz s3, row_done
    li t4,1

    beq t4,s3,row_add1
    add t1,t1,t5
    add t2,t2,t6
row_add1:
    add t1,t1,t5
    add t2  ,t2,t6
row_done:
    li a7,3
    beqz s7,lf_done
    la t0,arr_face
    add t0,t0,s7
    lbu a7,-1(t0)

lf_done:
    j loop_turn

add_bound:
    addi s8,s8,1
    j restart

next_face:
    addi s3,s3,1
    add t1,t1,t5
    add t2,t2,t6
    j loop_face 
end:
    li a7, 10
    ecall