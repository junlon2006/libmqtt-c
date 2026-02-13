/**
 * @file mqtt_net.h
 * @brief MQTT network abstraction layer interface
 * 
 * This header defines the network abstraction layer for the MQTT client library.
 * Users must implement these interfaces to port the library to different network stacks.
 */

#ifndef MQTT_NET_H
#define MQTT_NET_H

#include <stdint.h>
#include <stddef.h>

/** @brief Opaque socket handle */
typedef void* mqtt_socket_t;

/**
 * @brief Network abstraction layer API structure
 * 
 * This structure contains function pointers for all network operations.
 * Users must implement all these functions and register them via mqtt_net_init().
 */
typedef struct {
    /** @brief Connect to remote host
     *  @param host Hostname or IP address
     *  @param port Port number
     *  @param timeout_ms Connection timeout in milliseconds
     *  @return Socket handle on success, NULL on failure
     */
    mqtt_socket_t (*connect)(const char* host, uint16_t port, uint32_t timeout_ms);
    
    /** @brief Disconnect and close socket
     *  @param sock Socket handle
     */
    void (*disconnect)(mqtt_socket_t sock);
    
    /** @brief Send data through socket
     *  @param sock Socket handle
     *  @param buf Data buffer to send
     *  @param len Length of data
     *  @param timeout_ms Send timeout in milliseconds
     *  @return Number of bytes sent, or negative on error
     */
    int (*send)(mqtt_socket_t sock, const uint8_t* buf, size_t len, uint32_t timeout_ms);
    
    /** @brief Receive data from socket
     *  @param sock Socket handle
     *  @param buf Buffer to store received data
     *  @param len Maximum length to receive
     *  @param timeout_ms Receive timeout in milliseconds
     *  @return Number of bytes received, 0 on timeout, negative on error
     */
    int (*recv)(mqtt_socket_t sock, uint8_t* buf, size_t len, uint32_t timeout_ms);
} mqtt_net_api_t;

/**
 * @brief Initialize network abstraction layer
 * @param api Pointer to network API structure
 */
void mqtt_net_init(const mqtt_net_api_t* api);

/**
 * @brief Get current network API
 * @return Pointer to registered network API structure
 */
const mqtt_net_api_t* mqtt_net_get(void);

#endif /* MQTT_NET_H */
