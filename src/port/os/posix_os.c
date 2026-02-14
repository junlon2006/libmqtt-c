/**
 * @file posix_os.c
 * @brief POSIX platform OS abstraction layer implementation
 * 
 * This file implements the OS abstraction layer for POSIX-compliant systems
 * (Linux, macOS, Unix) using pthread and standard C library.
 */

#include "mqtt_os.h"
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

static void* posix_malloc(size_t size) {
    return malloc(size);
}

static void posix_free(void* ptr) {
    free(ptr);
}

static mqtt_mutex_t posix_mutex_create(void) {
    pthread_mutex_t* mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);
    return (mqtt_mutex_t)mutex;
}

static void posix_mutex_destroy(mqtt_mutex_t mutex) {
    pthread_mutex_destroy((pthread_mutex_t*)mutex);
    free(mutex);
}

static int posix_mutex_lock(mqtt_mutex_t mutex) {
    return pthread_mutex_lock((pthread_mutex_t*)mutex) == 0 ? 0 : -1;
}

static void posix_mutex_unlock(mqtt_mutex_t mutex) {
    pthread_mutex_unlock((pthread_mutex_t*)mutex);
}

static mqtt_sem_t posix_sem_create(uint32_t init_count) {
    sem_t* sem = malloc(sizeof(sem_t));
    sem_init(sem, 0, init_count);
    return (mqtt_sem_t)sem;
}

static void posix_sem_destroy(mqtt_sem_t sem) {
    sem_destroy((sem_t*)sem);
    free(sem);
}

static int posix_sem_wait(mqtt_sem_t sem) {
    return sem_wait((sem_t*)sem) == 0 ? 0 : -1;
}

static void posix_sem_post(mqtt_sem_t sem) {
    sem_post((sem_t*)sem);
}

static mqtt_thread_t posix_thread_create(mqtt_thread_func_t func, void* arg, 
                                         uint32_t stack_size, uint32_t priority) {
    pthread_t* thread = malloc(sizeof(pthread_t));
    pthread_create(thread, NULL, (void*(*)(void*))func, arg);
    return (mqtt_thread_t)thread;
}

static void posix_thread_destroy(mqtt_thread_t thread) {
    /* POSIX pthread_exit already exited, just join and free */
    pthread_join(*(pthread_t*)thread, NULL);
    free(thread);
}

static void posix_thread_exit(void) {
    pthread_exit(NULL);
}

static uint32_t posix_get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void posix_sleep_ms(uint32_t ms) {
    usleep(ms * 1000);
}

static const mqtt_os_api_t posix_os_api = {
    .malloc = posix_malloc,
    .free = posix_free,
    .mutex_create = posix_mutex_create,
    .mutex_destroy = posix_mutex_destroy,
    .mutex_lock = posix_mutex_lock,
    .mutex_unlock = posix_mutex_unlock,
    .sem_create = posix_sem_create,
    .sem_destroy = posix_sem_destroy,
    .sem_wait = posix_sem_wait,
    .sem_post = posix_sem_post,
    .thread_create = posix_thread_create,
    .thread_destroy = posix_thread_destroy,
    .thread_exit = posix_thread_exit,
    .get_time_ms = posix_get_time_ms,
    .sleep_ms = posix_sleep_ms
};

void mqtt_posix_init(void) {
    mqtt_os_init(&posix_os_api);
}
