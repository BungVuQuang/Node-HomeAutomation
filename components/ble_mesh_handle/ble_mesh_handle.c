/**
 ******************************************************************************
 * @file		ble_mesh.c
 * @author	Vu Quang Bung
 * @date		26 June 2023
 * Copyright (C) 2023 - Bung.VQ
 ******************************************************************************
 **/
/*-----------------------------------------------------------------------------*/
/* Header inclusions */
/*-----------------------------------------------------------------------------*/
#include "ble_mesh_handle.h"
// #include "ble_mesh_example_init.h"
// #include "ble_mesh_example_nvs.h"
/*-----------------------------------------------------------------------------*/
/* Local Constant definitions */
/*-----------------------------------------------------------------------------*/
const char *TAG_BLE = "ble_mesh";
/*-----------------------------------------------------------------------------*/
/* Local Macro definitions */
/*-----------------------------------------------------------------------------*/
#define TAG "WSN_SENSOR"
#define TAG_BACKUP "BACKUP_DATA"
#define CID_ESP 0x02E5
#define TEMPERATURE_GROUP_ADDRESS 0xC001
#define ESP_BLE_MESH_VND_MODEL_ID_CLIENT 0x0000
#define ESP_BLE_MESH_VND_MODEL_ID_SERVER 0x0001

#define ESP_BLE_MESH_VND_MODEL_OP_SEND ESP_BLE_MESH_MODEL_OP_3(0x00, CID_ESP)
#define ESP_BLE_MESH_VND_MODEL_OP_STATUS ESP_BLE_MESH_MODEL_OP_3(0x01, CID_ESP)

nvs_handle_t NVS_HANDLE;

static esp_ble_mesh_cfg_srv_t config_server = {
    .relay = ESP_BLE_MESH_RELAY_ENABLED,
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
#if defined(CONFIG_BLE_MESH_FRIEND)
    .friend_state = ESP_BLE_MESH_FRIEND_ENABLED,
#else
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BLE_MESH_GATT_PROXY_SERVER)
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
#else
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
    .default_ttl = 7,
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
};
#define SENSOR_PROPERTY_ID_0 0x0056 /* Present Indoor Ambient Temperature */
#define SENSOR_PROPERTY_ID_1 0x005B /* Present Outdoor Ambient Temperature */
static uint8_t net_buf_data_sensor_data_0[10];
static uint8_t net_buf_data_sensor_data_1[10];

static struct net_buf_simple sensor_data_0 = {
    .data = net_buf_data_sensor_data_0,
    .len = 0,
    .size = 10,
    .__buf = net_buf_data_sensor_data_0,
};

static struct net_buf_simple sensor_data_1 = {
    .data = net_buf_data_sensor_data_1,
    .len = 0,
    .size = 10,
    .__buf = net_buf_data_sensor_data_1,
};
#define SENSOR_POSITIVE_TOLERANCE ESP_BLE_MESH_SENSOR_UNSPECIFIED_POS_TOLERANCE
#define SENSOR_NEGATIVE_TOLERANCE ESP_BLE_MESH_SENSOR_UNSPECIFIED_NEG_TOLERANCE
#define SENSOR_SAMPLE_FUNCTION ESP_BLE_MESH_SAMPLE_FUNC_UNSPECIFIED
#define SENSOR_MEASURE_PERIOD ESP_BLE_MESH_SENSOR_NOT_APPL_MEASURE_PERIOD
#define SENSOR_UPDATE_INTERVAL ESP_BLE_MESH_SENSOR_NOT_APPL_UPDATE_INTERVAL
static esp_ble_mesh_sensor_state_t sensor_states[2] = {

    [0] = {

        .sensor_property_id = SENSOR_PROPERTY_ID_0,
        .descriptor.positive_tolerance = SENSOR_POSITIVE_TOLERANCE,
        .descriptor.negative_tolerance = SENSOR_NEGATIVE_TOLERANCE,
        .descriptor.sampling_function = SENSOR_SAMPLE_FUNCTION,
        .descriptor.measure_period = SENSOR_MEASURE_PERIOD,
        .descriptor.update_interval = SENSOR_UPDATE_INTERVAL,
        .sensor_data.format = ESP_BLE_MESH_SENSOR_DATA_FORMAT_A,
        .sensor_data.length = 0, /* 0 represents the length is 1 */
        .sensor_data.raw_value = &sensor_data_0,
    },
    [1] = {
        .sensor_property_id = SENSOR_PROPERTY_ID_1,
        .descriptor.positive_tolerance = SENSOR_POSITIVE_TOLERANCE,
        .descriptor.negative_tolerance = SENSOR_NEGATIVE_TOLERANCE,
        .descriptor.sampling_function = SENSOR_SAMPLE_FUNCTION,
        .descriptor.measure_period = SENSOR_MEASURE_PERIOD,
        .descriptor.update_interval = SENSOR_UPDATE_INTERVAL,
        .sensor_data.format = ESP_BLE_MESH_SENSOR_DATA_FORMAT_A,
        .sensor_data.length = 0, /* 0 represents the length is 1 */
        .sensor_data.raw_value = &sensor_data_1,
    },
};

/* 20 octets is large enough to hold two Sensor Descriptor state values. */
static uint8_t net_buf_data_bt_mesh_pub_msg_sensor_pub[20];
static struct net_buf_simple bt_mesh_pub_msg_sensor_pub = {
    .data = net_buf_data_bt_mesh_pub_msg_sensor_pub,
    .len = 10,
    .size = 20,
    .__buf = net_buf_data_bt_mesh_pub_msg_sensor_pub,
};
static esp_ble_mesh_model_pub_t sensor_pub = {
    .update = (uint32_t)NULL,
    .msg = &bt_mesh_pub_msg_sensor_pub,
    .dev_role = ROLE_NODE,
};
static esp_ble_mesh_sensor_srv_t sensor_server = {
    .rsp_ctrl.get_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
    .rsp_ctrl.set_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
    .state_count = ARRAY_SIZE(sensor_states),
    .states = sensor_states,
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(sensor_setup_pub, 20, ROLE_NODE);
static esp_ble_mesh_sensor_setup_srv_t sensor_setup_server = {
    .rsp_ctrl.get_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
    .rsp_ctrl.set_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
    .state_count = ARRAY_SIZE(sensor_states),
    .states = sensor_states,
};

static esp_ble_mesh_model_t root_models[] = {
    // model sensor server
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_SENSOR_SRV(&sensor_pub, &sensor_server),
    ESP_BLE_MESH_MODEL_SENSOR_SETUP_SRV(&sensor_setup_pub, &sensor_setup_server),
};

static esp_ble_mesh_model_op_t vnd_op[] = {
    // model vendor sever
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_SEND, 2),
    ESP_BLE_MESH_MODEL_OP_END,
};

static esp_ble_mesh_model_t vnd_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_SERVER,
                              vnd_op, NULL, NULL),
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, vnd_models),
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .elements = elements,
    .element_count = ARRAY_SIZE(elements),
};

static esp_ble_mesh_prov_t provision = {
    .uuid = dev_uuid,
};

// store = {
//     .net_idx = ESP_BLE_MESH_KEY_UNUSED,
//     .app_idx = ESP_BLE_MESH_KEY_UNUSED,
//     .task_send_check = 0,
//     .ack_check = 1,
//     .tid = 0x0,
// };

/**
 *
 *  @brief Hàm này lưu lại thông tin và trạng thái của sensor node
 *
 *  @return None
 */
void mesh_example_info_store(void)
{
    ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY, &store, sizeof(store));
}
/**
 *  @brief Hàm này khôi phục lại thông tin và trạng thái của sensor node
 *
 *  @return None
 */
void mesh_example_info_restore(void)
{
    esp_err_t err = ESP_OK;
    bool exist = false;

    err = ble_mesh_nvs_restore(NVS_HANDLE, NVS_KEY, &store, sizeof(store), &exist);
    if (err != ESP_OK)
    {
        return;
    }
    if (store.task_send_check == 1)
    {
        // xEventGroupSetBits(rx_task, START_CONNECTED_BIT);
    }
    if (exist)
    {
        ESP_LOGI(TAG, "Restore, net_idx 0x%04x, app_idx 0x%04x, tid 0x%02x",
                 store.net_idx, store.app_idx, store.tid);
    }
}

// static void example_change_led_state(esp_ble_mesh_model_t *model,
//                                      esp_ble_mesh_msg_ctx_t *ctx)
// {
//     uint16_t primary_addr = esp_ble_mesh_get_primary_element_address();
//     uint8_t elem_count = esp_ble_mesh_get_element_count();
//     uint8_t i;

//     if (ESP_BLE_MESH_ADDR_IS_UNICAST(ctx->recv_dst))
//     {
//         for (i = 0; i < elem_count; i++)
//         {
//             if (ctx->recv_dst == (primary_addr + i))
//             {
//                 printf("Đã vào ESP_BLE_MESH_ADDR_IS_UNICAST\n");
//             }
//         }
//     }
//     else if (ESP_BLE_MESH_ADDR_IS_GROUP(ctx->recv_dst))
//     {
//         if (esp_ble_mesh_is_model_subscribed_to_group(model, ctx->recv_dst))
//         {
//             // led = &led_state[model->element->element_addr - primary_addr];
//             // board_led_operation(led->pin, onoff);
//             printf("Đã vào  if (esp_ble_mesh_is_model_subscribed_to_group(model, ctx->recv_dst))\n");
//         }
//     }
//     else if (ctx->recv_dst == 0xFFFF)
//     {
//         printf("Đã vào ctx->recv_dst == 0xFFFF\n");
//     }
// }

static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index)
{
    ESP_LOGI(TAG, "net_idx 0x%03x, addr 0x%04x", net_idx, addr);
    ESP_LOGI(TAG, "flags 0x%02x, iv_index 0x%08x", flags, iv_index);
}
/**
 *  @brief Hàm này được gọi lại KHI Gateway gửi yêu cầu cấp phép cho Sensor NODE
 *
 *  @param[in] event Event của callBack
 *  @param[in] param Thông tin của sensor node
 *  @return None
 */
static void example_ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                             esp_ble_mesh_prov_cb_param_t *param)
{
    switch (event)
    {
    case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT: // Event khi đăng ký callback xong
        // rx_task = xEventGroupCreate();
        mesh_example_info_restore(); // Khôi phục lại thông tin của gateway

        ESP_LOGI(TAG, "ESP_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code %d", param->prov_register_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT: // Event báo đã phát bản tin quảng cáo
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT, err_code %d", param->node_prov_enable_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT: // Event báo Gateway đã Scan được ADV
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT, bearer %s",
                 param->node_prov_link_open.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT: // Đóng phát bản tin ADV
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT, bearer %s",
                 param->node_prov_link_close.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT: // Hoàn thành việc cấp phép
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT");
        prov_complete(param->node_prov_complete.net_idx, param->node_prov_complete.addr,
                      param->node_prov_complete.flags, param->node_prov_complete.iv_index);
        break;
    case ESP_BLE_MESH_NODE_PROV_RESET_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_RESET_EVT");
        break;
    case ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT, err_code %d", param->node_set_unprov_dev_name_comp.err_code);
        break;
    default:
        break;
    }
}
/**
 *  @brief Hàm này được gọi lại nếu Sensor node Bind App key với các Model thành công
 *
 *  @param[in] event Event của callBack
 *  @param[in] param Thông tin của sensor node
 *  @return None
 */
static void example_ble_mesh_config_server_cb(esp_ble_mesh_cfg_server_cb_event_t event,
                                              esp_ble_mesh_cfg_server_cb_param_t *param)
{
    if (event == ESP_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT)
    {
        switch (param->ctx.recv_op)
        {
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD");
            ESP_LOGI(TAG, "net_idx 0x%04x, app_idx 0x%04x",
                     param->value.state_change.appkey_add.net_idx,
                     param->value.state_change.appkey_add.app_idx);
            ESP_LOG_BUFFER_HEX("AppKey", param->value.state_change.appkey_add.app_key, 16);
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND");
            ESP_LOGI(TAG, "elem_addr 0x%04x, app_idx 0x%04x, cid 0x%04x, mod_id 0x%04x",
                     param->value.state_change.mod_app_bind.element_addr,
                     param->value.state_change.mod_app_bind.app_idx,
                     param->value.state_change.mod_app_bind.company_id,
                     param->value.state_change.mod_app_bind.model_id);
            store.app_idx = param->value.state_change.mod_app_bind.app_idx;
            mesh_example_info_store(); /* Store proper mesh example info */ // tạo eventGroup

            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD");
            ESP_LOGI(TAG, "elem_addr 0x%04x, sub_addr 0x%04x, cid 0x%04x, mod_id 0x%04x",
                     param->value.state_change.mod_sub_add.element_addr,
                     param->value.state_change.mod_sub_add.sub_addr,
                     param->value.state_change.mod_sub_add.company_id,
                     param->value.state_change.mod_sub_add.model_id);
            break;
        default:
            break;
        }
    }
}
/**
 *  @brief Hàm này được gọi lại nếu như Gateway request GetData Sensor Model đến Node
 *
 *  @param[in] event Event của callBack
 *  @param[in] param Thông tin của Gateway
 *  @return None
 */
static void example_ble_mesh_sensor_server_cb(esp_ble_mesh_sensor_server_cb_event_t event,
                                              esp_ble_mesh_sensor_server_cb_param_t *param)
{
    ESP_LOGI(TAG, "Sensor server, event %d, src 0x%04x, dst 0x%04x, model_id 0x%04x",
             event, param->ctx.addr, param->ctx.recv_dst, param->model->model_id);
    esp_ble_mesh_sensor_server_cb_param_t *param_temp = (esp_ble_mesh_sensor_server_cb_param_t *)param; // trở tới dữ liệu của gateway
    switch (event)
    {
    case ESP_BLE_MESH_SENSOR_SERVER_RECV_GET_MSG_EVT: // Event nhận thông tin model từ gateway
        switch (param->ctx.recv_op)
        {
        case ESP_BLE_MESH_MODEL_OP_SENSOR_GET:
            ESP_LOGI(TAG, "Gui data lan thu ESP_BLE_MESH_MODEL_OP_SENSOR_GET");
            // example_change_led_state(param->model, &param->ctx);
            store.param_po = *param_temp;
            store.task_send_check = 1;
            mesh_example_info_store(); // lưu lại cờ check đã nhận thông tin lần đầu của gateway
            esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                               ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, 3, (uint8_t *)"ack");
            // xEventGroupSetBits(rx_task, START_CONNECTED_BIT); // xét EventBit START_CONNECTED_BIT để Task DS18B20 UNBLOCKED
            break;
        default:
            ESP_LOGE(TAG, "Unknown Sensor Get opcode 0x%04x", param->ctx.recv_op);
            return;
        }
        break;
    case ESP_BLE_MESH_SENSOR_SERVER_RECV_SET_MSG_EVT:
        switch (param->ctx.recv_op)
        {
        default:
            ESP_LOGE(TAG, "Unknown Sensor Set opcode 0x%04x", param->ctx.recv_op);
            break;
        }
        break;
    default:
        ESP_LOGE(TAG, "Unknown Sensor Server event %d", event);
        break;
    }
}

/**
 *  @brief Hàm này được gọi lại nếu như có Event liên quan đến truyền nhận tin nhắn giữa các Node và Gateway
 *
 *  @param[in] event Event của callBack
 *  @param[in] param Thông tin của Gateway
 *  @return None
 */
static void example_ble_mesh_custom_model_cb(esp_ble_mesh_model_cb_event_t event,
                                             esp_ble_mesh_model_cb_param_t *param)
{
    esp_ble_mesh_model_cb_param_t *param_temp = (esp_ble_mesh_model_cb_param_t *)param;
    char temp[5];
    char *ptr;
    char res[5] = "ACK";
    switch (event)
    {

    case ESP_BLE_MESH_MODEL_OPERATION_EVT: // Event nhận được tin nhắn từ Gateway
        // esp_timer_stop(periodic_mesh_timer);
        sprintf(data_rx_mesh, "%.*s", param->model_operation.length, (char *)(param->model_operation.msg));
        esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                           ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(res), (uint8_t *)res);
        ESP_LOGI(TAG, "Sensor Data: %.*s\n", param->model_operation.length, (char *)(param->model_operation.msg));
        xEventGroupSetBits(s_mesh_network_event_group, MESH_MESSAGE_ARRIVE_BIT); // xoá Eventbit
        break;
    case ESP_BLE_MESH_MODEL_SEND_COMP_EVT: // Send message xong sẽ vào đây
        // xEventGroupSetBits(rx_task, START_RETRANSMIT_BIT);
        if (param->model_send_comp.err_code)
        {
            ESP_LOGE(TAG, "Failed to send message 0x%06x", param->model_send_comp.opcode);
            break;
        }
        break;
    case ESP_BLE_MESH_MODEL_PUBLISH_COMP_EVT:
    {
        // xEventGroupSetBits(rx_task, START_RETRANSMIT_BIT);
        sprintf(data_rx_mesh, "%.*s", param->model_operation.length, (char *)(param->model_operation.msg));
        // esp_timer_stop(periodic_mesh_timer);
        esp_ble_mesh_server_model_send_msg(store.param_po.model, &store.param_po.ctx,
                                           ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, strlen(res), (uint8_t *)res);
        xEventGroupSetBits(s_mesh_network_event_group, MESH_MESSAGE_ARRIVE_BIT); // xoá Eventbit

        ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_PUBLISH_COMP_EVT, err_code %d",
                 param->model_publish_comp.err_code);
        if (param->model_publish_comp.err_code)
        {
            ESP_LOGE(TAG, "Failed to publish message 0x%06x", param->model_publish_comp.err_code);
            break;
        }
        break;
    }
    default:
        break;
    }
}

/**
 *  @brief Khởi tạo BLE Mesh module và đăng ký các hàm callback xử lý event
 *
 *  @return None
 */
esp_err_t ble_mesh_init(void)
{
    esp_err_t err;

    esp_ble_mesh_register_prov_callback(example_ble_mesh_provisioning_cb);           // function callback xử lý event cấp phép của Node
    esp_ble_mesh_register_config_server_callback(example_ble_mesh_config_server_cb); // Register BLE Mesh Config Server Model callback.
    esp_ble_mesh_register_sensor_server_callback(example_ble_mesh_sensor_server_cb); // Register BLE Mesh Sensor Server Model callback
    esp_ble_mesh_register_custom_model_callback(example_ble_mesh_custom_model_cb);   // function callback được gọi khi

    esp_ble_mesh_init(&provision, &composition); // Initialize BLE Mesh module
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);
    esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT); // Phát các gói tin quảng cáo

    store.net_idx = ESP_BLE_MESH_KEY_UNUSED;
    store.app_idx = ESP_BLE_MESH_KEY_UNUSED;
    store.task_send_check = 0;
    store.ack_check = 1;
    store.tid = 0x0;
    return ESP_OK;
}
