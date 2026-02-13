/**
 * @file posix_net.c
 * @brief POSIX platform network abstraction layer implementation
 * 
 * This file implements the network abstraction layer for POSIX-compliant systems
 * using standard BSD sockets API.
 */

#include "mqtt_net.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

static mqtt_socket_t posix_connect(const char* host, uint16_t port, uint32_t timeout_ms) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return NULL;
    
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%u", port);
    
    if (getaddrinfo(host, port_str, &hints, &result) != 0) {
        close(sock);
        return NULL;
    }
    
    int connected = -1;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
            connected = 0;
            break;
        }
    }
    
    freeaddrinfo(result);
    
    if (connected != 0) {
        close(sock);
        return NULL;
    }
    
    return (mqtt_socket_t)(intptr_t)sock;
}

static void posix_disconnect(mqtt_socket_t sock) {
    close((int)(intptr_t)sock);
}

static int posix_send(mqtt_socket_t sock, const uint8_t* buf, size_t len, uint32_t timeout_ms) {
    int fd = (int)(intptr_t)sock;
    ssize_t ret;
    do {
        ret = send(fd, buf, len, 0);
    } while (ret < 0 && errno == EINTR);
    return ret;
}

static int posix_recv(mqtt_socket_t sock, uint8_t* buf, size_t len, uint32_t timeout_ms) {
    int fd = (int)(intptr_t)sock;
    fd_set readfds;
    struct timeval tv;
    int ret;
    
    do {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        
        ret = select(fd + 1, &readfds, NULL, NULL, &tv);
    } while (ret < 0 && errno == EINTR);
    
    if (ret <= 0) return ret;
    
    do {
        ret = recv(fd, buf, len, 0);
    } while (ret < 0 && errno == EINTR);
    
    return ret;
}

static const mqtt_net_api_t posix_net_api = {
    .connect = posix_connect,
    .disconnect = posix_disconnect,
    .send = posix_send,
    .recv = posix_recv
};

void mqtt_posix_net_init(void) {
    mqtt_net_init(&posix_net_api);
}
