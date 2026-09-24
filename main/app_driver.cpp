/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <driver/gpio.h>

#include <esp_matter.h>
#include <app_priv.h>

#include <device.h>
#include <button_gpio.h>

using namespace chip::app::Clusters;
using namespace esp_matter;

static const char *TAG = "app_driver";
extern uint16_t occupancy_endpoint_id;

static SemaphoreHandle_t pir_event_semaphore = NULL;

static esp_err_t report_occupancy(uint16_t endpoint_id, bool occupied)
{
    if (endpoint_id == 0) {
        return ESP_OK;
    }
    esp_matter_attr_val_t val = esp_matter_bitmap8(occupied ? 1 : 0);
    return attribute::update(endpoint_id, OccupancySensing::Id, OccupancySensing::Attributes::Occupancy::Id, &val);
}

static void IRAM_ATTR pir_gpio_isr_handler(void *arg)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    xSemaphoreGiveFromISR(pir_event_semaphore, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

static void occupancy_sensor_task(void *arg)
{
    gpio_num_t pir_gpio = (gpio_num_t)CONFIG_PIR_GPIO_NUM;

    while (true) {
        if (xSemaphoreTake(pir_event_semaphore, portMAX_DELAY) == pdTRUE) {
            bool occupied = gpio_get_level(pir_gpio);
            ESP_LOGI(TAG, "PIR state changed, occupied=%d", occupied);
            report_occupancy(occupancy_endpoint_id, occupied);
        }
    }
}

app_driver_handle_t app_driver_occupancy_sensor_init()
{
    gpio_num_t pir_gpio = (gpio_num_t)CONFIG_PIR_GPIO_NUM;

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = 1ULL << pir_gpio;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure PIR GPIO");
        return NULL;
    }

    pir_event_semaphore = xSemaphoreCreateBinary();
    if (pir_event_semaphore == NULL) {
        ESP_LOGE(TAG, "Failed to create PIR event semaphore");
        return NULL;
    }

    esp_err_t isr_service_err = gpio_install_isr_service(0);
    if (isr_service_err != ESP_OK && isr_service_err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to install GPIO ISR service");
        return NULL;
    }

    if (gpio_isr_handler_add(pir_gpio, pir_gpio_isr_handler, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add GPIO ISR handler");
        return NULL;
    }

    BaseType_t task_result = xTaskCreate(occupancy_sensor_task, "occupancy_sensor_task", 4096, NULL, 5, NULL);
    if (task_result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create occupancy sensor task");
        return NULL;
    }

    return (app_driver_handle_t)pir_event_semaphore;
}

esp_err_t app_driver_occupancy_sensor_set_defaults(uint16_t endpoint_id)
{
    gpio_num_t pir_gpio = (gpio_num_t)CONFIG_PIR_GPIO_NUM;
    return report_occupancy(endpoint_id, gpio_get_level(pir_gpio));
}

app_driver_handle_t app_driver_button_init()
{
    button_handle_t handle = NULL;
    const button_config_t btn_cfg = {0};
    const button_gpio_config_t btn_gpio_cfg = button_driver_get_config();

    if (iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create button device");
        return NULL;
    }

    return (app_driver_handle_t)handle;
}
