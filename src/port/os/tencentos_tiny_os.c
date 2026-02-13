/**
 * @file tencentos_tiny_os.c
 * @brief TencentOS-tiny OS abstraction layer implementation
 * 
 * TencentOS-tiny is a lightweight IoT operating system from Tencent.
 * This implementation provides OS abstraction for TencentOS-tiny kernel.
 */

#include "mqtt_os.h"
#include "tos_k.h"

static void* tencentos_malloc(size_t size) {
    return tos_mmheap_alloc(size);
}

static void tencentos_free(void* ptr) {
    tos_mmheap_free(ptr);
}

static mqtt_mutex_t tencentos_mutex_create(void) {
    k_mutex_t* mutex = tos_mmheap_alloc(sizeof(k_mutex_t));
    tos_mutex_create(mutex);
    return (mqtt_mutex_t)mutex;
}

static void tencentos_mutex_destroy(mqtt_mutex_t mutex) {
    tos_mutex_destroy((k_mutex_t*)mutex);
    tos_mmheap_free(mutex);
}

static int tencentos_mutex_lock(mqtt_mutex_t mutex) {
    return tos_mutex_pend_timed((k_mutex_t*)mutex, TOS_TIME_FOREVER) == K_ERR_NONE ? 0 : -1;
}

static void tencentos_mutex_unlock(mqtt_mutex_t mutex) {
    tos_mutex_post((k_mutex_t*)mutex);
}

static mqtt_sem_t tencentos_sem_create(uint32_t init_count) {
    k_sem_t* sem = tos_mmheap_alloc(sizeof(k_sem_t));
    tos_sem_create(sem, init_count);
    return (mqtt_sem_t)sem;
}

static void tencentos_sem_destroy(mqtt_sem_t sem) {
    tos_sem_destroy((k_sem_t*)sem);
    tos_mmheap_free(sem);
}

static int tencentos_sem_wait(mqtt_sem_t sem, uint32_t timeout_ms) {
    k_tick_t timeout = (timeout_ms == 0xFFFFFFFF) ? TOS_TIME_FOREVER : tos_millisec2tick(timeout_ms);
    return tos_sem_pend((k_sem_t*)sem, timeout) == K_ERR_NONE ? 0 : -1;
}

static void tencentos_sem_post(mqtt_sem_t sem) {
    tos_sem_post((k_sem_t*)sem);
}

static mqtt_thread_t tencentos_thread_create(mqtt_thread_func_t func, void* arg, 
                                             uint32_t stack_size, uint32_t priority) {
    k_task_t* task = tos_mmheap_alloc(sizeof(k_task_t));
    k_stack_t* stack = tos_mmheap_alloc(stack_size);
    tos_task_create(task, "mqtt_task", (k_task_entry_t)func, arg, priority, stack, stack_size, 0);
    return (mqtt_thread_t)task;
}

static void tencentos_thread_destroy(mqtt_thread_t thread) {
    /* TencentOS thread_exit already destroyed, no-op */
    (void)thread;
}

static void tencentos_thread_exit(void) {
    tos_task_destroy(NULL);
}

static uint32_t tencentos_get_time_ms(void) {
    return tos_systick_get() * 1000 / TOS_CFG_CPU_TICK_PER_SECOND;
}

static void tencentos_sleep_ms(uint32_t ms) {
    tos_task_delay(tos_millisec2tick(ms));
}

static const mqtt_os_api_t tencentos_os_api = {
    .malloc = tencentos_malloc,
    .free = tencentos_free,
    .mutex_create = tencentos_mutex_create,
    .mutex_destroy = tencentos_mutex_destroy,
    .mutex_lock = tencentos_mutex_lock,
    .mutex_unlock = tencentos_mutex_unlock,
    .sem_create = tencentos_sem_create,
    .sem_destroy = tencentos_sem_destroy,
    .sem_wait = tencentos_sem_wait,
    .sem_post = tencentos_sem_post,
    .thread_create = tencentos_thread_create,
    .thread_destroy = tencentos_thread_destroy,
    .thread_exit = tencentos_thread_exit,
    .get_time_ms = tencentos_get_time_ms,
    .sleep_ms = tencentos_sleep_ms
};

void mqtt_tencentos_tiny_init(void) {
    mqtt_os_init(&tencentos_os_api);
}
