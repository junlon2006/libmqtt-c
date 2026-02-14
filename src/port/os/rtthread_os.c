/**
 * @file rtthread_os.c
 * @brief RT-Thread RTOS OS abstraction layer implementation
 * 
 * RT-Thread is an open source real-time operating system from China.
 * This implementation provides OS abstraction for RT-Thread kernel.
 */

#include "mqtt_os.h"
#include <rtthread.h>

static void* rtthread_malloc(size_t size) {
    return rt_malloc(size);
}

static void rtthread_free(void* ptr) {
    rt_free(ptr);
}

static mqtt_mutex_t rtthread_mutex_create(void) {
    return (mqtt_mutex_t)rt_mutex_create("mqtt_mutex", RT_IPC_FLAG_FIFO);
}

static void rtthread_mutex_destroy(mqtt_mutex_t mutex) {
    rt_mutex_delete((rt_mutex_t)mutex);
}

static int rtthread_mutex_lock(mqtt_mutex_t mutex) {
    return rt_mutex_take((rt_mutex_t)mutex, RT_WAITING_FOREVER) == RT_EOK ? 0 : -1;
}

static void rtthread_mutex_unlock(mqtt_mutex_t mutex) {
    rt_mutex_release((rt_mutex_t)mutex);
}

static mqtt_sem_t rtthread_sem_create(uint32_t init_count) {
    return (mqtt_sem_t)rt_sem_create("mqtt_sem", init_count, RT_IPC_FLAG_FIFO);
}

static void rtthread_sem_destroy(mqtt_sem_t sem) {
    rt_sem_delete((rt_sem_t)sem);
}

static int rtthread_sem_wait(mqtt_sem_t sem) {
    return rt_sem_take((rt_sem_t)sem, RT_WAITING_FOREVER) == RT_EOK ? 0 : -1;
}

static void rtthread_sem_post(mqtt_sem_t sem) {
    rt_sem_release((rt_sem_t)sem);
}

static mqtt_thread_t rtthread_thread_create(mqtt_thread_func_t func, void* arg, 
                                            uint32_t stack_size, uint32_t priority) {
    rt_thread_t thread = rt_thread_create("mqtt_thread", (void (*)(void*))func, arg,
                                          stack_size, priority, 10);
    if (thread) {
        rt_thread_startup(thread);
    }
    return (mqtt_thread_t)thread;
}

static void rtthread_thread_destroy(mqtt_thread_t thread) {
    /* In RT-Thread, thread_exit already deleted itself, make this a no-op */
    (void)thread;
}

static void rtthread_thread_exit(void) {
    rt_thread_delete(rt_thread_self());
}

static uint32_t rtthread_get_time_ms(void) {
    return rt_tick_get() * 1000 / RT_TICK_PER_SECOND;
}

static void rtthread_sleep_ms(uint32_t ms) {
    rt_thread_mdelay(ms);
}

static const mqtt_os_api_t rtthread_os_api = {
    .malloc = rtthread_malloc,
    .free = rtthread_free,
    .mutex_create = rtthread_mutex_create,
    .mutex_destroy = rtthread_mutex_destroy,
    .mutex_lock = rtthread_mutex_lock,
    .mutex_unlock = rtthread_mutex_unlock,
    .sem_create = rtthread_sem_create,
    .sem_destroy = rtthread_sem_destroy,
    .sem_wait = rtthread_sem_wait,
    .sem_post = rtthread_sem_post,
    .thread_create = rtthread_thread_create,
    .thread_destroy = rtthread_thread_destroy,
    .thread_exit = rtthread_thread_exit,
    .get_time_ms = rtthread_get_time_ms,
    .sleep_ms = rtthread_sleep_ms
};

void mqtt_rtthread_init(void) {
    mqtt_os_init(&rtthread_os_api);
}
