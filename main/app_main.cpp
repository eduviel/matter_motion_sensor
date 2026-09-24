/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_console.h>

#include <common_macros.h>

#include <app_priv.h>
#include <app_reset.h>

/* Note: main/certification_declaration/ is copied from the sibling light/
 * project as a placeholder. Unlike light/main/app_main.cpp, this file does
 * not call SetCertificationDeclaration() — enabling
 * CONFIG_ENABLE_SET_CERT_DECLARATION_API here currently has no effect. */

static const char *TAG = "app_main";
uint16_t occupancy_endpoint_id = 0;

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "Interface IP Address changed");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        break;

    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
        break;

    case chip::DeviceLayer::DeviceEventType::kBLEDeinitialized:
        ESP_LOGI(TAG, "BLE deinitialized and memory reclaimed");
        break;

    default:
        break;
    }
}

// This callback is invoked when clients interact with the Identify Cluster.
static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}

// This device only reports the Occupancy attribute; there is no physical
// actuator to drive when a controller reads/writes an attribute here, so
// this callback is intentionally a no-op.
static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    return ESP_OK;
}

extern "C" void app_main()
{
    esp_err_t err = ESP_OK;

    /* Initialize the ESP NVS layer */
    nvs_flash_init();

    /* Initialize drivers */
    app_driver_handle_t occupancy_handle = app_driver_occupancy_sensor_init();
    ABORT_APP_ON_FAILURE(occupancy_handle != nullptr, ESP_LOGE(TAG, "Failed to initialize occupancy sensor driver"));

    app_driver_handle_t button_handle = app_driver_button_init();
    app_reset_button_register(button_handle);

    /* Create a Matter node and add the mandatory Root Node device type on endpoint 0 */
    node::config_t node_config;
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    ABORT_APP_ON_FAILURE(node != nullptr, ESP_LOGE(TAG, "Failed to create Matter node"));

    occupancy_sensor::config_t occupancy_config;
    occupancy_config.occupancy_sensing.occupancy_sensor_type = (uint8_t)OccupancySensing::OccupancySensorTypeEnum::kPir;
    occupancy_config.occupancy_sensing.occupancy_sensor_type_bitmap = (uint8_t)OccupancySensing::OccupancySensorTypeBitmap::kPir;
    occupancy_config.occupancy_sensing.feature_flags =
        cluster::occupancy_sensing::feature::passive_infrared::get_id();

    endpoint_t *endpoint = occupancy_sensor::create(node, &occupancy_config, ENDPOINT_FLAG_NONE, occupancy_handle);
    ABORT_APP_ON_FAILURE(endpoint != nullptr, ESP_LOGE(TAG, "Failed to create occupancy_sensor endpoint"));

    uint16_t ep_id = endpoint::get_id(endpoint);
    ESP_LOGI(TAG, "Occupancy sensor created with endpoint_id %d", ep_id);

    /* Matter start */
    err = esp_matter::start(app_event_cb);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to start Matter, err:%d", err));

    /* Only now publish the endpoint id for the background PIR task to use,
     * and report the PIR's real level now that the endpoint/attribute store exists */
    occupancy_endpoint_id = ep_id;
    app_driver_occupancy_sensor_set_defaults(ep_id);

#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
    esp_matter::console::factoryreset_register_commands();
    esp_matter::console::attribute_register_commands();
    esp_matter::console::init();
#endif

    while (true) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
