#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "esp_bt.h"
#include "nvs_flash.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"
#include "esp_log.h"

#define SPP_TAG "ESP32_SPP_RECEIVER"

void handle_data(uint8_t* data, uint16_t len)
{
    for(uint8_t i = 0; i < len; i++)
    {
        ESP_LOGI(SPP_TAG, "Data[%d] recieved: %d",i, data[i]);
    }
}


static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    switch (event) {
        case ESP_SPP_INIT_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");
            esp_bt_dev_set_device_name("ESP32_SPP_RECEIVER");
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, "SPP_SERVER");
            break;
        case ESP_SPP_START_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_START_EVT");
            break;
        case ESP_SPP_DATA_IND_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_DATA_IND_EVT");// len=%d handle=%d", param->data_ind.len, param->data_ind.handle);
            //char incoming_data[param->data_ind.len + 1];
            //memcpy(incoming_data, param->data_ind.data, param->data_ind.len);
            //incoming_data[param->data_ind.len] = '\0';
            handle_data(param->data_ind.data,param->data_ind.len);
            ESP_LOGI(SPP_TAG, "Received data:");
            break;
        case ESP_SPP_CONG_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_CONG_EVT");
            break;
        default:
            break;
    }
}

void app_main(void)
{

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    // Initialize and configure the Bluetooth controller
    //ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bluedroid_init_with_cfg(&bluedroid_cfg));
    ESP_ERROR_CHECK(esp_bluedroid_enable()); 

    // Register the SPP callback functioneb
    esp_spp_cfg_t esp_spp_cfg = {};
    esp_spp_cfg.mode = ESP_SPP_MODE_CB;
    esp_spp_cfg.tx_buffer_size = 15;
    ESP_ERROR_CHECK(esp_spp_register_callback(esp_spp_cb));
    ESP_ERROR_CHECK(esp_spp_enhanced_init(&esp_spp_cfg));

    // Main loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Run every 1 second
        //printf("test");
    }
}
