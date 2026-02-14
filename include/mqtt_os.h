/**
 * @file mqtt_os.h
 * @brief MQTT OS abstraction layer interface
 * 
 * This header defines the OS abstraction layer for the MQTT client library.
 * Users must implement these interfaces to port the library to different RTOS platforms.
 */

#ifndef MQTT_OS_H
#define MQTT_OS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque mutex handle */
typedef void* mqtt_mutex_t;

/** @brief Opaque semaphore handle */
typedef void* mqtt_sem_t;

/** @brief Opaque thread handle */
typedef void* mqtt_thread_t;

/** @brief Thread function prototype */
typedef void (*mqtt_thread_func_t)(void* arg);

/**
 * @brief OS abstraction layer API structure
 * 
 * This structure contains function pointers for all OS-dependent operations.
 * Users must implement all these functions and register them via mqtt_os_init().
 */
typedef struct {
    /** @brief Allocate memory */
    void* (*malloc)(size_t size);
    
    /** @brief Free memory */
    void (*free)(void* ptr);
    
    /** @brief Create a mutex */
    mqtt_mutex_t (*mutex_create)(void);
    
    /** @brief Destroy a mutex */
    void (*mutex_destroy)(mqtt_mutex_t mutex);
    
    /** @brief Lock a mutex (blocking)
     *  @param mutex Mutex handle
     *  @return 0 on success, -1 on failure
     */
    int (*mutex_lock)(mqtt_mutex_t mutex);
    
    /** @brief Unlock a mutex */
    void (*mutex_unlock)(mqtt_mutex_t mutex);
    
    /** @brief Create a semaphore
     *  @param init_count Initial count value
     *  @return Semaphore handle
     */
    mqtt_sem_t (*sem_create)(uint32_t init_count);
    
    /** @brief Destroy a semaphore */
    void (*sem_destroy)(mqtt_sem_t sem);
    
    /** @brief Wait on a semaphore (blocking)
     *  @param sem Semaphore handle
     *  @return 0 on success, -1 on failure
     */
    int (*sem_wait)(mqtt_sem_t sem);
    
    /** @brief Post to a semaphore */
    void (*sem_post)(mqtt_sem_t sem);
    
    /** @brief Create a thread
     *  @param func Thread function
     *  @param arg Thread argument
     *  @param stack_size Stack size in bytes
     *  @param priority Thread priority
     *  @return Thread handle
     */
    mqtt_thread_t (*thread_create)(mqtt_thread_func_t func, void* arg, 
                                   uint32_t stack_size, uint32_t priority);
    
    /** @brief Destroy a thread */
    void (*thread_destroy)(mqtt_thread_t thread);
    
    /** @brief Exit current thread (called by thread itself) */
    void (*thread_exit)(void);
    
    /** @brief Get current time in milliseconds */
    uint32_t (*get_time_ms)(void);
    
    /** @brief Sleep for specified milliseconds */
    void (*sleep_ms)(uint32_t ms);
} mqtt_os_api_t;

/**
 * @brief Initialize OS abstraction layer
 * @param api Pointer to OS API structure
 */
void mqtt_os_init(const mqtt_os_api_t* api);

/**
 * @brief Get current OS API
 * @return Pointer to registered OS API structure
 */
const mqtt_os_api_t* mqtt_os_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_OS_H */
