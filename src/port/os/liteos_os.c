/**
 * @file liteos_os.c
 * @brief Huawei LiteOS OS abstraction layer implementation
 * 
 * LiteOS is a lightweight IoT operating system from Huawei.
 * This implementation provides OS abstraction for LiteOS kernel.
 */

#include "mqtt_os.h"
#include "los_task.h"
#include "los_sem.h"
#include "los_mux.h"
#include "los_memory.h"
#include "los_sys.h"
#include <stdlib.h>

static void* liteos_malloc(size_t size) {
    return LOS_MemAlloc(m_aucSysMem0, size);
}

static void liteos_free(void* ptr) {
    LOS_MemFree(m_aucSysMem0, ptr);
}

static mqtt_mutex_t liteos_mutex_create(void) {
    UINT32* mutex_id = malloc(sizeof(UINT32));
    LOS_MuxCreate(mutex_id);
    return (mqtt_mutex_t)mutex_id;
}

static void liteos_mutex_destroy(mqtt_mutex_t mutex) {
    LOS_MuxDelete(*(UINT32*)mutex);
    free(mutex);
}

static int liteos_mutex_lock(mqtt_mutex_t mutex) {
    return LOS_MuxPend(*(UINT32*)mutex, LOS_WAIT_FOREVER) == LOS_OK ? 0 : -1;
}

static void liteos_mutex_unlock(mqtt_mutex_t mutex) {
    LOS_MuxPost(*(UINT32*)mutex);
}

static mqtt_sem_t liteos_sem_create(uint32_t init_count) {
    UINT32* sem_id = malloc(sizeof(UINT32));
    LOS_SemCreate(init_count, sem_id);
    return (mqtt_sem_t)sem_id;
}

static void liteos_sem_destroy(mqtt_sem_t sem) {
    LOS_SemDelete(*(UINT32*)sem);
    free(sem);
}

static int liteos_sem_wait(mqtt_sem_t sem) {
    return LOS_SemPend(*(UINT32*)sem, LOS_WAIT_FOREVER) == LOS_OK ? 0 : -1;
}

static void liteos_sem_post(mqtt_sem_t sem) {
    LOS_SemPost(*(UINT32*)sem);
}

static mqtt_thread_t liteos_thread_create(mqtt_thread_func_t func, void* arg, 
                                          uint32_t stack_size, uint32_t priority) {
    UINT32* task_id = malloc(sizeof(UINT32));
    TSK_INIT_PARAM_S task_param = {0};
    task_param.pfnTaskEntry = (TSK_ENTRY_FUNC)func;
    task_param.uwStackSize = stack_size;
    task_param.pcName = "mqtt_task";
    task_param.usTaskPrio = priority;
    task_param.uwArg = (UINT32)arg;
    LOS_TaskCreate(task_id, &task_param);
    return (mqtt_thread_t)task_id;
}

static void liteos_thread_destroy(mqtt_thread_t thread) {
    /* LiteOS thread_exit already deleted, just free handle */
    free(thread);
}

static void liteos_thread_exit(void) {
    LOS_TaskDelete(LOS_CurTaskIDGet());
}

static uint32_t liteos_get_time_ms(void) {
    return LOS_TickCountGet() * 1000 / LOSCFG_BASE_CORE_TICK_PER_SECOND;
}

static void liteos_sleep_ms(uint32_t ms) {
    LOS_TaskDelay(LOS_MS2Tick(ms));
}

static const mqtt_os_api_t liteos_os_api = {
    .malloc = liteos_malloc,
    .free = liteos_free,
    .mutex_create = liteos_mutex_create,
    .mutex_destroy = liteos_mutex_destroy,
    .mutex_lock = liteos_mutex_lock,
    .mutex_unlock = liteos_mutex_unlock,
    .sem_create = liteos_sem_create,
    .sem_destroy = liteos_sem_destroy,
    .sem_wait = liteos_sem_wait,
    .sem_post = liteos_sem_post,
    .thread_create = liteos_thread_create,
    .thread_destroy = liteos_thread_destroy,
    .thread_exit = liteos_thread_exit,
    .get_time_ms = liteos_get_time_ms,
    .sleep_ms = liteos_sleep_ms
};

void mqtt_liteos_init(void) {
    mqtt_os_init(&liteos_os_api);
}
