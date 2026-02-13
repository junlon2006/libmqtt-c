# Platform Porting Guide

This directory contains platform-specific implementations for different RTOS and network stacks.

## Directory Structure

```
src/port/
├── os/              - OS abstraction layer implementations
│   ├── posix_os.c       - POSIX (Linux/macOS/Unix)
│   ├── freertos_os.c    - FreeRTOS
│   ├── rtthread_os.c    - RT-Thread
│   ├── threadx_os.c     - ThreadX
│   ├── alios_os.c       - AliOS Things
│   ├── liteos_os.c      - Huawei LiteOS
│   ├── zephyr_os.c      - Zephyr RTOS
│   ├── mbed_os.c        - Arm Mbed OS
│   ├── cmsis_rtos2_os.c - CMSIS-RTOS2
│   ├── nuttx_os.c       - NuttX
│   ├── ucos3_os.c       - uC/OS-III
│   ├── riot_os.c        - RIOT OS
│   └── tencentos_tiny_os.c - TencentOS-tiny
├── net/             - Network abstraction layer implementations
│   ├── posix_net.c      - POSIX sockets (BSD)
│   └── lwip_net.c       - lwIP TCP/IP stack
└── tls/             - TLS/SSL abstraction layer implementations
    └── mbedtls_impl.c   - mbedTLS implementation
```

## Supported RTOS Platforms

### POSIX (Linux/macOS/Unix)
- **File**: `os/posix_os.c`
- **Init**: `mqtt_posix_init()`
- **Dependencies**: pthread, standard C library

### FreeRTOS
- **File**: `os/freertos_os.c`
- **Init**: `mqtt_freertos_init()`
- **Dependencies**: FreeRTOS kernel

### RT-Thread
- **File**: `os/rtthread_os.c`
- **Init**: `mqtt_rtthread_init()`
- **Dependencies**: RT-Thread kernel

### ThreadX (Azure RTOS)
- **File**: `os/threadx_os.c`
- **Init**: `mqtt_threadx_init()`
- **Dependencies**: Azure RTOS ThreadX

### AliOS Things
- **File**: `os/alios_os.c`
- **Init**: `mqtt_alios_init()`
- **Dependencies**: AliOS Things kernel

### Huawei LiteOS
- **File**: `os/liteos_os.c`
- **Init**: `mqtt_liteos_init()`
- **Dependencies**: LiteOS kernel

### Zephyr
- **File**: `os/zephyr_os.c`
- **Init**: `mqtt_zephyr_init()`
- **Dependencies**: Zephyr RTOS

### Mbed OS
- **File**: `os/mbed_os.c`
- **Init**: `mqtt_mbed_init()`
- **Dependencies**: Arm Mbed OS

### CMSIS-RTOS2
- **File**: `os/cmsis_rtos2_os.c`
- **Init**: `mqtt_cmsis_rtos2_init()`
- **Dependencies**: CMSIS-RTOS2 API

### NuttX
- **File**: `os/nuttx_os.c`
- **Init**: `mqtt_nuttx_init()`
- **Dependencies**: NuttX RTOS

### uC/OS-III
- **File**: `os/ucos3_os.c`
- **Init**: `mqtt_ucos3_init()`
- **Dependencies**: Micrium uC/OS-III

### RIOT OS
- **File**: `os/riot_os.c`
- **Init**: `mqtt_riot_init()`
- **Dependencies**: RIOT OS

### TencentOS-tiny
- **File**: `os/tencentos_tiny_os.c`
- **Init**: `mqtt_tencentos_tiny_init()`
- **Dependencies**: TencentOS-tiny kernel

## Supported Network Stacks

### POSIX Sockets (BSD)
- **File**: `net/posix_net.c`
- **Init**: `mqtt_posix_net_init()`
- **Dependencies**: Standard BSD sockets API

### lwIP
- **File**: `net/lwip_net.c`
- **Init**: `mqtt_lwip_init()`
- **Dependencies**: lwIP TCP/IP stack

## Supported TLS Libraries

### mbedTLS
- **File**: `tls/mbedtls_impl.c`
- **Init**: `mqtt_mbedtls_init()`
- **Dependencies**: mbedTLS library
- **Best for**: Embedded systems, IoT devices

## Porting to New Platform

### 1. Implement OS Abstraction Layer

Create a new file `src/port/os/your_rtos_os.c` and implement all functions in `mqtt_os_api_t`:

```c
#include "mqtt_os.h"
#include "your_rtos.h"

// Implement all required functions
static void* your_malloc(size_t size) { ... }
static void your_free(void* ptr) { ... }
// ... more functions

static const mqtt_os_api_t your_os_api = {
    .malloc = your_malloc,
    .free = your_free,
    // ... all function pointers
};

void mqtt_your_rtos_init(void) {
    mqtt_os_init(&your_os_api);
}
```

### 2. Implement Network Abstraction Layer

Create a new file `src/port/net/your_stack_net.c` and implement all functions in `mqtt_net_api_t`:

```c
#include "mqtt_net.h"
#include "your_network_stack.h"

// Implement all required functions
static mqtt_socket_t your_connect(const char* host, uint16_t port, uint32_t timeout_ms) { ... }
// ... more functions

static const mqtt_net_api_t your_net_api = {
    .connect = your_connect,
    .disconnect = your_disconnect,
    .send = your_send,
    .recv = your_recv
};

void mqtt_your_stack_init(void) {
    mqtt_net_init(&your_net_api);
}
```

### 3. Use in Your Application

```c
#include "mqtt.h"

int main(void) {
    // Initialize OS and network layers
    mqtt_your_rtos_init();
    mqtt_your_stack_init();
    
    // Use MQTT client as normal
    mqtt_config_t config = { ... };
    mqtt_client_t* client = mqtt_client_create(&config);
    // ...
}
```

## Testing

Each port should be tested with the demo application to ensure:
- Memory allocation/deallocation works correctly
- Thread creation and synchronization primitives function properly
- Network connectivity and data transfer is reliable
- Automatic reconnection works as expected
