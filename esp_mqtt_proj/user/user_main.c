/* main.c -- MQTT client example
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
#include "sntp.h"
#include "user_light.h"

MQTT_Client mqttClient;
typedef unsigned long u32_t;
static ETSTimer sntp_timer;

#define LIGHT_SWITCH_ON          "ON"
#define LIGHT_SWITCH_OFF         "OFF"

#define LIGHT_SWITCH_STATE       "/mqtt/switch/state"
#define LIGHT_SWITCH_CONTROL     "/mqtt/switch/control"
#define LIGHT_BRIGHTNESS_STATE   "/mqtt/brightness/state"
#define LIGHT_BRIGHTNESS_CONTROL "/mqtt/brightness/control"

#define LIGHT_GPIO_1 			 5
#define LIGHT_GPIO_ON 			 1
#define LIGHT_GPIO_OFF 			 0

#define LIGHT_PWM_CYCLE			    1000
#define LIGHT_PWM_DUTY_MAX  		(LIGHT_PWM_CYCLE*1000/45)
#define LIGHT_PWM_DUTY_UNIT         (LIGHT_PWM_DUTY_MAX/100)
#define LIGHT_PWM_DUTY(n)           ((n*45)/(LIGHT_PWM_CYCLE * 1000))

uint32 io_info[][3] =
{{PWM_4_OUT_IO_MUX,PWM_4_OUT_IO_FUNC,PWM_4_OUT_IO_NUM},
{PWM_1_OUT_IO_MUX,PWM_1_OUT_IO_FUNC,PWM_1_OUT_IO_NUM},
{PWM_2_OUT_IO_MUX,PWM_2_OUT_IO_FUNC,PWM_2_OUT_IO_NUM}};
struct light_param light_p;
struct light_state light_s;
void  hw_timer_cmd(u8 cmd)
{
    u32 ReadValue, Value;
    ReadValue = RTC_REG_READ(FRC1_CTRL_ADDRESS);

    if(cmd == 0)  Value = ReadValue & (~BIT7);
    else Value = ReadValue | BIT7;

    RTC_REG_WRITE(FRC1_CTRL_ADDRESS, Value);
}

void sntpfn()
{
    u32_t ts = 0;
    ts = sntp_get_current_timestamp();
    os_printf("current time : %s\n", sntp_get_real_time(ts));
    if (ts == 0) {
        //os_printf("did not get a valid time from sntp server\n");
    } else {
            os_timer_disarm(&sntp_timer);
            MQTT_Connect(&mqttClient);
    }
}


void wifiConnectCb(uint8_t status)
{
    if(status == STATION_GOT_IP){
        sntp_setservername(0, "cn.pool.ntp.org");        // set sntp server after got ip address
        sntp_init();
        os_timer_disarm(&sntp_timer);
        os_timer_setfn(&sntp_timer, (os_timer_func_t *)sntpfn, NULL);
        os_timer_arm(&sntp_timer, 1000, 1);//1s
    } else {
          MQTT_Disconnect(&mqttClient);
    }
}

void mqttConnectedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    char bright[3];
    uint32 value, duty;
    INFO("MQTT: Connected\r\n");


    MQTT_Subscribe(client, LIGHT_SWITCH_CONTROL, 0);
    MQTT_Subscribe(client, LIGHT_BRIGHTNESS_CONTROL, 0);

    duty = LIGHT_PWM_DUTY(light_p.pwm_duty[0]);
    os_sprintf(bright, "%d", duty);
    MQTT_Publish(client, LIGHT_BRIGHTNESS_STATE, bright, strlen(bright), 0, 0);

    if(light_s.state == LIGHT_GPIO_OFF)
    {
    	MQTT_Publish(client, LIGHT_SWITCH_STATE, LIGHT_SWITCH_OFF, 3, 0, 0);
    }
    else
    {
    	MQTT_Publish(client, LIGHT_SWITCH_STATE, LIGHT_SWITCH_ON, 2, 0, 0);
    }
}

void mqttDisconnectedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    INFO("MQTT: Disconnected\r\n");
}

void mqttPublishedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    INFO("MQTT: Published\r\n");
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
    char *topicBuf = (char*)os_zalloc(topic_len+1),
            *dataBuf = (char*)os_zalloc(data_len+1);
    uint32 brightness, duty;
    MQTT_Client* client = (MQTT_Client*)args;

    os_memcpy(topicBuf, topic, topic_len);
    topicBuf[topic_len] = 0;

    os_memcpy(dataBuf, data, data_len);
    dataBuf[data_len] = 0;

    if(0 != os_strstr(topicBuf, LIGHT_SWITCH_CONTROL))
    {
    	if(0 != os_strstr(dataBuf, LIGHT_SWITCH_OFF))
    	{
    		MQTT_Publish(client, LIGHT_SWITCH_STATE, LIGHT_SWITCH_OFF, 3, 0, 0);
    		hw_timer_cmd(0);
    		GPIO_OUTPUT_SET(LIGHT_GPIO_1, LIGHT_GPIO_OFF);
    		light_s.state = LIGHT_GPIO_OFF;
    	}
    	else
    	{
    		MQTT_Publish(client, LIGHT_SWITCH_STATE, LIGHT_SWITCH_ON, 2, 0, 0);
    		hw_timer_cmd(1);
    		pwm_set_duty(light_p.pwm_duty[0], 0);
    		pwm_start();
    		light_s.state = LIGHT_GPIO_ON;
    	}

    }

    if(0 != os_strstr(topicBuf, LIGHT_BRIGHTNESS_CONTROL))
    {
    	if(data_len == 3)
    	{
    		brightness = ((dataBuf[0] - 0x30)*100);
    		brightness += ((dataBuf[1] - 0x30) * 10);
    		brightness += (dataBuf[2] - 0x30);
    	}
    	else if(data_len == 2)
    	{
    		brightness = ((dataBuf[0] - 0x30)*10);
    		brightness += (dataBuf[1] - 0x30) ;
    	}
    	else
    	{
    		brightness += (dataBuf[0] - 0x30) ;
    	}
    	MQTT_Publish(client, LIGHT_BRIGHTNESS_STATE, dataBuf, strlen(dataBuf), 0, 0);
        if(brightness == 0)
        {
        	MQTT_Publish(client, LIGHT_SWITCH_STATE, LIGHT_SWITCH_OFF, 3, 0, 0);
        	light_p.pwm_duty[0] = 0;
        	light_s.state = LIGHT_GPIO_OFF;
        }
        else{
        	MQTT_Publish(client, LIGHT_SWITCH_STATE, LIGHT_SWITCH_ON, 2, 0, 0);
        	light_p.pwm_duty[0] = brightness*LIGHT_PWM_DUTY_UNIT;
        	light_s.state = LIGHT_GPIO_ON;
        }
    	pwm_set_duty(light_p.pwm_duty[0], 0);
    	pwm_start();
    }

    INFO("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);
    os_free(topicBuf);
    os_free(dataBuf);
}


/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
 *******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void user_init(void)
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_delay_us(60000);
    CFG_Load();
    light_p.pwm_period = LIGHT_PWM_CYCLE;
    light_p.pwm_duty[0] = LIGHT_PWM_DUTY_MAX;
    light_s.state = LIGHT_GPIO_OFF;
    pwm_init(light_p.pwm_period, light_p.pwm_duty, 1, io_info);
    hw_timer_cmd(0);
    GPIO_OUTPUT_SET(LIGHT_GPIO_1, LIGHT_GPIO_OFF);

    MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);
    //MQTT_InitConnection(&mqttClient, "192.168.11.122", 1880, 0);

    MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
    //MQTT_InitClient(&mqttClient, "client_id", "user", "pass", 120, 1);

    MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);
    MQTT_OnConnected(&mqttClient, mqttConnectedCb);
    MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
    MQTT_OnPublished(&mqttClient, mqttPublishedCb);
    MQTT_OnData(&mqttClient, mqttDataCb);

    WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);

    INFO("\r\nSystem started ...\r\n");
}
