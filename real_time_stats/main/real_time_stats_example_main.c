/* FreeRTOS Real Time Stats Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "station_wifi.h"
#include "bus_serial_driver.h"

static const char *TAG = "task simple";

static void usart_recv_task(void *arg)
{
    bus_serial_driver_t* serial = (bus_serial_driver_t*)arg;
    while (1) {
        bus_sync_rx((bus_driver_t**)serial);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void usart_send_task(void *arg)
{
    bus_serial_driver_t* serial = (bus_serial_driver_t*)arg;
    while (1) {
        bus_sync_tx((bus_driver_t**)serial);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void recv_task(void *arg)
{
    uint8_t buffer[256];
    bus_serial_driver_t* serial = (bus_serial_driver_t*)arg;
    while (1) {
        bus_read((bus_driver_t**)serial, buffer, 256);     
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void send_task(void *arg)
{
    uint8_t buffer[256] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
    bus_serial_driver_t* serial = (bus_serial_driver_t*)arg;
    while (1) {   
        bus_write((bus_driver_t**)serial,buffer,10);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void app_main(void)
{
    //Allow other core to finish initialization
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();


    bus_serial_driver_t* serial = bus_serial_driver_register("ttyEsp", 1);
    bus_init((bus_driver_t**)serial);

    xTaskCreatePinnedToCore(usart_recv_task, "usart_recv_task", 1024, serial, 3, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(usart_send_task, "usart_send_task", 1024, serial, 3, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(recv_task, "recv_task", 1024, serial, 3, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(send_task, "send_task", 1024, serial, 3, NULL, tskNO_AFFINITY);
}
