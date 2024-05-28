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
#include "iot_servo.h"
#include "esp_timer.h"

// Bluetooth Defines:
#define SPP_TAG "SPP_RECEIVER"
#define DEVICE_NAME "SPP_RECEIVER"
#define SERVER_NAME "SPP_SERVER"

//LEDC Defines: TODO: use this to change channel in set angle and use the gpio in the channel init 
#define LEDC_PINKY    (15)
#define LEDC_RING     (2)
#define LEDC_MIDDLE   (0)
#define LEDC_POINTER  (5)
#define LEDC_THUMB    (18)

//GPIO Defines:
#define GPIO_PINKY    15
#define GPIO_RING     2
#define GPIO_MIDDLE   0
#define GPIO_POINTER  5
#define GPIO_THUMB    18
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_PINKY) | (1ULL<<GPIO_RING) | (1ULL<<GPIO_MIDDLE) | (1ULL<<GPIO_POINTER) | (1ULL<<GPIO_THUMB))

//Servo Defines:
#define SERVO_PINKY   15
#define SERVO_RING    2
#define SERVO_MIDDLE  0
#define SERVO_POINTER 5
#define SERVO_THUMB   18

//Threshold
#define THRESHOLD_VAL 10

void handle_data(uint8_t* data, uint16_t len)
{
    int val = data[0];
    if( val >THRESHOLD_VAL )
    {
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, 180);
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 1, 180);
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 2, 180);
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 3, 180);
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 4, 180);

    }
    else if (val < THRESHOLD_VAL)
    {
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, 0);
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 1, 0);
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 2, 0);
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 3, 0);
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 4, 5);
    }
    ESP_LOGI(SPP_TAG, " %d", val);
}

static char *bda2str(uint8_t * bda, char *str, size_t size)
{
    if (bda == NULL || str == NULL || size < 18) {
        return NULL;
    }

    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
}

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    char bda_str[18] = {0};
    switch (event) {
    case ESP_SPP_INIT_EVT:
        if(param->init.status == ESP_SPP_SUCCESS)
        {
            ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");  
            esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, SERVER_NAME);   
        }        
        else
        {
                ESP_LOGE(SPP_TAG, "ESP_SPP_INIT_EVT status:%d", param->init.status);
        }
        break;  
    case ESP_SPP_START_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_START_EVT");
        if(param->start.status == ESP_SPP_SUCCESS)
        {
            ESP_LOGI(SPP_TAG, "ESP_SPP_START_EVT handle:%"PRIu32" sec_id:%d scn:%d", param->start.handle, param->start.sec_id,
                param->start.scn);
            
            esp_bt_dev_set_device_name(DEVICE_NAME);
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        }
        else
        {
            ESP_LOGE(SPP_TAG, "ESP_SPP_START_EVT status:%d", param->start.status);
        }
        break;
    case ESP_SPP_DATA_IND_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_DATA_IND_EVT");// len=%d handle=%d", param->data_ind.len, param->data_ind.handle);
        //char incoming_data[param->data_ind.len + 1];
        //memcpy(incoming_data, param->data_ind.data, param->data_ind.len);
        //incoming_data[param->data_ind.len] = '\0';
        handle_data(param->data_ind.data,param->data_ind.len);
        break;
    case ESP_SPP_CONG_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CONG_EVT");
        break;
    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_OPEN_EVT status:%d handle:%"PRIu32", rem_bda:[%s]", param->srv_open.status,
                param->srv_open.handle, bda2str(param->srv_open.rem_bda, bda_str, sizeof(bda_str)));
                break;
    case ESP_SPP_SRV_STOP_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_STOP_EVT");
        break;
    case ESP_SPP_UNINIT_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_UNINIT_EVT");
        break;
    default:
        break;
    }
}

void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    char bda_str[18] = {0};
 
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT:{
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(SPP_TAG, "authentication success: %s bda:[%s]", param->auth_cmpl.device_name,
                     bda2str(param->auth_cmpl.bda, bda_str, sizeof(bda_str)));
        } else {
            ESP_LOGE(SPP_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }
    case ESP_BT_GAP_PIN_REQ_EVT:{
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
        if (param->pin_req.min_16_digit) {
            ESP_LOGI(SPP_TAG, "Input pin code: 0000 0000 0000 0000");
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
            ESP_LOGI(SPP_TAG, "Input pin code: 1234");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '2';
            pin_code[2] = '3';
            pin_code[3] = '4';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    }
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %"PRIu32, param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%"PRIu32, param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_MODE_CHG_EVT mode:%d bda:[%s]", param->mode_chg.mode,
                 bda2str(param->mode_chg.bda, bda_str, sizeof(bda_str)));
        break;

    default: {
        ESP_LOGI(SPP_TAG, "event: %d", event);
        break;
    }
    }
    return;
}

// LEDC Init Function 
static void ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .freq_hz          = 4000,  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration for the Pinky
    ledc_channel_config_t ledc_channel_pinky = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_PINKY,            // GPIO NUMBER
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_pinky));
    ESP_LOGI(SPP_TAG, "Pinky Init");

    // Prepare and then apply the LEDC PWM channel configuration for the Ring
    ledc_channel_config_t ledc_channel_ring = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_1,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_RING,            // GPIO NUMBER
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_ring));
    ESP_LOGI(SPP_TAG, "Ring init");

    // Prepare and then apply the LEDC PWM channel configuration for the middle
    ledc_channel_config_t ledc_channel_middle = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_2,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_MIDDLE,            // GPIO NUMBER
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_middle));
    ESP_LOGI(SPP_TAG, "Middle init");

    // Prepare and then apply the LEDC PWM channel configuration for the pointer
    ledc_channel_config_t ledc_channel_pointer = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_3,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_POINTER,            // GPIO NUMBER
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_pointer));
    ESP_LOGI(SPP_TAG, "Pointer init");

    // Prepare and then apply the LEDC PWM channel configuration for the thumb
    ledc_channel_config_t ledc_channel_thumb = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_4,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_THUMB,            // GPIO NUMBER
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_thumb));
    ESP_LOGI(SPP_TAG, "Thumn init");
}

static void gpio_init(void)
{
    gpio_config_t gpio_cfg ={
        .intr_type       = GPIO_INTR_DISABLE,
        .mode            = GPIO_MODE_OUTPUT,
        .pin_bit_mask    = GPIO_OUTPUT_PIN_SEL,
        .pull_down_en    = 0,
        .pull_up_en      = 0,
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg));
}

static void servo_init(void)
{
    servo_config_t servo_cfg = {
        .max_angle = 180,
        .min_width_us = 500,
        .max_width_us = 2500,
        .freq = 50,
        .timer_number = LEDC_TIMER_0,
        .channels = {
            .servo_pin = {
                SERVO_PINKY,
                SERVO_RING,
                SERVO_MIDDLE,
                SERVO_POINTER,
                SERVO_THUMB,
            },
            .ch = {
                LEDC_CHANNEL_0,
                LEDC_CHANNEL_1,
                LEDC_CHANNEL_2,
                LEDC_CHANNEL_3,
                LEDC_CHANNEL_4,
            },
        },
        .channel_number = 1,
    };

    ESP_ERROR_CHECK(iot_servo_init(LEDC_LOW_SPEED_MODE, &servo_cfg));

    // Set all servos to 0 as starting position
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, 0);
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 1, 0);
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 2, 0);
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 3, 0);
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 4, 5);
}

void app_main(void)
{
    char bda_str[18] = {0};
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    // Init the GPIO
    gpio_init();
    //Init the LEDC
    ledc_init();
    // Init the servo
    servo_init();

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

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
    esp_spp_cfg.enable_l2cap_ertm = true; 
    esp_spp_cfg.tx_buffer_size = 0;
    ESP_ERROR_CHECK(esp_spp_register_callback(esp_spp_cb));
    ESP_ERROR_CHECK(esp_spp_enhanced_init(&esp_spp_cfg));

    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));

    /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);

    ESP_LOGI(SPP_TAG, "Own address:[%s]", bda2str((uint8_t *)esp_bt_dev_get_address(), bda_str, sizeof(bda_str)));

    // Main loop
    while (1) {
    
        vTaskDelay(pdMS_TO_TICKS(50)); // Run every 1 second
/*
        float angle = 100.0f;

        // Set angle to 100 degree
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, angle);

        for(uint8_t i = 0; i <= 180; i = i +20)
        {
            iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, i);
            ESP_LOGI(SPP_TAG,"...Changing Angle...");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

         iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, 0);
         */

    }
}
