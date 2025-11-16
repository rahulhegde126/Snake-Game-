    .equ D_PAD_0_BASE,        0xf0000000
    .equ D_PAD_0_UP,          0xf0000000
    .equ D_PAD_0_DOWN,        0xf0000004
    .equ D_PAD_0_LEFT,        0xf0000008
    .equ D_PAD_0_RIGHT,       0xf000000c

    .equ LED_MATRIX_0_BASE,   0xf0000010
    .equ LED_MATRIX_0_WIDTH,  0x23        # 35
    .equ LED_MATRIX_0_HEIGHT, 0x19        # 25

    .equ LED_MATRIX_1_BASE,   0xf0000dbc  # 7x5 score matrix (not used here)
    .equ LED_MATRIX_1_WIDTH,  0x7
    .equ LED_MATRIX_1_HEIGHT, 0x5

    .equ COLOR_SNAKE,  0x00FF0000   # red
    .equ COLOR_APPLE,  0x0090EE90   # light green
    .equ COLOR_BLACK,  0x00000000

    .equ DIR_UP,    0
    .equ DIR_DOWN,  1
    .equ DIR_LEFT,  2
    .equ DIR_RIGHT, 3

    .equ DELAY_COUNT, 2000         # change for speed

    .data

snake_x:      .word 17        # initial x
snake_y:      .word 12        # initial y
snake_dir:    .word DIR_RIGHT # initial direction

apple_x:      .word 10
apple_y:      .word 5

score:        .word 0
game_over:    .word 0

rng_state:    .word 1         # simple pseudo-random state

    .text
    .globl _start

_start:
    # initialize game
    jal ra, init_game

main_loop:
    # if game_over != 0 -> stop
    la   t0, game_over
    lw   t1, 0(t0)
    bnez t1, end_program

    # delay + read input many times
    jal  ra, delay_and_read_input

    # move snake
    jal  ra, move_snake

    # check if snake ate apple
    jal  ra, check_eat_apple

    # draw frame
    jal  ra, draw_frame

    j    main_loop

end_program:
end_loop:
    j end_loop

################################################
# init_game
################################################
init_game:
    # score = 0
    la   t0, score
    sw   x0, 0(t0)

    # game_over = 0
    la   t0, game_over
    sw   x0, 0(t0)

    # snake_x = WIDTH / 2
    li   t1, LED_MATRIX_0_WIDTH
    srai t1, t1, 1
    la   t0, snake_x
    sw   t1, 0(t0)

    # snake_y = HEIGHT / 2
    li   t1, LED_MATRIX_0_HEIGHT
    srai t1, t1, 1
    la   t0, snake_y
    sw   t1, 0(t0)

    # snake_dir = RIGHT
    la   t0, snake_dir
    li   t1, DIR_RIGHT
    sw   t1, 0(t0)

    # rng_state = 1
    la   t0, rng_state
    li   t1, 1
    sw   t1, 0(t0)

    # spawn apple
    jal  ra, spawn_apple

    # clear screen
    jal  ra, clear_leds

    ret

################################################
# delay_and_read_input
################################################
delay_and_read_input:
    li   t0, DELAY_COUNT

delay_loop:
    beqz t0, delay_done

    # read D-pad during delay
    jal  ra, read_input_direction

    addi t0, t0, -1
    j    delay_loop

delay_done:
    ret

################################################
# read_input_direction
################################################
read_input_direction:
    # t1 = current dir
    la   t0, snake_dir
    lw   t1, 0(t0)

    # UP
    li   t2, D_PAD_0_UP
    lw   t3, 0(t2)
    bnez t3, dir_up_check

    # DOWN
    li   t2, D_PAD_0_DOWN
    lw   t3, 0(t2)
    bnez t3, dir_down_check

    # LEFT
    li   t2, D_PAD_0_LEFT
    lw   t3, 0(t2)
    bnez t3, dir_left_check

    # RIGHT
    li   t2, D_PAD_0_RIGHT
    lw   t3, 0(t2)
    bnez t3, dir_right_check

    ret

dir_up_check:
    li   t4, DIR_DOWN
    beq  t1, t4, ret_no_change
    li   t1, DIR_UP
    j    store_new_dir

dir_down_check:
    li   t4, DIR_UP
    beq  t1, t4, ret_no_change
    li   t1, DIR_DOWN
    j    store_new_dir

dir_left_check:
    li   t4, DIR_RIGHT
    beq  t1, t4, ret_no_change
    li   t1, DIR_LEFT
    j    store_new_dir

dir_right_check:
    li   t4, DIR_LEFT
    beq  t1, t4, ret_no_change
    li   t1, DIR_RIGHT
    j    store_new_dir

store_new_dir:
    la   t0, snake_dir
    sw   t1, 0(t0)
ret_no_change:
    ret

################################################
# move_snake
################################################
move_snake:
    # load snake_x, snake_y, snake_dir
    la   t0, snake_x
    lw   t1, 0(t0)      # t1 = x
    la   t0, snake_y
    lw   t2, 0(t0)      # t2 = y
    la   t0, snake_dir
    lw   t3, 0(t0)      # t3 = dir

    li   t4, DIR_UP
    beq  t3, t4, move_up
    li   t4, DIR_DOWN
    beq  t3, t4, move_down
    li   t4, DIR_LEFT
    beq  t3, t4, move_left
    li   t4, DIR_RIGHT
    beq  t3, t4, move_right
    j    move_done

move_up:
    addi t2, t2, -1
    j    wrap_y
move_down:
    addi t2, t2, 1
    j    wrap_y
move_left:
    addi t1, t1, -1
    j    wrap_x
move_right:
    addi t1, t1, 1
    j    wrap_x

wrap_x:
    # if x < 0: x += WIDTH
    bgez t1, check_x_high
    li   t5, LED_MATRIX_0_WIDTH
    add  t1, t1, t5
check_x_high:
    li   t5, LED_MATRIX_0_WIDTH
    blt  t1, t5, wrap_y
    sub  t1, t1, t5

wrap_y:
    # if y < 0: y += HEIGHT
    bgez t2, check_y_high
    li   t5, LED_MATRIX_0_HEIGHT
    add  t2, t2, t5
check_y_high:
    li   t5, LED_MATRIX_0_HEIGHT
    blt  t2, t5, move_done
    sub  t2, t2, t5

move_done:
    la   t0, snake_x
    sw   t1, 0(t0)
    la   t0, snake_y
    sw   t2, 0(t0)
    ret

################################################
# check_eat_apple
################################################
check_eat_apple:
    # snake_x, snake_y
    la   t0, snake_x
    lw   t1, 0(t0)
    la   t0, snake_y
    lw   t2, 0(t0)

    # apple_x, apple_y
    la   t0, apple_x
    lw   t3, 0(t0)
    la   t0, apple_y
    lw   t4, 0(t0)

    bne  t1, t3, not_eaten
    bne  t2, t4, not_eaten

    # score++
    la   t0, score
    lw   t5, 0(t0)
    addi t5, t5, 1
    sw   t5, 0(t0)

    # new apple
    jal  ra, spawn_apple

not_eaten:
    ret

################################################
# spawn_apple  (simple LCG)
################################################
spawn_apple:
    la   t0, rng_state
    lw   t1, 0(t0)

    li   t2, 1103515245
    mul  t1, t1, t2
    li   t2, 12345
    add  t1, t1, t2
    sw   t1, 0(t0)

    # apple_x = rng_state % WIDTH
    li   t2, LED_MATRIX_0_WIDTH
    remu t3, t1, t2
    la   t0, apple_x
    sw   t3, 0(t0)

    # apple_y = (rng_state >> 5) % HEIGHT
    srli t1, t1, 5
    li   t2, LED_MATRIX_0_HEIGHT
    remu t3, t1, t2
    la   t0, apple_y
    sw   t3, 0(t0)

    ret

################################################
# draw_frame
################################################
draw_frame:
    jal  ra, clear_leds
    jal  ra, draw_apple
    jal  ra, draw_snake_head
    ret

################################################
# clear_leds
################################################
clear_leds:
    li   t0, LED_MATRIX_0_WIDTH
    li   t1, LED_MATRIX_0_HEIGHT
    mul  t2, t0, t1          # total pixels

    li   t3, LED_MATRIX_0_BASE
    li   t4, COLOR_BLACK

clear_loop:
    beqz t2, clear_done
    sw   t4, 0(t3)
    addi t3, t3, 4
    addi t2, t2, -1
    j    clear_loop

clear_done:
    ret

################################################
# draw_apple
################################################
draw_apple:
    la   t0, apple_x
    lw   t1, 0(t0)
    la   t0, apple_y
    lw   t2, 0(t0)

    li   t3, LED_MATRIX_0_WIDTH
    mul  t4, t2, t3
    add  t4, t4, t1          # index

    li   t5, LED_MATRIX_0_BASE
    slli t4, t4, 2           # *4
    add  t5, t5, t4

    li   t6, COLOR_APPLE
    sw   t6, 0(t5)
    ret

################################################
# draw_snake_head
################################################
draw_snake_head:
    la   t0, snake_x
    lw   t1, 0(t0)
    la   t0, snake_y
    lw   t2, 0(t0)

    li   t3, LED_MATRIX_0_WIDTH
    mul  t4, t2, t3
    add  t4, t4, t1          # index

    li   t5, LED_MATRIX_0_BASE
    slli t4, t4, 2
    add  t5, t5, t4

    li   t6, COLOR_SNAKE
    sw   t6, 0(t5)
    ret
