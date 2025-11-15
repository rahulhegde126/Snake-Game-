#include "ripes_system.h"
#include <stdio.h>
#include <stdlib.h>   // for rand()

// Size and color definitions
#define LED_MATRIX_WIDTH    35   // Width of main LED matrix (columns)
#define LED_MATRIX_HEIGHT   25   // Height of main LED matrix (rows)
#define LED_MATRIX_WIDTH_2  7    // Width of score LED matrix
#define LED_MATRIX_HEIGHT_2 5    // Height of score LED matrix

#define SNAKE_COLOR         0xFF0000  // Snake color (red)
#define APPLE_COLOR         0x90EE90  // Regular apple color (light green)
#define PURPLE_APPLE_COLOR  0x800080  // Purple (hazard) apple color
#define SCORE_COLOR         0xFFFFFF  // Score color (white)

#define MAX_SNAKE_LENGTH    100       // Maximum snake length (segments)
#define SNAKE_INIT_X        (LED_MATRIX_WIDTH / 2)   // Initial snake X (center)
#define SNAKE_INIT_Y        (LED_MATRIX_HEIGHT / 2)  // Initial snake Y (center)

#define DELAY               2500      // Delay count for game speed (higher = slower)

// Hardware-mapped IO registers
volatile unsigned int *d_pad_up    = (volatile unsigned int *)D_PAD_0_UP;
volatile unsigned int *d_pad_down  = (volatile unsigned int *)D_PAD_0_DOWN;
volatile unsigned int *d_pad_left  = (volatile unsigned int *)D_PAD_0_LEFT;
volatile unsigned int *d_pad_right = (volatile unsigned int *)D_PAD_0_RIGHT;

volatile unsigned int *led_matrix   = (volatile unsigned int *)LED_MATRIX_0_BASE;  // Main LED matrix
volatile unsigned int *led_matrix_2 = (volatile unsigned int *)LED_MATRIX_1_BASE;  // Score LED matrix

// Score counter
int contador = 0;  // Player score (increases when snake eats a normal apple)

// Snake movement directions
typedef enum {
   UP,
   DOWN,
   LEFT,
   RIGHT
} Direction;

// Apple position
typedef struct {
   int x;
   int y;
} Apple;

// Snake head and segment position
typedef struct {
   int x;
   int y;
   Direction dir;
} Position;

// Snake state
Position snake[MAX_SNAKE_LENGTH];
int snakeLength = 1;

Apple apple, purpleApple;

unsigned int randomSeed = 0;  // Reserved for RNG seed if needed
int game_over = 0;            // 0 = running, non-zero = game over

// 3x5 font size for score LED matrix
#define NUM_WIDTH  3
#define NUM_HEIGHT 5

// 3x5 font patterns for digits 0–9 (used for score display on second matrix)
unsigned int numbers[10][NUM_HEIGHT] = {
    {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
    {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
    {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
    {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
    {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
    {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
    {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
    {0b111, 0b001, 0b001, 0b001, 0b001}, // 7
    {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
    {0b111, 0b101, 0b111, 0b001, 0b111}  // 9
};

// Clear main LED matrix
void resetLEDs() {
   for (int y = 0; y < LED_MATRIX_HEIGHT; y++) {
       for (int x = 0; x < LED_MATRIX_WIDTH; x++) {
           led_matrix[y * LED_MATRIX_WIDTH + x] = 0x0;
       }
   }
}

// Clear score LED matrix
void resetLEDs_2() {
   for (int y = 0; y < LED_MATRIX_HEIGHT_2; y++) {
       for (int x = 0; x < LED_MATRIX_WIDTH_2; x++) {
           led_matrix_2[y * LED_MATRIX_WIDTH_2 + x] = 0x0;
       }
   }
}

// Initialize snake at center and clear both LED matrices
void setupSnake() {
   snake[0].x   = SNAKE_INIT_X;
   snake[0].y   = SNAKE_INIT_Y;
   snake[0].dir = RIGHT;

   resetLEDs();
   resetLEDs_2();
}

// Draw snake on main LED matrix (each segment as a 2x2 block)
void renderSnake() {
   for (int i = 0; i < snakeLength; i++) {
       for (int dx = 0; dx < 2; dx++) {
           for (int dy = 0; dy < 2; dy++) {
               int nx = snake[i].x + dx;
               int ny = snake[i].y + dy;
               if (nx >= 0 && nx < LED_MATRIX_WIDTH &&
                   ny >= 0 && ny < LED_MATRIX_HEIGHT) {
                   led_matrix[ny * LED_MATRIX_WIDTH + nx] = SNAKE_COLOR;
               }
           }
       }
   }
}

// Generate a new regular apple at a random free position
void createApple() {
   do {
       apple.x = rand() % (LED_MATRIX_WIDTH  - 2) + 1;  // avoid borders
       apple.y = rand() % (LED_MATRIX_HEIGHT - 2) + 1;
   } while (led_matrix[apple.y * LED_MATRIX_WIDTH + apple.x] == SNAKE_COLOR);
}

// Draw regular apple (2x2 block)
void renderApple() {
   for (int dx = 0; dx < 2; dx++) {
       for (int dy = 0; dy < 2; dy++) {
           int nx = apple.x + dx;
           int ny = apple.y + dy;
           led_matrix[ny * LED_MATRIX_WIDTH + nx] = APPLE_COLOR;
       }
   }
}

// Generate a new purple apple (hazard) at a random free position
void createPurpleApple() {
   do {
       purpleApple.x = rand() % (LED_MATRIX_WIDTH  - 2) + 1;
       purpleApple.y = rand() % (LED_MATRIX_HEIGHT - 2) + 1;
   } while (led_matrix[purpleApple.y * LED_MATRIX_WIDTH + purpleApple.x] == SNAKE_COLOR ||
            (purpleApple.x == apple.x && purpleApple.y == apple.y));
}

// Draw purple apple (2x2 block)
void renderPurpleApple() {
   for (int dx = 0; dx < 2; dx++) {
       for (int dy = 0; dy < 2; dy++) {
           int nx = purpleApple.x + dx;
           int ny = purpleApple.y + dy;
           led_matrix[ny * LED_MATRIX_WIDTH + nx] = PURPLE_APPLE_COLOR;
       }
   }
}

// Display a 0–99 score on the 7x5 score LED matrix
void displayNumber(int num) {
    resetLEDs_2();

    if (num < 0)  num = 0;
    if (num > 99) num = 99;

    int digit1 = num / 10;
    int digit2 = num % 10;
    int offset = 0;

    // Draw first digit (if score >= 10)
    if (num >= 10) {
        for (int y = 0; y < NUM_HEIGHT; y++) {
            for (int x = 0; x < NUM_WIDTH; x++) {
                if (numbers[digit1][y] & (1 << (NUM_WIDTH - 1 - x))) {
                    led_matrix_2[y * LED_MATRIX_WIDTH_2 + offset + x] = SCORE_COLOR;
                }
            }
        }
        offset += NUM_WIDTH + 1;  // space between digits
    }

    // Draw second digit (or single digit if <10)
    for (int y = 0; y < NUM_HEIGHT; y++) {
        for (int x = 0; x < NUM_WIDTH; x++) {
            if (numbers[digit2][y] & (1 << (NUM_WIDTH - 1 - x))) {
                led_matrix_2[y * LED_MATRIX_WIDTH_2 + offset + x] = SCORE_COLOR;
            }
        }
    }
}

// Extend snake by 2 segments and update score
void extendSnake() {
    if (snakeLength + 2 <= MAX_SNAKE_LENGTH) {
        for (int j = 0; j < 2; j++) {
            Position newHead = snake[0];

            switch (snake[0].dir) {
                case UP:    newHead.y--; break;
                case DOWN:  newHead.y++; break;
                case LEFT:  newHead.x--; break;
                case RIGHT: newHead.x++; break;
            }

            // Shift body one position back
            for (int i = snakeLength; i > 0; i--) {
                snake[i] = snake[i - 1];
            }

            snake[0] = newHead;
            snakeLength++;
        }

        contador++;
        displayNumber(contador);  // show updated score
    }
}

// Read D-pad and update direction (no movement here)
void readDirectionInput() {
    if (*d_pad_up && snake[0].dir != DOWN) {
        snake[0].dir = UP;
    } else if (*d_pad_down && snake[0].dir != UP) {
        snake[0].dir = DOWN;
    } else if (*d_pad_left && snake[0].dir != RIGHT) {
        snake[0].dir = LEFT;
    } else if (*d_pad_right && snake[0].dir != LEFT) {
        snake[0].dir = RIGHT;
    }
}

// Move snake one step based on current direction
void navigateSnake() {
    Position nextPosition = snake[0];

    // Toroidal movement (wrap around edges)
    switch (nextPosition.dir) {
        case UP:
            nextPosition.y = (nextPosition.y - 1 + LED_MATRIX_HEIGHT) % LED_MATRIX_HEIGHT;
            break;
        case DOWN:
            nextPosition.y = (nextPosition.y + 1) % LED_MATRIX_HEIGHT;
            break;
        case LEFT:
            nextPosition.x = (nextPosition.x - 1 + LED_MATRIX_WIDTH) % LED_MATRIX_WIDTH;
            break;
        case RIGHT:
            nextPosition.x = (nextPosition.x + 1) % LED_MATRIX_WIDTH;
            break;
    }
    
    // Hit purple apple => game over
    if (nextPosition.x == purpleApple.x && nextPosition.y == purpleApple.y) {
        game_over = 1;
        return;
    }

    // Hit regular apple => grow and respawn apples
    if (nextPosition.x == apple.x && nextPosition.y == apple.y) {
        extendSnake();
        createApple();
        createPurpleApple();
    } else {
        // Move body forward
        for (int i = snakeLength - 1; i > 0; i--) {
            snake[i] = snake[i - 1];
        }
        snake[0] = nextPosition;
    }
}

// Detect collision of head with body
int detectCollision() {
    for (int i = 1; i < snakeLength; i++) {
        if (snake[0].x == snake[i].x &&
            snake[0].y == snake[i].y) {
            return 1;
        }
    }
    return 0;
}

// Delay with continuous input polling for better responsiveness
void pause() {
    for (volatile unsigned int i = 0; i < DELAY; ++i) {
        readDirectionInput();  // keep reading D-pad during "wait"
    }
}

// Main function (game loop)
int main() {
    setupSnake();
    createApple();
    createPurpleApple();

    while (!game_over) {
        // 1. During this time you can change direction
        pause();

        // 2. Move snake once per frame
        navigateSnake();

        // 3. Check self-collision
        if (detectCollision()) {
            game_over = 1;
            break;
        }

        // 4. Redraw frame
        resetLEDs();
        renderSnake();
        renderApple();
        renderPurpleApple();
    }

    return 0;
}
