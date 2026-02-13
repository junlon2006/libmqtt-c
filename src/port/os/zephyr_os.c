/**
 * @file zephyr_os.c
 * @brief Zephyr RTOS OS abstraction layer implementation
 * 
 * Zephyr is a scalable real-time operating system from Linux Foundation.
 * This implementation provides OS abstraction for Zephyr kernel.
 */

#include "mqtt_os.h"
#include <zephyr/kernel.h>

static void* zephyr_malloc(size_t size) {
    return k_malloc(size);
}

static void zephyr_free(void* ptr) {
    k_free(ptr);
}

static mqtt_mutex_t zephyr_mutex_create(void) {
    struct k_mutex* mutex = k_malloc(sizeof(struct k_mutex));
    k_mutex_init(mutex);
    return (mqtt_mutex_t)mutex;
}

static void zephyr_mutex_destroy(mqtt_mutex_t mutex) {
    k_free(mutex);
}

static int zephyr_mutex_lock(mqtt_mutex_t mutex, uint32_t timeout_ms) {
    k_timeout_t timeout = (timeout_ms == 0xFFFFFFFF) ? K_FOREVER : K_MSEC(timeout_ms);
    return k_mutex_lock((struct k_mutex*)mutex, timeout) == 0 ? 0 : -1;
}

static void zephyr_mutex_unlock(mqtt_mutex_t mutex) {
    k_mutex_unlock((struct k_mutex*)mutex);
}

static mqtt_sem_t zephyr_sem_create(uint32_t init_count) {
    struct k_sem* sem = k_malloc(sizeof(struct k_sem));
    k_sem_init(sem, init_count, UINT_MAX);
    return (mqtt_sem_t)sem;
}

static void zephyr_sem_destroy(mqtt_sem_t sem) {
    k_free(sem);
}

static int zephyr_sem_wait(mqtt_sem_t sem, uint32_t timeout_ms) {
    k_timeout_t timeout = (timeout_ms == 0xFFFFFFFF) ? K_FOREVER : K_MSEC(timeout_ms);
    return k_sem_take((struct k_sem*)sem, timeout) == 0 ? 0 : -1;
}

static void zephyr_sem_post(mqtt_sem_t sem) {
    k_sem_give((struct k_sem*)sem);
}

static mqtt_thread_t zephyr_thread_create(mqtt_thread_func_t func, void* arg, 
                                          uint32_t stack_size, uint32_t priority) {
    k_tid_t thread = k_thread_create(k_malloc(sizeof(struct k_thread)),
                                     k_malloc(stack_size),
                                     stack_size,
                                     (k_thread_entry_t)func,
                                     arg, NULL, NULL,
                                     priority, 0, K_NO_WAIT);
    return (mqtt_thread_t)thread;
}

static void zephyr_thread_destroy(mqtt_thread_t thread) {
    /* Zephyr k_thread_abort already aborts, no-op */
    (void)thread;
}

static void zephyr_thread_exit(void) {
    k_thread_abort(k_current_get());
}

static uint32_t zephyr_get_time_ms(void) {
    return k_uptime_get_32();
}

static void zephyr_sleep_ms(uint32_t ms) {
    k_msleep(ms);
}

static const mqtt_os_api_t zephyr_os_api = {
    .malloc = zephyr_malloc,
    .free = zephyr_free,
    .mutex_create = zephyr_mutex_create,
    .mutex_destroy = zephyr_mutex_destroy,
    .mutex_lock = zephyr_mutex_lock,
    .mutex_unlock = zephyr_mutex_unlock,
    .sem_create = zephyr_sem_create,
    .sem_destroy = zephyr_sem_destroy,
    .sem_wait = zephyr_sem_wait,
    .sem_post = zephyr_sem_post,
    .thread_create = zephyr_thread_create,
    .thread_destroy = zephyr_thread_destroy,
    .thread_exit = zephyr_thread_exit,
    .get_time_ms = zephyr_get_time_ms,
    .sleep_ms = zephyr_sleep_ms
};

void mqtt_zephyr_init(void) {
    mqtt_os_init(&zephyr_os_api);
}
