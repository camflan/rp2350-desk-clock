// lv_conf.h
#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

#define LV_HOR_RES_MAX 466
#define LV_VER_RES_MAX 466
#define LV_DPI_DEF 320

#define LV_MEM_SIZE (128 * 1024)
#define LV_USE_DRAW_SW 1

#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "pico/time.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (to_ms_since_boot(get_absolute_time()))

#endif
