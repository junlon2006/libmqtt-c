/**
 * @file mqtt.h
 * @brief MQTT 3.1.1 client library main interface
 * 
 * This is a lightweight MQTT 3.1.1 client implementation designed for RTOS platforms.
 * Features:
 * - Minimal resource usage (~2KB RAM)
 * - Fully portable via OS and network abstraction layers
 * - Automatic reconnection with subscription recovery
 * - Thread-safe operations
 * - Support for QoS 0/1
 */

#ifndef MQTT_H
#define MQTT_H

#include <stdint.h>
#include <stddef.h>
#include "mqtt_os.h"
#include "mqtt_net.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum MQTT packet size */
#define MQTT_MAX_PACKET_SIZE  1024

/** @brief Receive buffer size */
#define MQTT_RECV_BUF_SIZE    1024

/** @brief Maximum number of subscriptions to track for auto-resubscribe */
#define MQTT_MAX_SUBSCRIPTIONS 8

/**
 * @brief MQTT client connection state
 */
typedef enum {
    MQTT_STATE_DISCONNECTED = 0,  /**< Client is disconnected */
    MQTT_STATE_CONNECTED          /**< Client is connected */
} mqtt_state_t;

/**
 * @brief Message callback function type
 * @param topic Topic name
 * @param payload Message payload
 * @param len Payload length
 * @param user_data User-defined data passed from config
 */
typedef void (*mqtt_msg_callback_t)(const char* topic, const uint8_t* payload, size_t len, void* user_data);

/**
 * @brief Subscription information (internal use)
 */
typedef struct {
    char topic[128];  /**< Topic filter */
    uint8_t qos;      /**< QoS level */
} mqtt_subscription_t;

/**
 * @brief MQTT client configuration
 */
typedef struct {
    char* host;                      /**< Broker hostname or IP */
    uint16_t port;                   /**< Broker port (usually 1883) */
    char* client_id;                 /**< Client identifier */
    char* username;                  /**< Username (NULL if not used) */
    char* password;                  /**< Password (NULL if not used) */
    uint16_t keepalive;              /**< Keep-alive interval in seconds */
    uint8_t clean_session;           /**< Clean session flag (1=clean, 0=persistent) */
    uint8_t use_tls;                 /**< Enable TLS/SSL (1=enabled, 0=disabled) */
    void* tls_config;                /**< TLS configuration pointer */
    mqtt_msg_callback_t msg_cb;      /**< Message received callback */
    void* user_data;                 /**< User-defined data passed to callback */
} mqtt_config_t;

/**
 * @brief MQTT client handle (opaque structure)
 */
typedef struct {
    mqtt_config_t config;                                /**< Client configuration */
    mqtt_socket_t socket;                                /**< Network socket handle */
    mqtt_state_t state;                                  /**< Connection state */
    mqtt_mutex_t mutex;                                  /**< Thread safety mutex */
    mqtt_thread_t recv_thread;                           /**< Receive thread handle */
    mqtt_sem_t thread_exit_sem;                          /**< Thread exit synchronization semaphore */
    uint16_t packet_id;                                  /**< Packet ID counter */
    uint32_t last_ping_time;                             /**< Last ping timestamp */
    uint8_t send_buf[MQTT_MAX_PACKET_SIZE];              /**< Send buffer */
    uint8_t recv_buf[MQTT_RECV_BUF_SIZE];                /**< Receive buffer */
    volatile uint8_t running;                            /**< Thread running flag */
    mqtt_subscription_t subscriptions[MQTT_MAX_SUBSCRIPTIONS]; /**< Subscription list */
    uint8_t sub_count;                                   /**< Number of subscriptions */
} mqtt_client_t;

/**
 * @brief Create MQTT client instance
 * @param config Client configuration
 * @return Client handle on success, NULL on failure
 */
mqtt_client_t* mqtt_client_create(const mqtt_config_t* config);

/**
 * @brief Destroy MQTT client instance
 * @param client Client handle
 */
void mqtt_client_destroy(mqtt_client_t* client);

/**
 * @brief Connect to MQTT broker
 * @param client Client handle
 * @return 0 on success, -1 on failure
 */
int mqtt_client_connect(mqtt_client_t* client);

/**
 * @brief Disconnect from MQTT broker
 * @param client Client handle
 */
void mqtt_client_disconnect(mqtt_client_t* client);

/**
 * @brief Subscribe to a topic
 * @param client Client handle
 * @param topic Topic filter
 * @param qos QoS level (0 or 1)
 * @return 0 on success, -1 on failure
 * @note Subscriptions are automatically restored after reconnection
 */
int mqtt_client_subscribe(mqtt_client_t* client, const char* topic, uint8_t qos);

/**
 * @brief Publish a message
 * @param client Client handle
 * @param topic Topic name
 * @param payload Message payload
 * @param len Payload length
 * @param qos QoS level (0 or 1)
 * @return 0 on success, -1 on failure
 */
int mqtt_client_publish(mqtt_client_t* client, const char* topic, const uint8_t* payload, size_t len, uint8_t qos);

/**
 * @brief Maintain MQTT connection (must be called periodically)
 * @param client Client handle
 * @return 0 if connected, -1 if disconnected (will auto-reconnect)
 * @note This function handles:
 *       - Keep-alive ping messages
 *       - Automatic reconnection on network failure
 *       - Automatic resubscription after reconnection
 */
int mqtt_client_loop(mqtt_client_t* client);

/**
 * @brief Check if client is connected
 * @param client Client handle
 * @return 1 if connected, 0 if disconnected
 */
int mqtt_client_is_connected(mqtt_client_t* client);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_H */
