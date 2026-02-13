# libmqtt - Lightweight MQTT Client Library

A minimal MQTT 3.1.1 client implementation designed for resource-constrained RTOS platforms.

## Features

- **Lightweight**: Core code uses <2KB RAM, suitable for embedded devices
- **Portable**: Fully abstracted OS and network layers, supports any RTOS
- **Zero Dependencies**: No third-party library dependencies
- **MQTT 3.1.1**: Supports QoS 0/1, CONNECT, PUBLISH, SUBSCRIBE, PING
- **Auto-Reconnect**: Automatic reconnection with subscription recovery
- **Thread-Safe**: Built-in mutex protection

## Architecture

```
include/           - Public API headers
  mqtt.h           - Main MQTT client API
  mqtt_os.h        - OS abstraction layer interface
  mqtt_net.h       - Network abstraction layer interface

src/core/          - Core MQTT implementation
  mqtt.c           - MQTT client logic
  mqtt_os.c        - OS abstraction layer
  mqtt_net.c       - Network abstraction layer

src/port/          - Platform-specific implementations
  posix_os.c       - POSIX OS implementation
  posix_net.c      - POSIX network implementation
  example_freertos.c - FreeRTOS example
  example_lwip.c   - lwIP network example

examples/          - Example applications
  demo.c           - Complete demo application
```

## Building

### Using CMake (Recommended)

```bash
mkdir build && cd build
cmake ..
make
```

### Using Make

```bash
make
```

## Quick Start

### 1. Implement OS Abstraction Layer

Implement the `mqtt_os_api_t` interface for your RTOS. See `src/port/posix_os.c` for reference.

```c
typedef struct {
    void* (*malloc)(size_t size);
    void (*free)(void* ptr);
    mqtt_mutex_t (*mutex_create)(void);
    void (*mutex_destroy)(mqtt_mutex_t mutex);
    int (*mutex_lock)(mqtt_mutex_t mutex, uint32_t timeout_ms);
    void (*mutex_unlock)(mqtt_mutex_t mutex);
    // ... more functions
} mqtt_os_api_t;

void mqtt_os_init(const mqtt_os_api_t* api);
```

### 2. Implement Network Abstraction Layer

Implement the `mqtt_net_api_t` interface for your network stack. See `src/port/posix_net.c` for reference.

```c
typedef struct {
    mqtt_socket_t (*connect)(const char* host, uint16_t port, uint32_t timeout_ms);
    void (*disconnect)(mqtt_socket_t sock);
    int (*send)(mqtt_socket_t sock, const uint8_t* buf, size_t len, uint32_t timeout_ms);
    int (*recv)(mqtt_socket_t sock, uint8_t* buf, size_t len, uint32_t timeout_ms);
} mqtt_net_api_t;

void mqtt_net_init(const mqtt_net_api_t* api);
```

### 3. Use the MQTT Client

```c
#include "mqtt.h"

// Message callback
void on_message(const char* topic, const uint8_t* payload, size_t len, void* user_data) {
    printf("Received: %s\n", topic);
}

int main(void) {
    // Initialize OS and network layers
    mqtt_posix_init();
    mqtt_posix_net_init();
    
    // Configure client
    mqtt_config_t config = {
        .host = "broker.emqx.io",
        .port = 1883,
        .client_id = "my_client",
        .username = NULL,
        .password = NULL,
        .keepalive = 60,
        .clean_session = 1,
        .msg_cb = on_message,
        .user_data = NULL
    };
    
    // Create and connect
    mqtt_client_t* client = mqtt_client_create(&config);
    mqtt_client_connect(client);
    
    // Subscribe
    mqtt_client_subscribe(client, "test/topic", 0);
    
    // Publish
    const char* msg = "Hello MQTT";
    mqtt_client_publish(client, "test/topic", (uint8_t*)msg, strlen(msg), 0);
    
    // Main loop (handles keep-alive and auto-reconnect)
    while (1) {
        mqtt_client_loop(client);
        sleep(1);
    }
    
    // Cleanup
    mqtt_client_disconnect(client);
    mqtt_client_destroy(client);
    
    return 0;
}
```

## API Reference

### Client Management

- `mqtt_client_create()` - Create MQTT client instance
- `mqtt_client_destroy()` - Destroy client instance
- `mqtt_client_connect()` - Connect to broker
- `mqtt_client_disconnect()` - Disconnect from broker
- `mqtt_client_is_connected()` - Check connection status

### Messaging

- `mqtt_client_subscribe()` - Subscribe to topic
- `mqtt_client_publish()` - Publish message
- `mqtt_client_loop()` - Maintain connection (must be called periodically)

### Features

- **Automatic Reconnection**: `mqtt_client_loop()` automatically reconnects on network failure
- **Subscription Recovery**: All subscriptions are automatically restored after reconnection
- **Keep-Alive**: Automatic PING messages to maintain connection

## Resource Usage

- **ROM**: ~8KB (code)
- **RAM**: ~2KB (client structure + buffers)
- **Threads**: 1 receive thread (optional)

## Supported Platforms

- **RTOS**: FreeRTOS, RT-Thread, ThreadX, Zephyr
- **Network**: lwIP, BSD sockets, custom TCP/IP stacks
- **Testing**: Linux, macOS, Unix (POSIX)

## Configuration

Edit `include/mqtt.h` to adjust:

```c
#define MQTT_MAX_PACKET_SIZE  1024  // Maximum packet size
#define MQTT_RECV_BUF_SIZE    1024  // Receive buffer size
#define MQTT_MAX_SUBSCRIPTIONS 8    // Max subscriptions to track
```

## Examples

See `examples/demo.c` for a complete working example.

Run the demo:
```bash
./mqtt_demo
```

## License

MIT License - see LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## Author

libmqtt development team
