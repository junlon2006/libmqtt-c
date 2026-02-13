#!/bin/bash

echo "Building MQTT TLS demo..."

gcc -Wall -O2 -I../include \
    ../src/core/mqtt.c \
    ../src/core/mqtt_os.c \
    ../src/core/mqtt_net.c \
    ../src/core/mqtt_tls.c \
    ../src/port/os/posix_os.c \
    ../src/port/net/posix_net.c \
    ../src/port/tls/openssl_tls.c \
    tls_demo.c \
    -lssl -lcrypto -lpthread \
    -o tls_demo

if [ $? -eq 0 ]; then
    echo "Build successful! Run with: ./tls_demo"
else
    echo "Build failed!"
    exit 1
fi
