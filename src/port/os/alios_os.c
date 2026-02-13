/**
 * @file alios_os.c
 * @brief AliOS Things OS abstraction layer implementation
 * 
 * AliOS Things is an IoT operating system from Alibaba Cloud.
 * This implementation provides OS abstraction for AliOS Things kernel.
 */

#include "mqtt_os.h"
#include <aos/kernel.h>
#include <stdlib.h>

static void* alios_malloc(size_t size) {
    return aos_malloc(size);
}

static void alios_free(void* ptr) {
    aos_free(ptr);
}

static mqtt_mutex_t alios_mutex_create(void) {
    aos_mutex_t* mutex = malloc(sizeof(aos_mutex_t));
    aos_mutex_new(mutex);
    return (mqtt_mutex_t)mutex;
}

static void alios_mutex_destroy(mqtt_mutex_t mutex) {
    aos_mutex_free((aos_mutex_t*)mutex);
    free(mutex);
}

static int alios_mutex_lock(mqtt_mutex_t mutex) {
    return aos_mutex_lock((aos_mutex_t*)mutex, AOS_WAIT_FOREVER) == 0 ? 0 : -1;
}

static void alios_mutex_unlock(mqtt_mutex_t mutex) {
    aos_mutex_unlock((aos_mutex_t*)mutex);
}

static mqtt_sem_t alios_sem_create(uint32_t init_count) {
    aos_sem_t* sem = malloc(sizeof(aos_sem_t));
    aos_sem_new(sem, init_count);
    return (mqtt_sem_t)sem;
}

static void alios_sem_destroy(mqtt_sem_t sem) {
    aos_sem_free((aos_sem_t*)sem);
    free(sem);
}

static int alios_sem_wait(mqtt_sem_t sem, uint32_t timeout_ms) {
    return aos_sem_wait((aos_sem_t*)sem, timeout_ms) == 0 ? 0 : -1;
}

static void alios_sem_post(mqtt_sem_t sem) {
    aos_sem_signal((aos_sem_t*)sem);
}

static mqtt_thread_t alios_thread_create(mqtt_thread_func_t func, void* arg, 
                                         uint32_t stack_size, uint32_t priority) {
    aos_task_t* task = malloc(sizeof(aos_task_t));
    aos_task_new_ext(task, "mqtt_task", func, arg, stack_size, priority);
    return (mqtt_thread_t)task;
}

static void alios_thread_destroy(mqtt_thread_t thread) {
    /* In AliOS, thread_exit already exited, just free the handle */
    free(thread);
}

static void alios_thread_exit(void) {
    aos_task_exit(0);
}

static uint32_t alios_get_time_ms(void) {
    return aos_now_ms();
}

static void alios_sleep_ms(uint32_t ms) {
    aos_msleep(ms);
}

static const mqtt_os_api_t alios_os_api = {
    .malloc = alios_malloc,
    .free = alios_free,
    .mutex_create = alios_mutex_create,
    .mutex_destroy = alios_mutex_destroy,
    .mutex_lock = alios_mutex_lock,
    .mutex_unlock = alios_mutex_unlock,
    .sem_create = alios_sem_create,
    .sem_destroy = alios_sem_destroy,
    .sem_wait = alios_sem_wait,
    .sem_post = alios_sem_post,
    .thread_create = alios_thread_create,
    .thread_destroy = alios_thread_destroy,
    .thread_exit = alios_thread_exit,
    .get_time_ms = alios_get_time_ms,
    .sleep_ms = alios_sleep_ms
};

void mqtt_alios_init(void) {
    mqtt_os_init(&alios_os_api);
}
