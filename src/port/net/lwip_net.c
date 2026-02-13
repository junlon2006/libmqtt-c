/**
 * @file lwip_net.c
 * @brief lwIP network stack abstraction layer implementation
 * 
 * This file implements the network abstraction layer for lwIP TCP/IP stack.
 * Commonly used in embedded systems with custom network hardware.
 */

#include "mqtt_net.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <string.h>

static mqtt_socket_t lwip_connect(const char* host, uint16_t port, uint32_t timeout_ms) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return NULL;
    
    struct hostent* server = gethostbyname(host);
    if (!server) {
        close(sock);
        return NULL;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    struct timeval tv = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return NULL;
    }
    
    return (mqtt_socket_t)(intptr_t)sock;
}

static void lwip_disconnect(mqtt_socket_t sock) {
    close((int)(intptr_t)sock);
}

static int lwip_send(mqtt_socket_t sock, const uint8_t* buf, size_t len, uint32_t timeout_ms) {
    return send((int)(intptr_t)sock, buf, len, 0);
}

static int lwip_recv(mqtt_socket_t sock, uint8_t* buf, size_t len, uint32_t timeout_ms) {
    int fd = (int)(intptr_t)sock;
    fd_set readfds;
    struct timeval tv;
    
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    int ret = select(fd + 1, &readfds, NULL, NULL, &tv);
    if (ret <= 0) return ret;
    
    return recv(fd, buf, len, 0);
}

static const mqtt_net_api_t lwip_net_api = {
    .connect = lwip_connect,
    .disconnect = lwip_disconnect,
    .send = lwip_send,
    .recv = lwip_recv
};

void mqtt_lwip_init(void) {
    mqtt_net_init(&lwip_net_api);
}
