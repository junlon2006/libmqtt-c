/**
 * @file tls_demo.c
 * @brief MQTT TLS demo using POSIX and OpenSSL
 * 
 * Compile: gcc -o tls_demo tls_demo.c posix_tls.c mqtt.c mqtt_os.c mqtt_net.c mqtt_tls.c \
 *          posix_os.c posix_net.c -I../../include -lssl -lcrypto -lpthread
 */

#include "mqtt.h"
#include "mqtt_tls.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>

void mqtt_posix_init(void);
void mqtt_posix_net_init(void);
void mqtt_openssl_init(void);

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
    
    printf("=== MQTT TLS Demo ===\n");
    
    // Initialize layers
    mqtt_posix_init();
    mqtt_posix_net_init();
    mqtt_openssl_init();
    
    // TLS configuration (optional - can use NULL for system default CA)
    mqtt_tls_config_t tls_config = {
        .ca_cert = NULL,        // Use system CA certificates
        .ca_cert_len = 0,
        .client_cert = NULL,    // No client certificate
        .client_cert_len = 0,
        .client_key = NULL,
        .client_key_len = 0,
        .verify_mode = 2        // Verify required
    };
    
    // MQTT configuration
    mqtt_config_t config = {
        .host = "test.mosquitto.org",  // Public test broker with TLS
        .port = 8883,                   // TLS port
        .client_id = "libmqtt_tls_demo",
        .username = NULL,
        .password = NULL,
        .keepalive = 60,
        .clean_session = 1,
        .use_tls = 1,                   // Enable TLS
        .tls_config = &tls_config,
        .msg_cb = on_message,
        .user_data = NULL
    };
    
    printf("Creating and connecting MQTT client to %s:%d with TLS...\n", config.host, config.port);
    mqtt_client_t* client = mqtt_client_create(&config);
    if (!client) {
        printf("Failed to create and connect client\n");
        return -1;
    }
    
    printf("Connected successfully!\n");
    
    // Subscribe
    printf("Subscribing to topic: test/tls...\n");
    if (mqtt_client_subscribe(client, "test/tls", 0) == 0) {
        printf("Subscribed successfully\n");
    }
    
    // Publish test message
    const char* msg = "Hello from libmqtt TLS!";
    printf("Publishing: %s\n", msg);
    mqtt_client_publish(client, "test/tls", (uint8_t*)msg, strlen(msg), 0);
    
    // Main loop
    printf("\nRunning... (Press Ctrl+C to exit)\n");
    int count = 0;
    while (running) {
        mqtt_os_get()->sleep_ms(1000);
        
        if (++count % 30 == 0 && mqtt_client_is_connected(client)) {
            char buf[128];
            snprintf(buf, sizeof(buf), "TLS Publish message #%d", count / 30);
            mqtt_client_publish(client, "test/tls", (uint8_t*)buf, strlen(buf), 0);
            printf("[SEND] %s\n", buf);
        }
    }
    
    printf("\nDisconnecting and destroying client...\n");
    mqtt_client_destroy(client);
    
    printf("Done!\n");
    return 0;
}
