/**
 * @file mqtt_tls.c
 * @brief MQTT TLS abstraction layer implementation
 */

#include "mqtt_tls.h"

/* Global TLS API pointer */
static const mqtt_tls_api_t* g_tls_api = NULL;

void mqtt_tls_init(const mqtt_tls_api_t* api) {
    g_tls_api = api;
}

const mqtt_tls_api_t* mqtt_tls_get(void) {
    return g_tls_api;
}
