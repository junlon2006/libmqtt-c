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
#include <time.h>

static mqtt_socket_t posix_connect(const char* host, uint16_t port, uint32_t timeout_ms) {
    struct addrinfo hints, *result, *rp;
    struct timespec start, now;
    int64_t remaining_ms;
    mqtt_socket_t mqtt_sock = NULL;

    // 1. DNS resolution
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;          // Force IPv4
    hints.ai_socktype = SOCK_STREAM;

    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%u", port);

    if (getaddrinfo(host, port_str, &hints, &result) != 0) {
        return NULL;
    }

    // Record start time for total timeout control
    clock_gettime(CLOCK_MONOTONIC, &start);

    // 2. Iterate through address list
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        // 2.1 Calculate remaining timeout
        if (timeout_ms != UINT32_MAX) {  // UINT32_MAX means infinite wait
            clock_gettime(CLOCK_MONOTONIC, &now);
            int64_t elapsed_ms = (now.tv_sec - start.tv_sec) * 1000 +
                                 (now.tv_nsec - start.tv_nsec) / 1000000;
            remaining_ms = timeout_ms - elapsed_ms;
            if (remaining_ms <= 0) {
                // Total timeout exhausted, stop trying remaining addresses
                break;
            }
        } else {
            remaining_ms = -1;  // Infinite
        }

        // 2.2 Create new socket
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            continue;
        }

        // 2.3 Set to non-blocking mode
        int flags = fcntl(sock, F_GETFL, 0);
        if (flags < 0) {
            close(sock);
            continue;
        }
        if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
            close(sock);
            continue;
        }

        // 2.4 Initiate connection
        int ret = connect(sock, rp->ai_addr, rp->ai_addrlen);
        if (ret == 0) {
            // Immediate success (e.g., loopback address)
            fcntl(sock, F_SETFL, flags);  // Restore blocking mode
            mqtt_sock = (mqtt_socket_t)(intptr_t)sock;
            break;
        }

        if (ret < 0 && errno != EINPROGRESS) {
            // Connection failed immediately (e.g., network unreachable)
            close(sock);
            continue;
        }

        // 2.5 Wait for connection completion using select()
        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(sock, &writefds);

        struct timeval tv, *ptv = NULL;
        if (remaining_ms >= 0) {
            tv.tv_sec = remaining_ms / 1000;
            tv.tv_usec = (remaining_ms % 1000) * 1000;
            ptv = &tv;
        }

        int sel_ret;
        do {
            // Recalculate remaining time before each retry (handle time consumed by EINTR)
            if (timeout_ms != UINT32_MAX) {
                clock_gettime(CLOCK_MONOTONIC, &now);
                int64_t elapsed_ms = (now.tv_sec - start.tv_sec) * 1000 +
                                     (now.tv_nsec - start.tv_nsec) / 1000000;
                remaining_ms = timeout_ms - elapsed_ms;
                if (remaining_ms <= 0) {
                    sel_ret = 0;  // Total timeout exhausted
                    break;
                }
                tv.tv_sec = remaining_ms / 1000;
                tv.tv_usec = (remaining_ms % 1000) * 1000;
                ptv = &tv;
            }
            sel_ret = select(sock + 1, NULL, &writefds, NULL, ptv);
        } while (sel_ret < 0 && errno == EINTR);

        if (sel_ret < 0) {
            // select() error
            close(sock);
            continue;
        }

        if (sel_ret == 0) {
            // Connection timeout
            close(sock);
            continue;
        }

        // 2.6 Check final connection status
        int so_error;
        socklen_t len = sizeof(so_error);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) {
            close(sock);
            continue;
        }

        if (so_error == 0) {
            // Connection successful, restore blocking mode
            fcntl(sock, F_SETFL, flags);
            mqtt_sock = (mqtt_socket_t)(intptr_t)sock;
            break;
        }

        // Connection failed (e.g., connection refused)
        close(sock);
        // Continue trying next address
    }

    freeaddrinfo(result);
    return mqtt_sock;
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
