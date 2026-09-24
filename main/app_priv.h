/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <esp_err.h>
#include <esp_matter.h>

typedef void *app_driver_handle_t;

/** Initialize the PIR occupancy sensor driver
 *
 * Configures the PIR GPIO (CONFIG_PIR_GPIO_NUM) as an interrupt-driven
 * input and starts the background task that reports occupancy changes
 * to the Matter data model whenever the pin's level changes.
 *
 * @return Handle on success.
 * @return NULL in case of failure.
 */
app_driver_handle_t app_driver_occupancy_sensor_init();

/** Report the PIR's current level as the initial value of the
 * `Occupancy` attribute.
 *
 * Must be called after the occupancy_sensor endpoint has been created
 * and `esp_matter::start()` has run, so the attribute store already
 * has a valid entry for `endpoint_id`.
 *
 * @param[in] endpoint_id Endpoint ID of the occupancy_sensor endpoint.
 *
 * @return ESP_OK on success.
 */
esp_err_t app_driver_occupancy_sensor_set_defaults(uint16_t endpoint_id);

/** Initialize the button driver (reused for factory reset only — this
 * project has no toggle action to register on it).
 *
 * @return Handle on success.
 * @return NULL in case of failure.
 */
app_driver_handle_t app_driver_button_init();
