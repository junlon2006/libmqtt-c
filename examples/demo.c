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

/* MQTT Broker Configuration */
#define MQTT_BROKER_HOST        "est.mosquitto.org"
#define MQTT_BROKER_PORT        1883
#define MQTT_CLIENT_ID          "libmqtt_test_client"
#define MQTT_USERNAME           NULL
#define MQTT_PASSWORD           NULL
#define MQTT_KEEPALIVE          60
#define MQTT_CLEAN_SESSION      1

/* Topic Configuration */
#define MQTT_SUB_TOPIC_1        "test/demo/sub1"
#define MQTT_SUB_TOPIC_2        "test/demo/sub2"
#define MQTT_PUB_TOPIC          "test/demo/pub"

/* Application Configuration */
#define MQTT_LOOP_INTERVAL_MS   1000
#define MQTT_HEARTBEAT_INTERVAL 30

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
        .host = MQTT_BROKER_HOST,
        .port = MQTT_BROKER_PORT,
        .client_id = MQTT_CLIENT_ID,
        .username = MQTT_USERNAME,
        .password = MQTT_PASSWORD,
        .keepalive = MQTT_KEEPALIVE,
        .clean_session = MQTT_CLEAN_SESSION,
        .msg_cb = on_message,
        .user_data = NULL
    };
    
    printf("Creating and connecting MQTT client to %s:%d...\n", config.host, config.port);
    client = mqtt_client_create(&config);
    if (!client) {
        printf("Failed to create and connect client\n");
        return -1;
    }
    
    printf("Connected successfully!\n");
    
    const char* sub_topic = MQTT_SUB_TOPIC_1;
    const char* pub_topic = MQTT_PUB_TOPIC;
    
    printf("Subscribing to topic: %s...\n", sub_topic);
    if (mqtt_client_subscribe(client, sub_topic, 0) == 0) {
        printf("Subscribed successfully\n");
    } else {
        printf("Subscribe failed\n");
    }
    
    printf("Subscribing to topic: %s...\n", MQTT_SUB_TOPIC_2);
    if (mqtt_client_subscribe(client, MQTT_SUB_TOPIC_2, 0) == 0) {
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
        if (++count % MQTT_HEARTBEAT_INTERVAL == 0 && mqtt_client_is_connected(client)) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Heartbeat #%d from libmqtt client", count / MQTT_HEARTBEAT_INTERVAL);
            if (mqtt_client_publish(client, MQTT_PUB_TOPIC, (uint8_t*)buf, strlen(buf), 0) == 0) {
                printf("[SEND] Published heartbeat: %s\n", buf);
            }
        }
    
        mqtt_os_get()->sleep_ms(MQTT_LOOP_INTERVAL_MS);
    }
    
    printf("\nDisconnecting and destroying client...\n");
    mqtt_client_destroy(client);
    
    printf("Done!\n");
    return 0;
}
