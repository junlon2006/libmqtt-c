/**
 * @file mqtt_tls.h
 * @brief MQTT TLS/SSL abstraction layer interface
 * 
 * This header defines the TLS abstraction layer for secure MQTT connections.
 * Users can implement this interface using mbedTLS, OpenSSL, or other TLS libraries.
 */

#ifndef MQTT_TLS_H
#define MQTT_TLS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque TLS context handle */
typedef void* mqtt_tls_context_t;

/** @brief Opaque TLS session handle */
typedef void* mqtt_tls_session_t;

/**
 * @brief TLS configuration structure
 */
typedef struct {
    const char* ca_cert;        /**< CA certificate (PEM format) */
    size_t ca_cert_len;         /**< CA certificate length */
    const char* client_cert;    /**< Client certificate (optional) */
    size_t client_cert_len;     /**< Client certificate length */
    const char* client_key;     /**< Client private key (optional) */
    size_t client_key_len;      /**< Client key length */
    int verify_mode;            /**< Certificate verification mode (0=none, 1=optional, 2=required) */
} mqtt_tls_config_t;

/**
 * @brief TLS abstraction layer API structure
 */
typedef struct {
    /** @brief Initialize TLS context
     *  @param config TLS configuration
     *  @return TLS context handle on success, NULL on failure
     */
    mqtt_tls_context_t (*init)(const mqtt_tls_config_t* config);
    
    /** @brief Cleanup TLS context
     *  @param ctx TLS context handle
     */
    void (*cleanup)(mqtt_tls_context_t ctx);
    
    /** @brief Create TLS session and perform handshake
     *  @param ctx TLS context handle
     *  @param hostname Server hostname (for SNI)
     *  @param fd Socket file descriptor
     *  @return TLS session handle on success, NULL on failure
     */
    mqtt_tls_session_t (*connect)(mqtt_tls_context_t ctx, const char* hostname, int fd);
    
    /** @brief Close TLS session
     *  @param session TLS session handle
     */
    void (*disconnect)(mqtt_tls_session_t session);
    
    /** @brief Send data over TLS
     *  @param session TLS session handle
     *  @param buf Data buffer
     *  @param len Data length
     *  @return Number of bytes sent, or negative on error
     */
    int (*send)(mqtt_tls_session_t session, const uint8_t* buf, size_t len);
    
    /** @brief Receive data over TLS
     *  @param session TLS session handle
     *  @param buf Buffer to store received data
     *  @param len Maximum length to receive
     *  @return Number of bytes received, 0 on timeout, negative on error
     */
    int (*recv)(mqtt_tls_session_t session, uint8_t* buf, size_t len);
} mqtt_tls_api_t;

/**
 * @brief Initialize TLS abstraction layer
 * @param api Pointer to TLS API structure
 */
void mqtt_tls_init(const mqtt_tls_api_t* api);

/**
 * @brief Get current TLS API
 * @return Pointer to registered TLS API structure
 */
const mqtt_tls_api_t* mqtt_tls_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_TLS_H */
