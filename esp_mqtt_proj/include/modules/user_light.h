/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __USER_LIGHT_H__
#define __USER_LIGHT_H__
/*pwm.h: function and macro definition of PWM API , driver level */
/*user_light.h: user interface for light API, user level*/
/*user_light_adj: API for color changing and lighting effects, user level*/


#include "pwm.h"


/* NOTICE !!! ---this is for 512KB spi flash.*/
/* You can change to other sector if you use other size spi flash. */
/* Refer to the documentation about OTA support and flash mapping*/
//#define PRIV_PARAM_START_SEC		0xFC //0x3C or 0x7C, or 0xFC
//#define PRIV_PARAM_SAVE     0



/*Define the channel number of PWM*/
/*In this demo, we can set 3 for 3 PWM channels: RED, GREEN, BLUE*/
/*Or , we can choose 5 channels : RED,GREEN,BLUE,COLD-WHITE,WARM-WHITE*/
#define PWM_CHANNEL	1  //  5:5channel ; 3:3channel


/*Definition of GPIO PIN params, for GPIO initialization*/
#define PWM_0_OUT_IO_MUX PERIPHS_IO_MUX_MTDI_U
#define PWM_0_OUT_IO_NUM 12
#define PWM_0_OUT_IO_FUNC  FUNC_GPIO12

#define PWM_1_OUT_IO_MUX PERIPHS_IO_MUX_MTDO_U
#define PWM_1_OUT_IO_NUM 15
#define PWM_1_OUT_IO_FUNC  FUNC_GPIO15

#define PWM_2_OUT_IO_MUX PERIPHS_IO_MUX_MTCK_U
#define PWM_2_OUT_IO_NUM 13
#define PWM_2_OUT_IO_FUNC  FUNC_GPIO13

#define PWM_3_OUT_IO_MUX PERIPHS_IO_MUX_MTMS_U
#define PWM_3_OUT_IO_NUM 14
#define PWM_3_OUT_IO_FUNC  FUNC_GPIO14

#define PWM_4_OUT_IO_MUX PERIPHS_IO_MUX_GPIO5_U
#define PWM_4_OUT_IO_NUM 5
#define PWM_4_OUT_IO_FUNC  FUNC_GPIO5

struct light_param {
    uint32  pwm_period;
    uint32  pwm_duty[PWM_CHANNEL];
};

struct light_state {
    uint32  state;
};
#endif

