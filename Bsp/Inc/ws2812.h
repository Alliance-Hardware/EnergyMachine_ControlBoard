#ifndef __WS2812_H__
#define __WS2812_H__

#include "main.h"
#include "gpio.h"
#include "tim.h"
#include "stdio.h"
#include "string.h"
//定时器相关参数
#define TIMER &htim2
#define CHANNEL TIM_CHANNEL_3
//这里的周期是90,根据定时器实际情况修改
#define CODE_ONE_DUTY 66
#define CODE_ZERO_DUTY 21
//WS2812级联数量
#define WS2812_NUM 64

#define RST_PERIOD_NUM 100

extern uint32_t ws2812_color[WS2812_NUM];

// 将颜色数组直接更新到 LED，不使用渐变过渡
void ws2812_update(void);

// 渐变的更新LED颜色
void ws2812_gradient(uint8_t steps, uint16_t delay_ms);

// 设置LED颜色（24bit颜色）
void ws2812_set(uint8_t led_id, uint32_t color);

// 设置LED颜色（RGB）
void ws2812_set_rgb(uint8_t led_id, uint8_t r, uint8_t g, uint8_t b);

// 设置所有LED颜色
void ws2812_set_all(uint32_t color);

// RGB转换为24bit颜色
uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b);

// 24bit颜色转换为RGB
void color_to_rgb(uint32_t color, uint8_t *r, uint8_t *g, uint8_t *b);

#endif
