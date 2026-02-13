/**
 * @file freertos_os.c
 * @brief FreeRTOS OS abstraction layer implementation
 */

#include "mqtt.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdlib.h>
#include <sys/time.h>

static void* freertos_malloc(size_t size) {
    return pvPortMalloc(size);
}

static void freertos_free(void* ptr) {
    vPortFree(ptr);
}

static mqtt_mutex_t freertos_mutex_create(void) {
    return (mqtt_mutex_t)xSemaphoreCreateMutex();
}

static void freertos_mutex_destroy(mqtt_mutex_t mutex) {
    vSemaphoreDelete((SemaphoreHandle_t)mutex);
}

static int freertos_mutex_lock(mqtt_mutex_t mutex, uint32_t timeout_ms) {
    TickType_t ticks = (timeout_ms == 0xFFFFFFFF) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake((SemaphoreHandle_t)mutex, ticks) == pdTRUE ? 0 : -1;
}

static void freertos_mutex_unlock(mqtt_mutex_t mutex) {
    xSemaphoreGive((SemaphoreHandle_t)mutex);
}

static mqtt_sem_t freertos_sem_create(uint32_t init_count) {
    return (mqtt_sem_t)xSemaphoreCreateCounting(0xFFFF, init_count);
}

static void freertos_sem_destroy(mqtt_sem_t sem) {
    vSemaphoreDelete((SemaphoreHandle_t)sem);
}

static int freertos_sem_wait(mqtt_sem_t sem, uint32_t timeout_ms) {
    TickType_t ticks = (timeout_ms == 0xFFFFFFFF) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake((SemaphoreHandle_t)sem, ticks) == pdTRUE ? 0 : -1;
}

static void freertos_sem_post(mqtt_sem_t sem) {
    xSemaphoreGive((SemaphoreHandle_t)sem);
}

static mqtt_thread_t freertos_thread_create(mqtt_thread_func_t func, void* arg, 
                                            uint32_t stack_size, uint32_t priority) {
    TaskHandle_t handle;
    xTaskCreate((TaskFunction_t)func, "mqtt", stack_size / 4, arg, priority, &handle);
    return (mqtt_thread_t)handle;
}

static void freertos_thread_destroy(mqtt_thread_t thread) {
    /* FreeRTOS: thread_exit (vTaskDelete(NULL)) already deleted itself, no-op */
    (void)thread;
}

static void freertos_thread_exit(void) {
    vTaskDelete(NULL);
}

static uint32_t freertos_get_time_ms(void) {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

static void freertos_sleep_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static const mqtt_os_api_t freertos_os_api = {
    .malloc = freertos_malloc,
    .free = freertos_free,
    .mutex_create = freertos_mutex_create,
    .mutex_destroy = freertos_mutex_destroy,
    .mutex_lock = freertos_mutex_lock,
    .mutex_unlock = freertos_mutex_unlock,
    .sem_create = freertos_sem_create,
    .sem_destroy = freertos_sem_destroy,
    .sem_wait = freertos_sem_wait,
    .sem_post = freertos_sem_post,
    .thread_create = freertos_thread_create,
    .thread_destroy = freertos_thread_destroy,
    .thread_exit = freertos_thread_exit,
    .get_time_ms = freertos_get_time_ms,
    .sleep_ms = freertos_sleep_ms
};

void mqtt_freertos_init(void) {
    mqtt_os_init(&freertos_os_api);
}
