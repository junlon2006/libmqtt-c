/**
 * @file mqtt_os.c
 * @brief MQTT OS abstraction layer implementation
 */

#include "mqtt_os.h"

static const mqtt_os_api_t* g_os_api = NULL;

void mqtt_os_init(const mqtt_os_api_t* api) {
    g_os_api = api;
}

const mqtt_os_api_t* mqtt_os_get(void) {
    return g_os_api;
}
