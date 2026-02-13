/**
 * @file ucos3_os.c
 * @brief Micrium uC/OS-III OS abstraction layer implementation
 * 
 * uC/OS-III is a commercial real-time operating system from Micrium.
 * This implementation provides OS abstraction for uC/OS-III kernel.
 */

#include "mqtt_os.h"
#include <os.h>
#include <stdlib.h>

static void* ucos3_malloc(size_t size) {
    return malloc(size);
}

static void ucos3_free(void* ptr) {
    free(ptr);
}

static mqtt_mutex_t ucos3_mutex_create(void) {
    OS_MUTEX* mutex = malloc(sizeof(OS_MUTEX));
    OS_ERR err;
    OSMutexCreate(mutex, "mqtt_mutex", &err);
    return (mqtt_mutex_t)mutex;
}

static void ucos3_mutex_destroy(mqtt_mutex_t mutex) {
    OS_ERR err;
    OSMutexDel((OS_MUTEX*)mutex, OS_OPT_DEL_ALWAYS, &err);
    free(mutex);
}

static int ucos3_mutex_lock(mqtt_mutex_t mutex, uint32_t timeout_ms) {
    OS_ERR err;
    OS_TICK timeout = (timeout_ms == 0xFFFFFFFF) ? 0 : timeout_ms;
    OSMutexPend((OS_MUTEX*)mutex, timeout, OS_OPT_PEND_BLOCKING, NULL, &err);
    return (err == OS_ERR_NONE) ? 0 : -1;
}

static void ucos3_mutex_unlock(mqtt_mutex_t mutex) {
    OS_ERR err;
    OSMutexPost((OS_MUTEX*)mutex, OS_OPT_POST_NONE, &err);
}

static mqtt_sem_t ucos3_sem_create(uint32_t init_count) {
    OS_SEM* sem = malloc(sizeof(OS_SEM));
    OS_ERR err;
    OSSemCreate(sem, "mqtt_sem", init_count, &err);
    return (mqtt_sem_t)sem;
}

static void ucos3_sem_destroy(mqtt_sem_t sem) {
    OS_ERR err;
    OSSemDel((OS_SEM*)sem, OS_OPT_DEL_ALWAYS, &err);
    free(sem);
}

static int ucos3_sem_wait(mqtt_sem_t sem, uint32_t timeout_ms) {
    OS_ERR err;
    OS_TICK timeout = (timeout_ms == 0xFFFFFFFF) ? 0 : timeout_ms;
    OSSemPend((OS_SEM*)sem, timeout, OS_OPT_PEND_BLOCKING, NULL, &err);
    return (err == OS_ERR_NONE) ? 0 : -1;
}

static void ucos3_sem_post(mqtt_sem_t sem) {
    OS_ERR err;
    OSSemPost((OS_SEM*)sem, OS_OPT_POST_1, &err);
}

static mqtt_thread_t ucos3_thread_create(mqtt_thread_func_t func, void* arg, 
                                         uint32_t stack_size, uint32_t priority) {
    OS_TCB* tcb = malloc(sizeof(OS_TCB));
    CPU_STK* stack = malloc(stack_size);
    OS_ERR err;
    OSTaskCreate(tcb, "mqtt_task", (OS_TASK_PTR)func, arg, priority,
                 stack, stack_size / 10, stack_size, 0, 0, NULL,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR, &err);
    return (mqtt_thread_t)tcb;
}

static void ucos3_thread_destroy(mqtt_thread_t thread) {
    /* uC/OS-III thread_exit already deleted, no-op */
    (void)thread;
}

static void ucos3_thread_exit(void) {
    OS_ERR err;
    OSTaskDel(NULL, &err);
}

static uint32_t ucos3_get_time_ms(void) {
    OS_ERR err;
    return OSTimeGet(&err);
}

static void ucos3_sleep_ms(uint32_t ms) {
    OS_ERR err;
    OSTimeDlyHMSM(0, 0, 0, ms, OS_OPT_TIME_HMSM_STRICT, &err);
}

static const mqtt_os_api_t ucos3_os_api = {
    .malloc = ucos3_malloc,
    .free = ucos3_free,
    .mutex_create = ucos3_mutex_create,
    .mutex_destroy = ucos3_mutex_destroy,
    .mutex_lock = ucos3_mutex_lock,
    .mutex_unlock = ucos3_mutex_unlock,
    .sem_create = ucos3_sem_create,
    .sem_destroy = ucos3_sem_destroy,
    .sem_wait = ucos3_sem_wait,
    .sem_post = ucos3_sem_post,
    .thread_create = ucos3_thread_create,
    .thread_destroy = ucos3_thread_destroy,
    .thread_exit = ucos3_thread_exit,
    .get_time_ms = ucos3_get_time_ms,
    .sleep_ms = ucos3_sleep_ms
};

void mqtt_ucos3_init(void) {
    mqtt_os_init(&ucos3_os_api);
}
