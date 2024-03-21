#ifndef MATRIX_KEYPAD_H
#define MATRIX_KEYPAD_H

#include <stdbool.h>

#define ROW_NUM 4
#define COL_NUM 4

extern volatile bool key_states[ROW_NUM][COL_NUM];
void matrix_keypad_init();
void read_matrix_keypad();

#endif /* MATRIX_KEYPAD_H */
