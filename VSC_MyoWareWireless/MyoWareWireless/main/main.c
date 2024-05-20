#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_bt.h"
#include "nvs_flash.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/adc.h"

#define SPP_TAG "SPP_SENDER"
// #define SPP_SERVER_NAME "SPP_SERVER"
#define DEVICE_NAME "SPP_SENDER"

#define STATUS_LED_PIN 13
#define SPP_DATA_LEN 1

static uint32_t spp_handle = 0;

// GPIO Output defines
#define GPIO_OUTPUT_IO_0    13
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0))

//GPIO Input defines
#define GPIO_INPUT_IO_0     35
#define GPIO_INPUT_IO_1     36
#define GPIO_INPUT_IO_2     39
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1) | (1ULL <<GPIO_INPUT_IO_2))

static const char* SPP_SERVER_NAME = "SPP_SERVER";
static const char remote_device_name[] = "SPP_RECEIVER";
static uint8_t peer_bdname_len;
static char peer_bdname[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
static esp_bd_addr_t peer_bd_addr;
static bool server_found = false;

static uint8_t spp_data[SPP_DATA_LEN];
static uint8_t *s_p_data = NULL; /* data pointer of spp_data */

static void esp_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

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

static bool get_name_from_eir(uint8_t* eir, char* bdname, uint8_t* bdname_len)
{
    uint8_t* local_bdname = NULL;
    uint8_t local_bdname_len = 0;
    
    if(!eir)
    {
        return false;
    }

    // Extract complete local name
    local_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &local_bdname_len);
    if(!local_bdname)
    {
        // if failed extrtact short local name
        local_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &local_bdname_len);
    }

    if(local_bdname)
    {
        // If name is longer than max set len to max
        if(local_bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN)
        {
            local_bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
        }

        // Copy the Local Name and add terminating null
        if(local_bdname)
        {
            memcpy(bdname, local_bdname, local_bdname_len);
            bdname[local_bdname_len] = '\0';
        }

        // Sabe name len
        if(bdname_len)
        {
            *bdname_len = local_bdname_len;
        }
        return true;
    }
    return false;
}

// ESP Serial Port Profile (SSP) Callback Funktion TODO: add comments
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {

    uint8_t i = 0;
    char bda_str[18] = {0};

    switch (event) {
        case ESP_SPP_INIT_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");
            if (param->init.status == ESP_SPP_SUCCESS)
            {
                esp_bt_dev_set_device_name(DEVICE_NAME);
                esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
                esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 30, 0);
            }
            else
            {
                ESP_LOGE(SPP_TAG, "ESP_SPP_INIT_EVT status:%d", param->init.status);
            }
            break;
        case ESP_SPP_DISCOVERY_COMP_EVT:
            // Once the server is discovered connect
            ESP_LOGI(SPP_TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
            if (param->disc_comp.status == ESP_SPP_SUCCESS) {
                ESP_LOGI(SPP_TAG, "Discovery complete, connecting...");
                esp_spp_connect(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_MASTER, param->disc_comp.scn[0], peer_bd_addr);
            } else {
                ESP_LOGE(SPP_TAG, "Discovery failed: %d", param->disc_comp.status);
            }
            break;
        case ESP_SPP_OPEN_EVT:
            if (param->open.status == ESP_SPP_SUCCESS) {
                ESP_LOGI(SPP_TAG, "ESP_SPP_OPEN_EVT handle:%"PRIu32" rem_bda:[%s]", param->open.handle,
                        bda2str(param->open.rem_bda, bda_str, sizeof(bda_str)));
                spp_handle = param->start.handle;
                /*   Start to write the first data packet */
                esp_spp_write(param->open.handle, SPP_DATA_LEN, spp_data);
                s_p_data = spp_data;
              
                
            } else {
                ESP_LOGE(SPP_TAG, "ESP_SPP_OPEN_EVT status:%d", param->open.status);
            }
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
            //spp_handle = param->open.handle;
            break;
        case ESP_SPP_CL_INIT_EVT:
            if (param->cl_init.status == ESP_SPP_SUCCESS) {
                ESP_LOGI(SPP_TAG, "ESP_SPP_CL_INIT_EVT handle:%"PRIu32" sec_id:%d", param->cl_init.handle, param->cl_init.sec_id);
            } else {
                ESP_LOGE(SPP_TAG, "ESP_SPP_CL_INIT_EVT status:%d", param->cl_init.status);
            }
            break;
        case ESP_SPP_WRITE_EVT:
            if (param->write.status == ESP_SPP_SUCCESS) {
                if (s_p_data + param->write.len == spp_data + SPP_DATA_LEN) {
                    /* Means the previous data packet be sent completely, send a new data packet */
                    s_p_data = spp_data;
                } else {
                    /*
                    * Means the previous data packet only be sent partially due to the lower layer congestion, resend the
                    * remainning data.
                    */
                    s_p_data += param->write.len;
                }
            }
            else
            {
                ESP_LOGE(SPP_TAG, "ESP_SPP_WRITE_EVT status:%d", param->write.status);
            } 
        
            break;

        default:
            break;
    }
}

static void esp_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event) {
    case ESP_BT_GAP_DISC_RES_EVT: {
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_DISC_RES_EVT");
        esp_log_buffer_hex(SPP_TAG, param->disc_res.bda, ESP_BD_ADDR_LEN);

        // Find the target peer divce name in the EIR data
        for(uint8_t i = 0; i < param->disc_res.num_prop; i++)
        {
            // Check type and get name from eir
            if(param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_EIR
            && get_name_from_eir(param->disc_res.prop[i].val, peer_bdname, &peer_bdname_len))
            {
                esp_log_buffer_char(SPP_TAG, peer_bdname, peer_bdname_len);

                // Ensure local and remote device names equal
                if(strlen(remote_device_name) == peer_bdname_len
                && strncmp(peer_bdname, remote_device_name, peer_bdname_len) == 0)
                {
                    ESP_LOGI(SPP_TAG, "............Device Found Starting SPP Discovery................");
                    server_found = true;
                    // Copy the bd addr
                    memcpy(peer_bd_addr, param->disc_res.bda, ESP_BD_ADDR_LEN);
                    // Have found target peer device, cancel discovery and start SPP discovery
                    esp_bt_gap_cancel_discovery();
                    esp_spp_start_discovery(peer_bd_addr);

                }
            }
        }
        break;
    }
    case ESP_BT_GAP_AUTH_CMPL_EVT:{
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(SPP_TAG, "authentication success: %s", param->auth_cmpl.device_name);
            ESP_LOG_BUFFER_HEX(SPP_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
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
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED && !server_found) {
            ESP_LOGI(SPP_TAG, "Discovery stopped, restarting discovery...");
            ESP_ERROR_CHECK(esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0));
        }
        break;
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %"PRIu32, param->cfm_req.num_val);
        ESP_LOGW(SPP_TAG, "To confirm the value, type `spp ok;`");
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%"PRIu32, param->key_notif.passkey);
        ESP_LOGW(SPP_TAG, "Waiting response...");
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        ESP_LOGW(SPP_TAG, "To input the key, type `spp key xxxxxx;`");
        break;
    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_MODE_CHG_EVT mode:%d", param->mode_chg.mode);
        break;
    
    default:
        break;
    }
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
    gpio_set_level(GPIO_OUTPUT_IO_0, 1);


    // Initialize Bluetooth controller:
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
    // Set the Bluetooth config
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    // Enable the Bluetooth Controller
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    // Init and enable Bluedroid - responsible for managing Bluetooth protocols
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bluedroid_init_with_cfg(&bluedroid_cfg));
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_bt_gap_register_callback(esp_gap_cb));

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

    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0);

    gpio_set_level(GPIO_OUTPUT_IO_0, 0);

       /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);

    ESP_LOGI(SPP_TAG, "Own address:[%s]", bda2str((uint8_t *)esp_bt_dev_get_address(), bda_str, sizeof(bda_str)));
    // Main loop to read and send ADC values
    while (1) {
        spp_data[0] = adc1_get_raw(ADC1_CHANNEL_0);
        
        if (server_found) {
            esp_spp_write(spp_handle, SPP_DATA_LEN, spp_data);
            // Toggle Status LED each time message is sent
            ESP_LOGI(SPP_TAG, "Analog value: %d", spp_data[0]);
            //ESP_LOGI(SPP_TAG, "handle: %lu", spp_handle);
            gpio_set_level(GPIO_OUTPUT_IO_0, !(gpio_get_level(GPIO_OUTPUT_IO_0)));
        }
        else
        {
            ESP_LOGI(SPP_TAG,"Not Connected .............");
            // Retry discovery
            //esp_spp_start_discovery(SPP_SERVER_NAME);
            vTaskDelay(pdMS_TO_TICKS(1000)); // Send every 1 second
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