#include <esp_log.h>
#include <mqtt_client.h>
#include "freertos/event_groups.h"


#include "mqtt_helper.h"
#include "cJSON.h"
#include "linked_list.h"

#define MQTT_BROKER_URL CONFIG_MQTT_BROKER_URL

static const char *TAG = "mqtt_helper";

const int MQTT_CONNECTED = BIT0;
const int RETAIN = 1;
static esp_mqtt_client_handle_t client;

static EventGroupHandle_t mqtt_event_group;

enum mqtt_qos { AT_MOST_ONCE, AT_LEAST_ONCE, EXACTLY_ONCE };

cJSON *root;
cJSON *time_string;
cJSON *mac_string;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event) {
  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
      ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
      xEventGroupSetBits(mqtt_event_group, MQTT_CONNECTED);  
      break;
    case MQTT_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
      break;
    case MQTT_EVENT_SUBSCRIBED:
      ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
      break;
    case MQTT_EVENT_PUBLISHED:
      ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
      delete(event->msg_id);
      break;
    case MQTT_EVENT_DATA:
      ESP_LOGI(TAG, "MQTT_EVENT_DATA");
      printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
      printf("DATA=%.*s\r\n", event->data_len, event->data);
      break;
    case MQTT_EVENT_ERROR:
      ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
      break;
    default:
      ESP_LOGI(TAG, "Other event id:%d", event->event_id);
      printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
      printf("DATA=%.*s\r\n", event->data_len, event->data);
      break;
  }
  return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
  mqtt_event_handler_cb(event_data);
}

void initialize_mqtt(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_BROKER_URL,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);

    esp_mqtt_client_start(client);

    mqtt_event_group = xEventGroupCreate();
}

void wait_for_mqtt_to_connect() {
    ESP_LOGI(TAG, "Waiting for MQTT client to connect");
	xEventGroupWaitBits(mqtt_event_group, MQTT_CONNECTED, false, true, portMAX_DELAY);
    xEventGroupClearBits(mqtt_event_group, MQTT_CONNECTED);
}

void wait_for_all_messages_to_be_published(void) {
    int retry = 0;
    const int retry_count = 50;
    while (!isEmpty() && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for all MQTT messages to be published... (%d/%d)", retry, retry_count);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    if(retry == retry_count) {
        ESP_LOGE(TAG, "Failed to wait for all MQTT messages to be published");
    }
}

void publish_message(char datetime[], char topic[], char key[], char payload[], char key2[], char payload2[]) {
    root = cJSON_CreateObject();
    // mac_string;
    // cars = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "datetime", cJSON_CreateString(datetime));
    
    uint8_t mac[6];
    char mac_as_text[18];
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac));
    sprintf(mac_as_text, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    cJSON_AddItemToObject(root, "mac", cJSON_CreateString(mac_as_text));

    cJSON_AddItemToObject(root, key, cJSON_CreateString(payload));
    cJSON_AddItemToObject(root, key2, cJSON_CreateString(payload2));

    char *json_as_string;

    json_as_string = cJSON_Print(root);
    ESP_LOGI(TAG, "\n%s", json_as_string);

    int msg_id = esp_mqtt_client_publish(client, topic, json_as_string, 0, EXACTLY_ONCE, RETAIN);
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
    insertFirst(msg_id, 0);

    free(json_as_string);

    /* free all objects under root and root itself */
    cJSON_Delete(root);
}