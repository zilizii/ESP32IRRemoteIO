/*
 * ESP32IRRemoteIO.cpp
 *
 *  Created on: 2018. márc. 11.
 *      Author: Dell
 */
#include <iostream>

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <map>
#include <vector>
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_intr.h"
#include "esp_intr_alloc.h"
#include "nvs_flash.h"
#include "esp_int_wdt.h"
#include "esp_task_wdt.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_intr_alloc.h"

#include "soc/cpu.h"
#include "soc/dport_reg.h"
#include "soc/io_mux_reg.h"
#include "soc/rtc_cntl_reg.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "sdkconfig.h"

#include "ESP32Remote.h"


/// Define section
using namespace std;

/// Function section

extern "C" {
	void app_main(void);
}

#define GPIO_INPUT_IO_0     GPIO_NUM_18
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_0)
#define ESP_INTR_FLAG_DEFAULT 0

static xQueueHandle gpio_evt_queue = NULL;

ESP32_RMT_Rx irrecv(22, 0);
ESP32_RMT_Tx irsend(23, 1);




/*
 * Playing with the IDE
 * GPIO used for triggering the test code
 * catching the GPIO specific changes
 * */
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}


static uint8_t command=0;

void rtmInput(void * parameters) {

	irrecv.init();
	while(true)
	{
		command=irrecv.readIRrec();
		if (command!=0) {
			printf("NEC Command received : %i\n",command);
		}

	}
	vTaskDelete(NULL);
}

void rmtOutput(void * parameters){

	std::map<std::string, pair<int,int>> storage ;

	storage.insert(std::pair<string,pair<int,int>> ("RED", pair<int,int>(0xF7,0x20) ));
	storage.insert(std::pair<string,pair<int,int>> ("GREEN", pair<int,int>(0xF7,0xA0) ));
	storage.insert(std::pair<string,pair<int,int>> ("BLUE", pair<int,int>(0xF7,0x60) ));
	uint address = 0x00;
	uint data = 0x00;
	irsend.init();
	ProtocolData_t * ChineseLED = new ProtocolData_t();
	ChineseLED->_name = "Chinese LED";
	ChineseLED->_length = 32;
	ChineseLED->_headerHigh = 9375;
	ChineseLED->_headerLow =  4500;
	ChineseLED->_highTimeHigh = 600;
	ChineseLED->_highTimeLow = 1650;
	ChineseLED->_lowTimeHigh = 600;
	ChineseLED->_lowTimeLow = 560;
	ChineseLED->_isAddress = true;
	ChineseLED->_isInvertedAddressRequired = false;
	ChineseLED->_addressLength = 16;
	ChineseLED->_isDataInverseAddedRequired = true;
	ChineseLED->_tolerance = 50;
	ChineseLED->_isStop = true;
	ChineseLED->_stopSignHigh = 560;
	ChineseLED->_stopSignLow = 1000;
	rmt_item32_t* item = new rmt_item32_t[ChineseLED->_length + 2];
	PulseDistanceCoding * ChineseLEDDriver = new  PulseDistanceCoding(ChineseLED);
	ChineseLEDDriver->GenerateOutput(item,0x00F7, 0xC0);
	ChineseLEDDriver->DecodeInput(item, address, data);
	printf("Some important address : %04x and data %04x \n",address,data);
	irsend.sendIR(item,ChineseLED->_length + 2);

	for(;;) {

		for(std::map<std::string,pair<int,int>>::iterator itr = storage.begin(); itr != storage.end(); ++itr ) {
			address = 0x0;
			data = 0x0;
			ChineseLEDDriver->GenerateOutput(item, (itr->second).first , (itr->second).second);
			ChineseLEDDriver->DecodeInput(item, address, data);
			//printf("Some important address : %04x and data %04x \n",address,data);
			irsend.sendIR(item,ChineseLED->_length + 2);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
		}
	}
	vTaskDelete(NULL);
}






void button(void * parameters) {

	uint32_t gpio_num;
	for(;;)
	{
		if( xQueueReceive( gpio_evt_queue, &gpio_num, (TickType_t) 0))
		{
			switch(gpio_num) {
			case(GPIO_INPUT_IO_0):
				printf("Buttoniser \n");
				break;
			default:
				printf("GPIO : %i \n",gpio_num);
			};


		}
	}
	vTaskDelete(NULL);
}



void app_main(void) {
	nvs_flash_init();

	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t) );

	/*
	 * GPIO setup
	 * */
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_NEGEDGE;
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_config(&io_conf);
	/*
	 * End of GPIO setup
	 */

	//gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_POSEDGE);

	//install gpio isr service
	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);


	//hook isr handler for specific gpio pin
	gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);

	xTaskCreate(rmtOutput, "test task", 2048, NULL , 10, NULL);
	while(1) {
	        vTaskDelay(1000 / portTICK_PERIOD_MS);
	    }


	//xTaskCreate(rtmInput, "remote control input task", 2048, NULL , 10, NULL);

}





