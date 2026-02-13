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

static mqtt_socket_t posix_connect(const char* host, uint16_t port, uint32_t timeout_ms) {
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

static void posix_disconnect(mqtt_socket_t sock) {
    close((int)(intptr_t)sock);
}

static int posix_send(mqtt_socket_t sock, const uint8_t* buf, size_t len, uint32_t timeout_ms) {
    return send((int)(intptr_t)sock, buf, len, 0);
}

static int posix_recv(mqtt_socket_t sock, uint8_t* buf, size_t len, uint32_t timeout_ms) {
    return recv((int)(intptr_t)sock, buf, len, 0);
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
