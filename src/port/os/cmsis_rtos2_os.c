/**
 * @file cmsis_rtos2_os.c
 * @brief CMSIS-RTOS2 OS abstraction layer implementation
 * 
 * CMSIS-RTOS2 is a standard API for ARM Cortex-M devices.
 * This implementation provides OS abstraction for CMSIS-RTOS2 compliant kernels.
 */

#include "mqtt_os.h"
#include "cmsis_os2.h"
#include <stdlib.h>

static void* cmsis_malloc(size_t size) {
    return malloc(size);
}

static void cmsis_free(void* ptr) {
    free(ptr);
}

static mqtt_mutex_t cmsis_mutex_create(void) {
    return (mqtt_mutex_t)osMutexNew(NULL);
}

static void cmsis_mutex_destroy(mqtt_mutex_t mutex) {
    osMutexDelete((osMutexId_t)mutex);
}

static int cmsis_mutex_lock(mqtt_mutex_t mutex, uint32_t timeout_ms) {
    uint32_t timeout = (timeout_ms == 0xFFFFFFFF) ? osWaitForever : timeout_ms;
    return osMutexAcquire((osMutexId_t)mutex, timeout) == osOK ? 0 : -1;
}

static void cmsis_mutex_unlock(mqtt_mutex_t mutex) {
    osMutexRelease((osMutexId_t)mutex);
}

static mqtt_sem_t cmsis_sem_create(uint32_t init_count) {
    return (mqtt_sem_t)osSemaphoreNew(0xFFFF, init_count, NULL);
}

static void cmsis_sem_destroy(mqtt_sem_t sem) {
    osSemaphoreDelete((osSemaphoreId_t)sem);
}

static int cmsis_sem_wait(mqtt_sem_t sem, uint32_t timeout_ms) {
    uint32_t timeout = (timeout_ms == 0xFFFFFFFF) ? osWaitForever : timeout_ms;
    return osSemaphoreAcquire((osSemaphoreId_t)sem, timeout) == osOK ? 0 : -1;
}

static void cmsis_sem_post(mqtt_sem_t sem) {
    osSemaphoreRelease((osSemaphoreId_t)sem);
}

static mqtt_thread_t cmsis_thread_create(mqtt_thread_func_t func, void* arg, 
                                         uint32_t stack_size, uint32_t priority) {
    osThreadAttr_t attr = {
        .name = "mqtt_thread",
        .stack_size = stack_size,
        .priority = (osPriority_t)priority
    };
    return (mqtt_thread_t)osThreadNew((osThreadFunc_t)func, arg, &attr);
}

static void cmsis_thread_destroy(mqtt_thread_t thread) {
    /* CMSIS osThreadExit already exited, no-op */
    (void)thread;
}

static void cmsis_thread_exit(void) {
    osThreadExit();
}

static uint32_t cmsis_get_time_ms(void) {
    return osKernelGetTickCount() * 1000 / osKernelGetTickFreq();
}

static void cmsis_sleep_ms(uint32_t ms) {
    osDelay(ms * osKernelGetTickFreq() / 1000);
}

static const mqtt_os_api_t cmsis_os_api = {
    .malloc = cmsis_malloc,
    .free = cmsis_free,
    .mutex_create = cmsis_mutex_create,
    .mutex_destroy = cmsis_mutex_destroy,
    .mutex_lock = cmsis_mutex_lock,
    .mutex_unlock = cmsis_mutex_unlock,
    .sem_create = cmsis_sem_create,
    .sem_destroy = cmsis_sem_destroy,
    .sem_wait = cmsis_sem_wait,
    .sem_post = cmsis_sem_post,
    .thread_create = cmsis_thread_create,
    .thread_destroy = cmsis_thread_destroy,
    .thread_exit = cmsis_thread_exit,
    .get_time_ms = cmsis_get_time_ms,
    .sleep_ms = cmsis_sleep_ms
};

void mqtt_cmsis_rtos2_init(void) {
    mqtt_os_init(&cmsis_os_api);
}
