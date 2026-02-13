# TLS/SSL Support

libmqtt provides TLS/SSL support through an abstraction layer.

## Usage

```c
mqtt_config_t config = {
    .host = "mqtt.example.com",
    .port = 8883,
    .use_tls = 1,
    .tls_config = &tls_config
};
```

See src/port/tls/ for implementations.
