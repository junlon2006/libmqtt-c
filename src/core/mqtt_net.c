/**
 * @file mqtt_net.c
 * @brief MQTT network abstraction layer implementation
 */

#include "mqtt_net.h"

/* Global network API pointer */

static const mqtt_net_api_t* g_net_api = NULL;

void mqtt_net_init(const mqtt_net_api_t* api) {
    g_net_api = api;
}

const mqtt_net_api_t* mqtt_net_get(void) {
    return g_net_api;
}
