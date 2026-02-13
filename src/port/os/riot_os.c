/**
 * @file riot_os.c
 * @brief RIOT OS abstraction layer implementation
 * 
 * RIOT is an IoT-friendly real-time operating system.
 * This implementation provides OS abstraction for RIOT kernel.
 */

#include "mqtt_os.h"
#include "thread.h"
#include "mutex.h"
#include "sema.h"
#include "xtimer.h"
#include <stdlib.h>

static void* riot_malloc(size_t size) {
    return malloc(size);
}

static void riot_free(void* ptr) {
    free(ptr);
}

static mqtt_mutex_t riot_mutex_create(void) {
    mutex_t* mutex = malloc(sizeof(mutex_t));
    mutex_init(mutex);
    return (mqtt_mutex_t)mutex;
}

static void riot_mutex_destroy(mqtt_mutex_t mutex) {
    free(mutex);
}

static int riot_mutex_lock(mqtt_mutex_t mutex) {
    mutex_lock((mutex_t*)mutex);
    return 0;
}

static void riot_mutex_unlock(mqtt_mutex_t mutex) {
    mutex_unlock((mutex_t*)mutex);
}

static mqtt_sem_t riot_sem_create(uint32_t init_count) {
    sema_t* sem = malloc(sizeof(sema_t));
    sema_create(sem, init_count);
    return (mqtt_sem_t)sem;
}

static void riot_sem_destroy(mqtt_sem_t sem) {
    sema_destroy((sema_t*)sem);
    free(sem);
}

static int riot_sem_wait(mqtt_sem_t sem, uint32_t timeout_ms) {
    if (timeout_ms == 0xFFFFFFFF) {
        sema_wait((sema_t*)sem);
        return 0;
    } else {
        return sema_wait_timed((sema_t*)sem, timeout_ms * 1000) == 0 ? 0 : -1;
    }
}

static void riot_sem_post(mqtt_sem_t sem) {
    sema_post((sema_t*)sem);
}

static mqtt_thread_t riot_thread_create(mqtt_thread_func_t func, void* arg, 
                                        uint32_t stack_size, uint32_t priority) {
    char* stack = malloc(stack_size);
    kernel_pid_t* pid = malloc(sizeof(kernel_pid_t));
    *pid = thread_create(stack, stack_size, priority, THREAD_CREATE_STACKTEST,
                         (thread_task_func_t)func, arg, "mqtt_thread");
    return (mqtt_thread_t)pid;
}

static void riot_thread_destroy(mqtt_thread_t thread) {
    free(thread);
}

static void riot_thread_exit(void) {
    thread_exit();
}

static uint32_t riot_get_time_ms(void) {
    return xtimer_now_usec() / 1000;
}

static void riot_sleep_ms(uint32_t ms) {
    xtimer_usleep(ms * 1000);
}

static const mqtt_os_api_t riot_os_api = {
    .malloc = riot_malloc,
    .free = riot_free,
    .mutex_create = riot_mutex_create,
    .mutex_destroy = riot_mutex_destroy,
    .mutex_lock = riot_mutex_lock,
    .mutex_unlock = riot_mutex_unlock,
    .sem_create = riot_sem_create,
    .sem_destroy = riot_sem_destroy,
    .sem_wait = riot_sem_wait,
    .sem_post = riot_sem_post,
    .thread_create = riot_thread_create,
    .thread_destroy = riot_thread_destroy,
    .thread_exit = riot_thread_exit,
    .get_time_ms = riot_get_time_ms,
    .sleep_ms = riot_sleep_ms
};

void mqtt_riot_init(void) {
    mqtt_os_init(&riot_os_api);
}
