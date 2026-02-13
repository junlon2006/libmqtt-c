/**
 * @file demo.c
 * @brief MQTT client demo application
 * 
 * This demo shows how to use the libmqtt library to:
 * - Connect to an MQTT broker
 * - Subscribe to topics
 * - Publish messages
 * - Handle automatic reconnection
 */

#include "mqtt.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>

void mqtt_posix_init(void);
void mqtt_posix_net_init(void);

static volatile int running = 1;

void signal_handler(int sig) {
    running = 0;
}

void on_message(const char* topic, const uint8_t* payload, size_t len, void* user_data) {
    printf("[RECV] Topic: %s, Payload: ", topic);
    for (size_t i = 0; i < len; i++) {
        printf("%c", payload[i]);
    }
    printf("\n");
}

int main(void) {
    signal(SIGINT, signal_handler);
    
    printf("=== MQTT Client Demo ===\n");
    
    mqtt_posix_init();
    mqtt_posix_net_init();
    
    mqtt_client_t* client = NULL;
    
    mqtt_config_t config = {
        .host = "10.91.0.69",
        .port = 1883,
        .client_id = "libmqtt_test_client",
        .username = NULL,
        .password = NULL,
        .keepalive = 60,
        .clean_session = 1,
        .msg_cb = on_message,
        .user_data = NULL
    };
    
    printf("Creating MQTT client...\n");
    client = mqtt_client_create(&config);
    if (!client) {
        printf("Failed to create client\n");
        return -1;
    }
    
    printf("Connecting to broker %s:%d...\n", config.host, config.port);
    if (mqtt_client_connect(client) != 0) {
        printf("Failed to connect to broker\n");
        mqtt_client_destroy(client);
        return -1;
    }
    
    printf("Connected successfully!\n");
    
    const char* sub_topic = "iot/v1/s2d/AC7A512C5AB200711AE9CA90A2DCDC17/reminder";
    const char* pub_topic = "test/demo";
    
    printf("Subscribing to topic: %s...\n", sub_topic);
    if (mqtt_client_subscribe(client, sub_topic, 0) == 0) {
        printf("Subscribed successfully\n");
    } else {
        printf("Subscribe failed\n");
    }
    
    printf("Subscribing to topic: %s...\n", pub_topic);
    if (mqtt_client_subscribe(client, pub_topic, 0) == 0) {
        printf("Subscribed successfully\n");
    }
    
    printf("Publishing test message...\n");
    const char* msg = "Hello from libmqtt!";
    if (mqtt_client_publish(client, pub_topic, (uint8_t*)msg, strlen(msg), 0) == 0) {
        printf("Published: %s\n", msg);
    }
    
    printf("\nRunning... (Press Ctrl+C to exit)\n");
    int count = 0;
    while (running) {
        mqtt_client_loop(client);
        mqtt_os_get()->sleep_ms(1000);
        
        if (++count % 30 == 0 && mqtt_client_is_connected(client)) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Heartbeat #%d from libmqtt client", count / 30);
            if (mqtt_client_publish(client, "test/demo", (uint8_t*)buf, strlen(buf), 0) == 0) {
                printf("[SEND] Published heartbeat: %s\n", buf);
            }
        }
    }
    
    printf("\nDisconnecting...\n");
    mqtt_client_disconnect(client);
    mqtt_client_destroy(client);
    
    printf("Done!\n");
    return 0;
}
