#include "keypad.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const gpio_num_t row_pins[ROW_NUM] = {GPIO_NUM_33, GPIO_NUM_32, GPIO_NUM_27, GPIO_NUM_13};
static const gpio_num_t col_pins[COL_NUM] = {GPIO_NUM_23, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_4};

volatile bool key_states[ROW_NUM][COL_NUM];

// 初始化矩阵键盘
void matrix_keypad_init() {
    // 配置行GPIO为输入模式
    for (uint8_t i = 0; i < ROW_NUM; i++) {
        gpio_set_direction(row_pins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(row_pins[i], GPIO_PULLUP_ONLY);
    }

    for (uint8_t i = 0; i < COL_NUM; i++) {
        gpio_set_direction(col_pins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(col_pins[i], GPIO_PULLUP_ONLY);
    }
}

void read_matrix_keypad() {
    for (uint8_t i = 0; i < ROW_NUM; i++) {
        gpio_set_direction(col_pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(col_pins[i], 0);
        for (uint8_t j = 0; j < COL_NUM; j++) {
            if (gpio_get_level(row_pins[j]) == 0) {
                key_states[i][j] = true;
            } else {
                key_states[i][j] = false;
            }
        }
        gpio_set_direction(col_pins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(col_pins[i], GPIO_PULLUP_ONLY);
    }
}
