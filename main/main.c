#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
// #include <pwm_audio.h>
#include <dac_audio.h>
#include <driver/gpio.h>
#include <math.h>
#include <esp_log.h>
// #include "scurry.h"
#include "justice_96_remix.h"
// #include "EFXTEST.h"
#include "ssd1306.h"
// #include "font8x8_basic.h"
#include "vol_table.h"
#include <string.h>

#define BUFF_SIZE 2048
#define SMP_RATE 44100
#define SMP_BIT 8
int8_t buffer_ch[4][BUFF_SIZE];
int8_t buffer[BUFF_SIZE];
float time_step = 1.0 / SMP_RATE;

size_t wrin;
uint8_t stp;
bool display_stat = true;

const uint8_t zero[8192] = {0};

#define CHL_NUM 4
#define TRACKER_ROW 64
uint8_t NUM_PATTERNS;
#define BUFFER_PATTERNS 2
#define NUM_ROWS 64
#define NUM_CHANNELS 4
#define PATTERN_SIZE (NUM_ROWS * NUM_CHANNELS * 4)

uint8_t part_table[128];
uint8_t part_point = 2;
uint8_t tracker_point = 0;

#define BASE_FREQ 8267
bool dispRedy = false;

float patch_table[16] = {3546836, 3572516, 3598624, 3624304, 3650840, 3676948, 3703912, 3730876,
                         3347816, 3372212, 3396180, 3421004, 3445828, 3471080, 3495904, 3521584};

inline void hexToDecimal(uint8_t num, uint8_t *tens, uint8_t *ones) {
    *tens = (num >> 4) & 0x0F;
    *ones = num & 0x0F;
}
inline uint8_t hexToDecimalTens(uint8_t num) {
    return (num >> 4) & 0x0F;
}
inline uint8_t hexToDecimalOnes(uint8_t num) {
    return num & 0x0F;
}
inline float freq_up(float base_freq, uint8_t n) {
    return base_freq * powf(2.0f, (n / 12.0f));
}
inline int clamp(int value, int min, int max) {
    if (value < min)
        return min;
    else if (value > max)
        return max;
    else
        return value;
}
float lmt = 0;
uint16_t lmtP = 0;
float comper = 1;

void limit(float a) {
    lmt += 2 + (a > 0 ? a : -a) * 14 / 512.0f;
    lmtP++;
    if (lmtP >= 4096) {
        comper = lmt / 4096;
        // printf("COMPER %f\n", comper);
        lmt = 0;
        lmtP = 0;
    }
}
// **************************INIT*END****************************
uint16_t WAVE_RATE = 8287;
// NOTE COMP START ----------------------------------------------
inline float midi_note_frequency(int note) {
    return 440.0 * powf(2.0, (note - 69) / 12.0);
}

inline float samp_frequency(int note) {
    return WAVE_RATE * powf(2.0, (note - 69) / 12.0);
}
// NOTE COMP END ------------------------------------------------

char ten[28];
int8_t vol[CHL_NUM] = {0};
int16_t period[4] = {1};
float data_index[CHL_NUM] = {0};
uint16_t data_index_int[CHL_NUM] = {0};
uint8_t smp_num[CHL_NUM] = {0};

int16_t temp;
uint16_t wave_info[33][5];
uint32_t wav_ofst[32];

// UI START -----------------------------------------------------
void display() {
    SSD1306_t dev;
    i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
    ssd1306_init(&dev, 128, 64);
    ssd1306_clear_screen(&dev, false);
    ssd1306_contrast(&dev, 0xff);
    // bool bs = 0;
    ssd1306_display_text(&dev, 2, "LOADING DATA....", 16, false);
    vTaskDelay(2);
    for (;;) {
        vTaskDelay(2);
        if (dispRedy) {
            break;
        }
    }
    for (;;) {
        uint8_t x;
        uint8_t volTemp;
        uint8_t addr[4];
        for (uint8_t contr = 0; contr < 4; contr++) {
            ssd1306_set_buffer(&dev, zero);
            sprintf(ten, "  %2d %2d>%2d %.3f", tracker_point, part_point, part_table[part_point], comper);
            addr[0] = data_index[0] * (32.0f / wave_info[smp_num[0]][0]);
            addr[1] = data_index[1] * (32.0f / wave_info[smp_num[1]][0]);
            addr[2] = data_index[2] * (32.0f / wave_info[smp_num[2]][0]);
            addr[3] = data_index[3] * (32.0f / wave_info[smp_num[3]][0]);
            // ssd1306_display_text(&dev, 7, tet, 16, false);
            ssd1306_display_text(&dev, 0, "CH1 CH2 CH3 CH4", 16, false);
            ssd1306_display_text(&dev, 6, ten, 16, false);
            for (x = 0; x < 32; x++) {
                _ssd1306_pixel(&dev, x, ((buffer_ch[0][(x + (contr * 128)) * 2]) / 4) + 32, false);
                _ssd1306_pixel(&dev, x, (uint8_t)(period[0] * (64.0f / 743.0f))%64, false);
                // printf("DISPLAY %d\n", roundf(period[0] * (64.0f / 743.0f)));
                volTemp = (vol[0]/2) % 64;
                _ssd1306_line(&dev, addr[0], 8, addr[0], 47, false);
                for (uint8_t i = 58; i < 64; i++) {
                    _ssd1306_line(&dev, 0, i, volTemp, i, false);
                }
            }
            for (x = 32; x < 64; x++) {
                _ssd1306_pixel(&dev, x, ((buffer_ch[1][((x-32) + (contr * 128)) * 2]) / 4) + 32, false);
                _ssd1306_pixel(&dev, x, (uint8_t)(period[1] * (64.0f / 743.0f))%64, false);
                volTemp = vol[1]/2;
                _ssd1306_line(&dev, addr[1]+32, 8, addr[1]+32, 47, false);
                for (uint8_t i = 58; i < 64; i++) {
                    _ssd1306_line(&dev, 31, i, volTemp+31, i, false);
                }
            }
            for (x = 64; x < 96; x++) {
                _ssd1306_pixel(&dev, x, ((buffer_ch[2][((x-64) + (contr * 128)) * 2]) / 4) + 32, false);
                _ssd1306_pixel(&dev, x, (uint8_t)(period[2] * (64.0f / 743.0f))%64, false);
                volTemp = vol[2]/2;
                _ssd1306_line(&dev, addr[2]+64, 8, addr[2]+64, 47, false);
                for (uint8_t i = 58; i < 64; i++) {
                    _ssd1306_line(&dev, 63, i, volTemp+63, i, false);
                }
            }
            for (x = 96; x < 128; x++) {
                _ssd1306_pixel(&dev, x, ((buffer_ch[3][((x-96) + (contr * 128)) * 2]) / 4) + 32, false);
                _ssd1306_pixel(&dev, x, (uint8_t)(period[3] * (64.0f / 743.0f))%64, false);
                volTemp = vol[3]/2;
                _ssd1306_line(&dev, addr[3]+96, 8, addr[3]+96, 47, false);
                for (uint8_t i = 58; i < 64; i++) {
                    _ssd1306_line(&dev, 95, i, volTemp+95, i, false);
                }
            }
            ssd1306_show_buffer(&dev);
            // vTaskDelay(1);
        }
    }
}
// UI END -------------------------------------------------------

void read_part_data(uint8_t* tracker_data, uint8_t pattern_index, uint16_t part_data[NUM_ROWS][NUM_CHANNELS][4]) {
//    int pattern_index = tracker_data[952 + part_number];
    uint8_t* pattern_data = tracker_data + 1084 + pattern_index * NUM_ROWS * NUM_CHANNELS * 4;

    for (int row_index = 0; row_index < NUM_ROWS; row_index++) {
        for (int channel_index = 0; channel_index < NUM_CHANNELS; channel_index++) {
            int byte_index = row_index * NUM_CHANNELS * 4 + channel_index * 4;
            uint8_t byte1 = pattern_data[byte_index];
            uint8_t byte2 = pattern_data[byte_index + 1];
            uint8_t byte3 = pattern_data[byte_index + 2];
            uint8_t byte4 = pattern_data[byte_index + 3];

            uint8_t sample_number = (byte1 & 0xF0) | (byte3 >> 4);
            uint16_t period = ((byte1 & 0x0F) << 8) | byte2;
            uint8_t effect1 = (byte3 & 0x0F);
            uint8_t effect2 = byte4;

            part_data[row_index][channel_index][0] = period;
            part_data[row_index][channel_index][1] = sample_number;
            part_data[row_index][channel_index][2] = effect1;
            part_data[row_index][channel_index][3] = effect2;
        }
    }
}

// AUDIO DATA COMP START ----------------------------------------
uint8_t arpNote[2][4] = {0};
float arpFreq[3][4];
int8_t sample1;
int8_t sample2;
int8_t make_data(float freq, uint8_t vole, uint8_t chl, bool isLoop, uint16_t loopStart, uint16_t loopLen, uint32_t smp_start, uint16_t smp_size, uint8_t smp_vol) {
    if (vole <= 0 || freq < 0) {
        return 0;
    }
    // 计算通道数据索引和浮点数部分
    float data_index_float = data_index[chl];
    int data_index_int_floor = (int)data_index_float;
    float frac = data_index_float - data_index_int_floor;
    // 边界检查和处理
    if (isLoop && data_index_int_floor >= (loopStart + loopLen)) {
        data_index[chl] = loopStart;
    } else if (!isLoop && data_index_int_floor >= smp_size) {
        vol[chl] = 0;
        return 0;
    }
    // 获取采样值
    sample1 = tracker_data[data_index_int_floor + smp_start];
    sample2 = tracker_data[data_index_int_floor + 1 + smp_start];
    // 达到采样末尾时不插值
    if (data_index_int_floor + 1 >= smp_size) {
        if (isLoop) {
            data_index[chl] = loopStart;
            sample2 = tracker_data[loopStart + 1 + smp_start];
        } else {
            return sample1;
        }
    }
    // 更新数据索引
    data_index[chl] += freq / (SMP_RATE << 1);
    return (int8_t)roundf((sample1 + frac * (sample2 - sample1)) * vol_table[vole] * vol_table[smp_vol]);
}
// AUDIO DATA COMP END ------------------------------------------

// float frq[CHL_NUM] = {0};

uint8_t part_buffer_point = 0;
uint16_t part_buffer[BUFFER_PATTERNS][NUM_ROWS][NUM_CHANNELS][4];
bool loadOk;

bool skipToNextPart = false;
uint8_t skipToAnyPart = false;
// COMP TASK START ----------------------------------------------
void comp() {
    uint8_t tick_time = 0;
    uint8_t tick_speed = 6;
    uint8_t arp_p = 0;
    bool enbArp[4] = {false};
    uint8_t volUp[4] = {0};
    uint8_t volDown[4] = {0};
    uint16_t portToneSpeed[4] = {0};
    uint16_t portToneTemp[4] = {0};
    uint16_t portToneSource[4] = {0};
    uint16_t portToneTarget[4] = {0};
    bool enbPortTone[4] = {false};
    uint16_t lastNote[4] = {false};
    bool enbSlideUp[4] = {false};
    uint8_t SlideUp[4] = {false};
    bool enbSlideDown[4] = {false};
    uint8_t SlideDown[4] = {false};
    bool enbVibrato[4] = {false};
    uint8_t VibratoSpeed[4];
    uint8_t VibratoDepth[4];
    uint8_t VibratoPos[4] = {32};
    bool enbTremolo[4] = {false};
    uint8_t TremoloPos[4] = {32};
    uint8_t TremoloSpeed[4] = {32};
    uint8_t TremoloDepth[4] = {32};
    int VibratoItem[4] = {0};
    uint16_t Mtick = 0;
    uint32_t frq[4] = {0};
    uint16_t TICK_NUL = roundf(SMP_RATE * 2 / (125 * 0.4));
    uint8_t volTemp[4];
    uint16_t OfstCfg[4];
    /*
    pwm_audio_config_t pwm_audio_config = {
        .gpio_num_left = GPIO_NUM_26,
        .ledc_channel_left = LEDC_CHANNEL_0,
        .ledc_timer_sel = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .ringbuf_len = BUFF_SIZE
    };
    pwm_audio_init(&pwm_audio_config);
    pwm_audio_set_param(SMP_RATE, SMP_BIT, 1);
    pwm_audio_start();
    // pwm_audio_set_volume(0);
    pwm_audio_write(&buffer, BUFF_SIZE, &wrin, portMAX_DELAY);
    */
    dac_audio_config_t dac_audio_config = {
        .i2s_num = I2S_NUM_0,
        .sample_rate = SMP_RATE,
        .bits_per_sample = SMP_BIT,
        .dac_mode = I2S_DAC_CHANNEL_LEFT_EN,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .max_data_size = BUFF_SIZE << 2
    };
    dac_audio_init(&dac_audio_config);
    dac_audio_set_param(SMP_RATE, SMP_BIT, 1);
    dac_audio_start();
    dac_audio_write(&buffer, BUFF_SIZE, &wrin, portMAX_DELAY);
    vTaskDelay(128);
    dispRedy = true;
    uint8_t chl;
    bool enbRetrigger[4] = {false};
    // uint8_t RetriggerPos[4] = {1};
    uint8_t RetriggerConfig[4] = {0};
    int16_t audio_temp;
    while(true) {
        for(uint16_t i = 0; i < BUFF_SIZE; i++) {
            for(chl = 0; chl < 4; chl++) {
                if (wave_info[smp_num[chl]][4] > 2) {
                    buffer_ch[chl][i] = make_data(frq[chl], vol[chl], chl, true, wave_info[smp_num[chl]][3]*2, wave_info[smp_num[chl]][4]*2, wav_ofst[smp_num[chl]], wave_info[smp_num[chl]][0], wave_info[smp_num[chl]][2]);
                } else {
                    buffer_ch[chl][i] = make_data(frq[chl], vol[chl], chl, false, 0, 0, wav_ofst[smp_num[chl]], wave_info[smp_num[chl]][0], wave_info[smp_num[chl]][2]);
                }
            }
            audio_temp = buffer_ch[0][i] 
                            + buffer_ch[1][i] 
                                + buffer_ch[2][i]
                                    + buffer_ch[3][i];
            limit(audio_temp);
            buffer[i] = (int8_t)clamp((audio_temp / comper), -127, 126);//limit(audio_temp);
            Mtick++;
            if (Mtick == TICK_NUL) {
                Mtick = 0;
                tick_time++;
                arp_p++;
                if (tick_time != tick_speed) {
                    for(chl = 0; chl < 4; chl++) {
                        // period[chl] += enbSlideDown[chl] ? SlideDown[chl] : (enbSlideUp[chl] ? -SlideUp[chl] : 0);
                        if (enbSlideUp[chl]) {
                            // printf("SLIDE UP %d", period[chl]);
                            period[chl] -= SlideUp[chl];
                            enbPortTone[chl] = false;
                            // printf(" - %d = %d\n", SlideUp[chl], period[chl]);
                        }
                        if (enbSlideDown[chl]) {
                            // printf("SLIDE DOWN %d", period[chl]);
                            period[chl] += SlideDown[chl];
                            enbPortTone[chl] = false;
                            // printf(" + %d = %d\n", SlideDown[chl], period[chl]);
                        }
                        vol[chl] += (volUp[chl] - volDown[chl]);
                        vol[chl] = (vol[chl] > 63) ? 63 : (vol[chl] < 1) ? 0 : vol[chl];

                        VibratoItem[chl] = enbVibrato[chl] ? (sine_table[VibratoPos[chl]] * VibratoDepth[chl]) >> 7 : 0;
                        VibratoPos[chl] = enbVibrato[chl] ? (VibratoPos[chl] + VibratoSpeed[chl]) & 63 : 0;

                        if (enbTremolo[chl]) {
                            int tremoloValue = (sine_table[TremoloPos[chl]] * TremoloDepth[chl]) >> 7;
                            vol[chl] += tremoloValue;
                            vol[chl] = (vol[chl] > 64) ? 64 : (vol[chl] < 1) ? 0 : vol[chl];
                            TremoloPos[chl] = (TremoloPos[chl] + TremoloSpeed[chl]) & 63;
                        } else {
                            TremoloPos[chl] = 0;
                        }

                        if (enbPortTone[chl]) {
                            int portToneSpeedValue = portToneSpeed[chl];
                            if (portToneSource[chl] > portToneTarget[chl]) {
                                period[chl] -= portToneSpeedValue;
                                period[chl] = (period[chl] < portToneTarget[chl]) ? portToneTarget[chl] : period[chl];
                                // printf("SIL %d + %d\n", portToneTemp[chl], portToneSpeed[chl]);
                            } else if (portToneSource[chl] < portToneTarget[chl]) {
                                period[chl] += portToneSpeedValue;
                                period[chl] = (period[chl] > portToneTarget[chl]) ? portToneTarget[chl] : period[chl];
                                // printf("SID %d - %d\n", portToneTemp[chl], portToneSpeed[chl]);
                            }
                            // printf("PORTTONE %d to %d. speeed=%d\n", portToneSource[chl], portToneTarget[chl], portToneSpeed[chl]);
                        }
                    }
                } else if (tick_time == tick_speed) {
                    tick_time = 0;
                    for (chl = 0; chl < 4; chl++) {
                        if (part_buffer[part_buffer_point][tracker_point][chl][2] == 13) {
                            if (part_buffer[part_buffer_point][tracker_point][chl][3] == 0) {
                                skipToNextPart = true;
                            }
                        }

                        if (part_buffer[part_buffer_point][tracker_point][chl][2] == 11) {
                            if (part_buffer[part_buffer_point][tracker_point][chl][3]) {
                                skipToAnyPart = part_buffer[part_buffer_point][tracker_point][chl][3];
                            }
                        }

                        if (part_buffer[part_buffer_point][tracker_point][chl][1]) {
                            smp_num[chl] = part_buffer[part_buffer_point][tracker_point][chl][1];
                            vol[chl] = 64;
                        }

                        if (part_buffer[part_buffer_point][tracker_point][chl][0]) {
                            if (part_buffer[part_buffer_point][tracker_point][chl][2] == 3
                                || part_buffer[part_buffer_point][tracker_point][chl][2] == 5) {
                                enbPortTone[chl] = true;
                                if (part_buffer[part_buffer_point][tracker_point][chl][0]) {
                                    portToneTarget[chl] = part_buffer[part_buffer_point][tracker_point][chl][0];
                                    portToneSource[chl] = lastNote[chl];
                                    lastNote[chl] = part_buffer[part_buffer_point][tracker_point][chl][0];
                                }
                                if (enbSlideDown[chl] || enbSlideUp[chl]) {
                                    portToneSource[chl] = period[chl];
                                }
                                // printf("PT TARGET SET TO %d. SOURCE IS %d\n", portToneTarget[chl], portToneSource[chl]);
                                if (part_buffer[part_buffer_point][tracker_point][chl][2] == 5) {
                                    hexToDecimal(part_buffer[part_buffer_point][tracker_point][chl][3], &volUp[chl], &volDown[chl]);
                                    continue;
                                } else {
                                    volUp[chl] = volDown[chl] = 0;
                                }
                                if (part_buffer[part_buffer_point][tracker_point][chl][3]) {
                                    portToneSpeed[chl] = part_buffer[part_buffer_point][tracker_point][chl][3];
                                }
                            } else {
                                data_index[chl] = 0;
                                lastNote[chl] = part_buffer[part_buffer_point][tracker_point][chl][0];
                                period[chl] = lastNote[chl];
                                enbPortTone[chl] = false;
                            }
                            if (!(part_buffer[part_buffer_point][tracker_point][chl][2] == 12) 
                                && part_buffer[part_buffer_point][tracker_point][chl][1]) {
                                vol[chl] = 64;
                            }
                            if (part_buffer[part_buffer_point][tracker_point][chl][1]) {
                                smp_num[chl] = part_buffer[part_buffer_point][tracker_point][chl][1];
                            }
                        }

                        if (part_buffer[part_buffer_point][tracker_point][chl][2] == 10
                                || part_buffer[part_buffer_point][tracker_point][chl][2] == 6) {
                            hexToDecimal(part_buffer[part_buffer_point][tracker_point][chl][3], &volUp[chl], &volDown[chl]);
                            // printf("VOL+=%d -=%d\n", volUp[chl], volDown[chl]);
                        } else {
                            volUp[chl] = volDown[chl] = 0;
                        }

                        if (part_buffer[part_buffer_point][tracker_point][chl][2] == 1) {
                            SlideUp[chl] = part_buffer[part_buffer_point][tracker_point][chl][3];
                            // printf("SET SLIDEUP IS %d\n", part_buffer[part_buffer_point][tracker_point][chl][3]);
                            enbSlideUp[chl] = true;
                        } else {
                            enbSlideUp[chl] = false;
                        }

                        if (part_buffer[part_buffer_point][tracker_point][chl][2] == 2) {
                            SlideDown[chl] = part_buffer[part_buffer_point][tracker_point][chl][3];
                            // printf("SET SLIDEDOWN IS %d\n", part_buffer[part_buffer_point][tracker_point][chl][3]);
                            enbSlideDown[chl] = true;
                        } else {
                            enbSlideDown[chl] = false;
                        }

                        if (part_buffer[part_buffer_point][tracker_point][chl][2] == 4
                                || part_buffer[part_buffer_point][tracker_point][chl][2] == 6) {
                            enbVibrato[chl] = true;
                            if ((part_buffer[part_buffer_point][tracker_point][chl][2] == 4)) {
                                if (hexToDecimalTens(part_buffer[part_buffer_point][tracker_point][chl][3])) {
                                    VibratoSpeed[chl] = hexToDecimalTens(part_buffer[part_buffer_point][tracker_point][chl][3]);
                                }
                                if (hexToDecimalOnes(part_buffer[part_buffer_point][tracker_point][chl][3])) {
                                    VibratoDepth[chl] = hexToDecimalOnes(part_buffer[part_buffer_point][tracker_point][chl][3]);
                                }
                                // printf("VIBRATO SPD %d DPH %d\n", VibratoSpeed[chl], VibratoDepth[chl]);
                            }
                        } else {
                            enbVibrato[chl] = false;
                        }

                        if (part_buffer[part_buffer_point][tracker_point][chl][2] == 7) {
                            enbVibrato[chl] = true;
                            if (hexToDecimalTens(part_buffer[part_buffer_point][tracker_point][chl][3])) {
                                TremoloSpeed[chl] = hexToDecimalTens(part_buffer[part_buffer_point][tracker_point][chl][3]);
                            }
                            if (hexToDecimalOnes(part_buffer[part_buffer_point][tracker_point][chl][3])) {
                                TremoloDepth[chl] = hexToDecimalOnes(part_buffer[part_buffer_point][tracker_point][chl][3]);
                            }
                            // printf("TREMOLO SPD %d DPH %d\n", VibratoSpeed[chl], VibratoDepth[chl]);
                        } else {
                            enbTremolo[chl] = false;
                        }

                        if (part_buffer[part_buffer_point][tracker_point][chl][2] == 9) {
                            OfstCfg[chl] = part_buffer[part_buffer_point][tracker_point][chl][3] << 8;
                            printf("SMP OFST %d\n", OfstCfg[chl]);
                        }

                        if (part_buffer[part_buffer_point][tracker_point][chl][2] == 12) {
                            vol[chl] = part_buffer[part_buffer_point][tracker_point][chl][3];
                        }
                        enbRetrigger[chl] = false;
                        if (part_buffer[part_buffer_point][tracker_point][chl][2] == 14) {
                            uint8_t decimalTens = hexToDecimalTens(part_buffer[part_buffer_point][tracker_point][chl][3]);
                            vol[chl] += (decimalTens == 10) ? hexToDecimalOnes(part_buffer[part_buffer_point][tracker_point][chl][3]) : ((decimalTens == 11) ? -hexToDecimalOnes(part_buffer[part_buffer_point][tracker_point][chl][3]) : 0);
                            vol[chl] = (vol[chl] > 64) ? 64 : ((vol[chl] < 1) ? 0 : vol[chl]);
                            if (decimalTens == 9) {
                                enbRetrigger[chl] = true;
                                RetriggerConfig[chl] = hexToDecimalOnes(part_buffer[part_buffer_point][tracker_point][chl][3]);
                                printf("RETRIGGER %d\n", RetriggerConfig[chl]);
                                volTemp[chl] = vol[chl];
                            }
                            // printf("LINE VOL %s TO %d\n", (decimalTens == 10) ? "UP" : ((decimalTens == 11) ? "DOWN" : "UNCHANGED"), vol[chl]);
                        }

                        if (part_buffer[part_buffer_point][tracker_point][chl][2] == 15) {
                            if (part_buffer[part_buffer_point][tracker_point][chl][3] < 32) {
                                tick_speed = part_buffer[part_buffer_point][tracker_point][chl][3];
                                // printf("SPD SET TO %d\n", tick_speed);
                            } else {
                                TICK_NUL = roundf((SMP_RATE << 1) / (part_buffer[part_buffer_point][tracker_point][chl][3] * 0.4));
                                // printf("MTICK SET TO %d\n", TICK_NUL);
                            }
                        }

                        if ((!part_buffer[part_buffer_point][tracker_point][chl][2])
                                && part_buffer[part_buffer_point][tracker_point][chl][3]) {
                            arp_p = 0;
                            hexToDecimal(part_buffer[part_buffer_point][tracker_point][chl][3], &arpNote[0][chl], &arpNote[1][chl]);
                            if (part_buffer[part_buffer_point][tracker_point][chl][0]) {
                                arpFreq[0][chl] = patch_table[wave_info[smp_num[chl]][1]] / period[chl];
                                arpFreq[1][chl] = freq_up(arpFreq[0][chl], arpNote[0][chl]);
                                arpFreq[2][chl] = freq_up(arpFreq[0][chl], arpNote[1][chl]);
                                // printf("ARP CTRL %d %d %f %f %f\n", arpNote[0][chl], arpNote[1][chl], arpFreq[0][chl], arpFreq[1][chl], arpFreq[2][chl]);
                            } else {
                                arpFreq[1][chl] = freq_up(arpFreq[0][chl], arpNote[0][chl]);
                                arpFreq[2][chl] = freq_up(arpFreq[0][chl], arpNote[1][chl]);
                            }
                            enbArp[chl] = true;
                        } else {
                            if (enbArp[chl]) {
                                frq[chl] = arpFreq[0][chl];
                                enbArp[chl] = false;
                            }
                        }
                        //if (chl == 3) {
                        //printf("PART=%d CHL=%d PINT=%d SPED=%d VOLE=%d VOL_F=%f FREQ=%f EFX1=%d EFX2=%d NOTE=%d SMPL=%d\n",
                        //    part_point, chl, tracker_point, tick_speed, vol[chl], vol_table[vol[chl]], frq[chl], part_buffer[part_buffer_point][tracker_point][chl][2], part_buffer[part_buffer_point][tracker_point][chl][3], part_buffer[part_buffer_point][tracker_point][chl][0], smp_num[chl]);
                        //}
                    }
                    tracker_point++;
                    if ((tracker_point > 63) || skipToNextPart || skipToAnyPart) {
                        tracker_point = 0;
                        if (skipToAnyPart) {
                            part_point = skipToAnyPart;
                            printf("SKIP TO %d\n", part_point);
                            skipToAnyPart = false;
                            read_part_data(tracker_data, part_table[part_point], part_buffer[!part_buffer_point]);
                            part_point++;
                        }
                        skipToNextPart = false;
                        part_buffer_point++;
                        if (part_buffer_point > 1){
                            part_buffer_point = 0;
                        }
                        loadOk = true;
                        printf("%d\n", part_buffer_point);
                    }
                }
                for (chl = 0; chl < 4; chl++) {
                    frq[chl] = patch_table[wave_info[smp_num[chl]][1]] / (period[chl] + VibratoItem[chl]);
                    if (enbRetrigger[chl]) {
                        // printf("RETPOS %d\n", RetriggerPos[chl]);
                        // if (RetriggerPos[chl] > RetriggerConfig[chl]) {
                        if (!(tick_time % RetriggerConfig[chl])) {
                            // printf("EXEC RET %d\n", RetriggerPos[chl]);
                            data_index[chl] = 0;
                            vol[chl] = volTemp[chl];
                        }
                    }
                    if (OfstCfg[chl]) {
                        data_index[chl] = OfstCfg[chl];
                        OfstCfg[chl] = 0;
                    }
                    if (arp_p > 2) {arp_p = 0;}
                    if (enbArp[chl]) {
                        frq[chl] = arpFreq[arp_p][chl];
                    }
                }
            }
        }
        dac_audio_write(&buffer, BUFF_SIZE, &wrin, portMAX_DELAY);
        vTaskDelay(1);
        //ESP_LOGI("STEP_SIZE", "%d %d", wrin, BUFF_SIZE);
    }
}
// COMP TASK END ------------------------------------------------

void load() {
    while(true) {
        if (part_buffer_point == 0 && loadOk) {
            read_part_data(tracker_data, part_table[part_point], part_buffer[1]);
            part_point++;
            if (part_point >= NUM_PATTERNS) {
                part_point = 0;
            }
            loadOk = false;
        } else if (part_buffer_point == 1 && loadOk) {
            read_part_data(tracker_data, part_table[part_point], part_buffer[0]);
            part_point++;
            if (part_point >= NUM_PATTERNS) {
                part_point = 0;
            }
            loadOk = false;
        }
        vTaskDelay(4);
    }
}

void read_pattern_table() {
    NUM_PATTERNS = tracker_data[950];
    for (uint8_t i = 0; i < NUM_PATTERNS; i++) {
        part_table[i] = tracker_data[952 + i];
        printf("%d ", part_table[i]);
    }
    printf("\n");
}

int find_max(int size) {
    if (size <= 0) {
        return -1;
    }
    int max = part_table[0];
    for (int i = 1; i < size; i++) {
        if (part_table[i] > max) {
            max = part_table[i];
        }
    }
    return max;
}

void read_wave_info() {
    for (uint8_t i = 1; i < 33; i++) {
        uint8_t* sample_data = tracker_data + 20 + (i - 1) * 30;
        wave_info[i][0] = ((sample_data[22] << 8) | sample_data[23]) * 2;  // 采样长度
        wave_info[i][1] = sample_data[24];  // 微调值
        wave_info[i][2] = sample_data[25];  // 音量
        wave_info[i][3] = (sample_data[26] << 8) | sample_data[27];  // 重复点
        wave_info[i][4] = (sample_data[28] << 8) | sample_data[29];  // 重复长度
        ESP_LOGI("WAVE INFO", "NUM=%d LEN=%d PAT=%d VOL=%d LOOPSTART=%d LOOPLEN=%d TRK_MAX=%d", i, wave_info[i][0], wave_info[i][1], wave_info[i][2], wave_info[i][3], wave_info[i][4], find_max(NUM_PATTERNS)+1);
    }
}

void comp_wave_ofst() {
    wav_ofst[1] = 1084 + ((find_max(NUM_PATTERNS)+1) * 1024);
    ESP_LOGI("FIND MAX", "%d OFST %ld", find_max(NUM_PATTERNS), wav_ofst[1]);
/*
    for (uint8_t i = 2; i < 33; i++) {
        printf("%d %d\n", i, wav_ofst[i]);
        wav_ofst[i] += wave_info[i][0];
        wav_ofst[i+1] = wav_ofst[i];
    }
*/
    for (uint8_t i = 1; i < 33; i++) {
        printf("1 %ld %ld %d\n", wav_ofst[i+1], wav_ofst[i], wave_info[i+1][0]);
        wav_ofst[i+1] += (wav_ofst[i] + wave_info[i][0]);
    }
}

/*
void read_wave_data(uint8_t (*wave_info)[5], uint8_t* tracker_data, uint8_t** wave_data) {
    for (int i = 0; i < 32; i++) {
        uint16_t sample_length = wave_info[i][0] * 2;
        wave_data[i] = (uint8_t*)malloc(sample_length * sizeof(uint8_t));
        if (wave_data[i] == NULL) {
            ESP_LOGE("WAVE READ", "MEMRY MALLOC FAIL!");
            exit(EXIT_FAILURE);
        }
        for (int j = 0; j < sample_length; j++) {
            wave_data[i][j] = tracker_data[20 + i * 30 + j];
        }
    }
}
*/

void app_main(void)
{
    xTaskCreate(&display, "wave_view", 7000, NULL, 5, NULL);
    // xTaskCreatePinnedToCore(&Cpu_task, "cpu_task", 4096, NULL, 0, NULL, 0);
/*
    for (int i = 0; i < NUM_PATTERNS; i++) {
        uint8_t* pattern_data = tracker_data + 1084 + i * PATTERN_SIZE;
        decode_pattern_data(pattern_data, part_data);
    }
*/
    read_pattern_table();
    read_wave_info(wave_info, tracker_data);
    comp_wave_ofst();
    // read_wave_data(wave_info, tracker_data, wave_data);
//    for (int i = 0; i <= 100; i++) {
//        midi_notes[i] = midi_note_frequency(i);
//    }
    for (uint8_t i = 1; i < 33; i++) {
        ESP_LOGI("WAVE INFO", "NUM=%d LEN=%d PAT=%d VOL=%d LOOPSTART=%d LOOPLEN=%d TRK_MAX=%d", i, wave_info[i][0], wave_info[i][1], wave_info[i][2], wave_info[i][3]*2, (wave_info[i][3]*2)+(wave_info[i][4]*2), find_max(NUM_PATTERNS)+1);
    }
    read_part_data(tracker_data, part_table[0], part_buffer[0]);
    read_part_data(tracker_data, part_table[1], part_buffer[1]);
    xTaskCreate(&comp, "Play", 8200, NULL, 5, NULL);
    xTaskCreatePinnedToCore(&load, "Load", 8192, NULL, 0, NULL, 0);
    vTaskDelay(32);
    uint8_t debugPart = 5;
    uint8_t debugChl = 0;
    for (uint8_t i = 0; i < NUM_ROWS; i++) {
        printf("Pattern %d, Row %d, Channel 3: Period=%d, Sample Number=%d, Effect1=%d, Effect2=%d\n",
               debugPart, i, part_buffer[debugPart][i][debugChl][0], part_buffer[debugPart][i][debugChl][1], part_buffer[debugPart][i][debugChl][2], part_buffer[debugPart][i][debugChl][3]);
    }
    for (uint8_t i = 0; i < NUM_ROWS; i++) {
        printf("Pattern %d, Row %d, Channel 3: Period=%d, Sample Number=%d, Effect1=%d, Effect2=%d\n",
               debugPart+1, i, part_buffer[debugPart][i][debugChl][0], part_buffer[debugPart][i][debugChl][1], part_buffer[debugPart][i][debugChl][2], part_buffer[debugPart][i][debugChl][3]);
    }
}