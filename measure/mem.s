.equ SIZE, 0x10

li t0,  0x20000000
li t1, SIZE

add t2, t0,t1
li t3, 4
loop:
    sw t3, 0(t0)
    addi t0, t0, 4
    bne t0,t2, loop
li a7, 10
ecall    

    
    
    

