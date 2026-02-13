/**
 * @file nuttx_os.c
 * @brief NuttX RTOS OS abstraction layer implementation
 * 
 * NuttX is an Apache real-time operating system.
 * This implementation provides OS abstraction for NuttX kernel.
 */

#include "mqtt_os.h"
#include <nuttx/config.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

static void* nuttx_malloc(size_t size) {
    return malloc(size);
}

static void nuttx_free(void* ptr) {
    free(ptr);
}

static mqtt_mutex_t nuttx_mutex_create(void) {
    pthread_mutex_t* mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);
    return (mqtt_mutex_t)mutex;
}

static void nuttx_mutex_destroy(mqtt_mutex_t mutex) {
    pthread_mutex_destroy((pthread_mutex_t*)mutex);
    free(mutex);
}

static int nuttx_mutex_lock(mqtt_mutex_t mutex, uint32_t timeout_ms) {
    return pthread_mutex_lock((pthread_mutex_t*)mutex) == 0 ? 0 : -1;
}

static void nuttx_mutex_unlock(mqtt_mutex_t mutex) {
    pthread_mutex_unlock((pthread_mutex_t*)mutex);
}

static mqtt_sem_t nuttx_sem_create(uint32_t init_count) {
    sem_t* sem = malloc(sizeof(sem_t));
    sem_init(sem, 0, init_count);
    return (mqtt_sem_t)sem;
}

static void nuttx_sem_destroy(mqtt_sem_t sem) {
    sem_destroy((sem_t*)sem);
    free(sem);
}

static int nuttx_sem_wait(mqtt_sem_t sem, uint32_t timeout_ms) {
    return sem_wait((sem_t*)sem) == 0 ? 0 : -1;
}

static void nuttx_sem_post(mqtt_sem_t sem) {
    sem_post((sem_t*)sem);
}

static mqtt_thread_t nuttx_thread_create(mqtt_thread_func_t func, void* arg, 
                                         uint32_t stack_size, uint32_t priority) {
    pthread_t* thread = malloc(sizeof(pthread_t));
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stack_size);
    pthread_create(thread, &attr, (void*(*)(void*))func, arg);
    pthread_attr_destroy(&attr);
    return (mqtt_thread_t)thread;
}

static void nuttx_thread_destroy(mqtt_thread_t thread) {
    pthread_cancel(*(pthread_t*)thread);
    pthread_join(*(pthread_t*)thread, NULL);
    free(thread);
}

static void nuttx_thread_exit(void) {
    pthread_exit(NULL);
}

static uint32_t nuttx_get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void nuttx_sleep_ms(uint32_t ms) {
    usleep(ms * 1000);
}

static const mqtt_os_api_t nuttx_os_api = {
    .malloc = nuttx_malloc,
    .free = nuttx_free,
    .mutex_create = nuttx_mutex_create,
    .mutex_destroy = nuttx_mutex_destroy,
    .mutex_lock = nuttx_mutex_lock,
    .mutex_unlock = nuttx_mutex_unlock,
    .sem_create = nuttx_sem_create,
    .sem_destroy = nuttx_sem_destroy,
    .sem_wait = nuttx_sem_wait,
    .sem_post = nuttx_sem_post,
    .thread_create = nuttx_thread_create,
    .thread_destroy = nuttx_thread_destroy,
    .thread_exit = nuttx_thread_exit,
    .get_time_ms = nuttx_get_time_ms,
    .sleep_ms = nuttx_sleep_ms
};

void mqtt_nuttx_init(void) {
    mqtt_os_init(&nuttx_os_api);
}
