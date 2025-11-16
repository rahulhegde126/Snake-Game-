# Snake Game on Ripes (RISC-V + LED Matrix + D-Pad)

This repository contains a simple Snake game implemented for the **Ripes** RISC-V simulator.

The game uses:

- A **D-Pad** for input (UP, DOWN, LEFT, RIGHT).
- A **35×25 LED matrix** for rendering the snake + apples.
- A **7×5 LED matrix** for displaying the score (C version).

There are two implementations:

1. `snake.c` — C implementation using `ripes_system.h`
2. `snake.asm` — pure RISC-V RV32I assembly implementation

Both are designed to work with the same memory-mapped IO layout.

---

## IO Map (matches `ripes_io.h`)
<img width="2879" height="1704" alt="image" src="https://github.com/user-attachments/assets/977aae95-04c1-43a9-a726-4c12d6db5386" />
```c
// D-Pad
#define D_PAD_0_BASE        (0xf0000000)
#define D_PAD_0_UP          (0xf0000000)
#define D_PAD_0_DOWN        (0xf0000004)
#define D_PAD_0_LEFT        (0xf0000008)
#define D_PAD_0_RIGHT       (0xf000000c)

// Main LED Matrix (35 x 25)
#define LED_MATRIX_0_BASE   (0xf0000010)
#define LED_MATRIX_0_SIZE   (0xdac)
#define LED_MATRIX_0_WIDTH  (0x23)  // 35
#define LED_MATRIX_0_HEIGHT (0x19)  // 25

// Score LED Matrix (7 x 5)
#define LED_MATRIX_1_BASE   (0xf0000dbc)
#define LED_MATRIX_1_SIZE   (0x8c)
#define LED_MATRIX_1_WIDTH  (0x7)   // 7
#define LED_MATRIX_1_HEIGHT (0x5)   // 5  
