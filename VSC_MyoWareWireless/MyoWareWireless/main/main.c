#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/adc.h"

#define SPP_TAG "ESP32_SPP_SENDER"
#define SPP_SERVER_NAME "SPP_SERVER"

#define STATUS_LED_PIN 13

static uint32_t spp_handle = 0;

// GPIO Output defines
#define GPIO_OUTPUT_IO_0    13
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0))

//GPIO Input defines
#define GPIO_INPUT_IO_0     35
#define GPIO_INPUT_IO_1     36
#define GPIO_INPUT_IO_2     39
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1) | (1ULL <<GPIO_INPUT_IO_2))



// ESP Serial Port Profile (SSP) Callback Funktion TODO: add comments
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    switch (event) {
        case ESP_SPP_INIT_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");
            esp_bt_dev_set_device_name("ESP32_SPP_SENDER");
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, SPP_SERVER_NAME);
            break;
        case ESP_SPP_START_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_START_EVT");
            break;
        case ESP_SPP_DATA_IND_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_DATA_IND_EVT");// len=%x handle=%x", param->data_ind.len, param->data_ind.handle);
            break;
        case ESP_SPP_CONG_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_CONG_EVT");
            break;
        case ESP_SPP_SRV_OPEN_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_OPEN_EVT");
            spp_handle = param->open.handle;
            break;
        default:
            break;
    }
}

void app_main(void)
{
    //zero-initialize the GPIO config structure.
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    // Configure output
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Initialize Bluetooth controller:
    // Set the Bluetooth config
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));

    // Enable the Bluetooth Controller
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    // Init and enable Bluedroid - responsible for managing Bluetooth protocols
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bluedroid_init_with_cfg(&bluedroid_cfg));
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // Register the SPP callback functioneb
    esp_spp_cfg_t esp_spp_cfg = {};
    esp_spp_cfg.mode = ESP_SPP_MODE_CB;
    esp_spp_cfg.tx_buffer_size = 15;
    ESP_ERROR_CHECK(esp_spp_register_callback(esp_spp_cb));
    ESP_ERROR_CHECK(esp_spp_enhanced_init(&esp_spp_cfg));

    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0);

    // Main loop to read and send ADC values
    while (1) {
        uint16_t analog_value = adc1_get_raw(ADC1_CHANNEL_0);
        ESP_LOGI(SPP_TAG, "Analog value: %d", analog_value);
        char message[20];
        snprintf(message, sizeof(message), "%d", analog_value);
        if (spp_handle != 0) {
            esp_spp_write(spp_handle, strlen(message), (uint8_t *)message);
            // Toggle Status LED each time message is sent
            gpio_set_level(GPIO_OUTPUT_IO_0, !(gpio_get_level(GPIO_OUTPUT_IO_0)));
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Send every 1 second
    }
    
}
/*
Program Structure Sender:
Setup: init pins: 
    ENV Pin: A3/39 (Input) ADC1_CH3
    Raw Pin: A4/36 (Input) ADC1_CH0
    Ref Pin: A5/35 (Input) ADC1_CH7
    Status Led: Pin 13 (Output) -> blink when not connected, high when connected 
1. Init Bluetooth controller
2. Init ADC
3. Connect Bluetooth to Reciever
4. Send ADC data 
*/

/*
Structure Reciever:
1. Init Bluetooth controller
3. Connect Bluetooth to Sender
4. 
*/