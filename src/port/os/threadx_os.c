/**
 * @file threadx_os.c
 * @brief ThreadX RTOS OS abstraction layer implementation
 * 
 * ThreadX (Azure RTOS) is a real-time operating system from Microsoft.
 * This implementation provides OS abstraction for ThreadX kernel.
 */

#include "mqtt_os.h"
#include "tx_api.h"
#include <stdlib.h>

static void* threadx_malloc(size_t size) {
    return malloc(size);
}

static void threadx_free(void* ptr) {
    free(ptr);
}

static mqtt_mutex_t threadx_mutex_create(void) {
    TX_MUTEX* mutex = malloc(sizeof(TX_MUTEX));
    tx_mutex_create(mutex, "mqtt_mutex", TX_NO_INHERIT);
    return (mqtt_mutex_t)mutex;
}

static void threadx_mutex_destroy(mqtt_mutex_t mutex) {
    tx_mutex_delete((TX_MUTEX*)mutex);
    free(mutex);
}

static int threadx_mutex_lock(mqtt_mutex_t mutex) {
    return tx_mutex_get((TX_MUTEX*)mutex, TX_WAIT_FOREVER) == TX_SUCCESS ? 0 : -1;
}

static void threadx_mutex_unlock(mqtt_mutex_t mutex) {
    tx_mutex_put((TX_MUTEX*)mutex);
}

static mqtt_sem_t threadx_sem_create(uint32_t init_count) {
    TX_SEMAPHORE* sem = malloc(sizeof(TX_SEMAPHORE));
    tx_semaphore_create(sem, "mqtt_sem", init_count);
    return (mqtt_sem_t)sem;
}

static void threadx_sem_destroy(mqtt_sem_t sem) {
    tx_semaphore_delete((TX_SEMAPHORE*)sem);
    free(sem);
}

static int threadx_sem_wait(mqtt_sem_t sem, uint32_t timeout_ms) {
    ULONG ticks = (timeout_ms == 0xFFFFFFFF) ? TX_WAIT_FOREVER : (timeout_ms * TX_TIMER_TICKS_PER_SECOND / 1000);
    return tx_semaphore_get((TX_SEMAPHORE*)sem, ticks) == TX_SUCCESS ? 0 : -1;
}

static void threadx_sem_post(mqtt_sem_t sem) {
    tx_semaphore_put((TX_SEMAPHORE*)sem);
}

static mqtt_thread_t threadx_thread_create(mqtt_thread_func_t func, void* arg, 
                                           uint32_t stack_size, uint32_t priority) {
    TX_THREAD* thread = malloc(sizeof(TX_THREAD));
    void* stack = malloc(stack_size);
    tx_thread_create(thread, "mqtt_thread", (VOID (*)(ULONG))func, (ULONG)arg,
                     stack, stack_size, priority, priority, TX_NO_TIME_SLICE, TX_AUTO_START);
    return (mqtt_thread_t)thread;
}

static void threadx_thread_destroy(mqtt_thread_t thread) {
    /* ThreadX: thread_exit already deleted, no-op */
    (void)thread;
}

static void threadx_thread_exit(void) {
    TX_THREAD* self = tx_thread_identify();
    tx_thread_terminate(self);
    tx_thread_delete(self);
}

static uint32_t threadx_get_time_ms(void) {
    return tx_time_get() * 1000 / TX_TIMER_TICKS_PER_SECOND;
}

static void threadx_sleep_ms(uint32_t ms) {
    tx_thread_sleep(ms * TX_TIMER_TICKS_PER_SECOND / 1000);
}

static const mqtt_os_api_t threadx_os_api = {
    .malloc = threadx_malloc,
    .free = threadx_free,
    .mutex_create = threadx_mutex_create,
    .mutex_destroy = threadx_mutex_destroy,
    .mutex_lock = threadx_mutex_lock,
    .mutex_unlock = threadx_mutex_unlock,
    .sem_create = threadx_sem_create,
    .sem_destroy = threadx_sem_destroy,
    .sem_wait = threadx_sem_wait,
    .sem_post = threadx_sem_post,
    .thread_create = threadx_thread_create,
    .thread_destroy = threadx_thread_destroy,
    .thread_exit = threadx_thread_exit,
    .get_time_ms = threadx_get_time_ms,
    .sleep_ms = threadx_sleep_ms
};

void mqtt_threadx_init(void) {
    mqtt_os_init(&threadx_os_api);
}
