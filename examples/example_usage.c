#include "mqtt.h"
#include <stdio.h>

void on_message(const char* topic, const uint8_t* payload, size_t len, void* user_data) {
    printf("Received: topic=%s, len=%zu\n", topic, len);
}

void mqtt_example_task(void* arg) {
    mqtt_config_t config = {
        .host = "broker.emqx.io",
        .port = 1883,
        .client_id = "mqtt_client_001",
        .username = NULL,
        .password = NULL,
        .keepalive = 60,
        .msg_cb = on_message,
        .user_data = NULL
    };
    
    mqtt_client_t* client = mqtt_client_create(&config);
    if (!client) {
        printf("Failed to create client\n");
        return;
    }
    
    if (mqtt_client_connect(client) != 0) {
        printf("Failed to connect\n");
        mqtt_client_destroy(client);
        return;
    }
    
    printf("Connected to broker\n");
    
    mqtt_client_subscribe(client, "test/topic", 0);
    
    const char* msg = "Hello MQTT";
    mqtt_client_publish(client, "test/topic", (uint8_t*)msg, strlen(msg), 0);
    
    while (1) {
        mqtt_client_loop(client);
        mqtt_os_get()->sleep_ms(1000);
    }
    
    mqtt_client_disconnect(client);
    mqtt_client_destroy(client);
}
