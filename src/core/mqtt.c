/**
 * @file mqtt.c
 * @brief MQTT core implementation
 */

#include "mqtt.h"
#include <string.h>

#define MQTT_CONNECT     1
#define MQTT_CONNACK     2
#define MQTT_PUBLISH     3
#define MQTT_SUBSCRIBE   8
#define MQTT_SUBACK      9
#define MQTT_PINGREQ     12
#define MQTT_PINGRESP    13
#define MQTT_DISCONNECT  14

#define MQTT_PROTOCOL_LEVEL         4
#define MQTT_PROTOCOL_NAME          "MQTT"
#define MQTT_PROTOCOL_NAME_LEN      4
#define MQTT_CONNECT_FIXED_HEADER   10
#define MQTT_CLEAN_SESSION_FLAG     0x02
#define MQTT_USERNAME_FLAG          0x80
#define MQTT_PASSWORD_FLAG          0x40
#define MQTT_SUBSCRIBE_FLAGS        0x02
#define MQTT_QOS_MASK               0x03
#define MQTT_QOS_SHIFT              1
#define MQTT_PACKET_TYPE_SHIFT      4
#define MQTT_REMAINING_LENGTH_MASK  0x80
#define MQTT_REMAINING_LENGTH_MAX   4
#define MQTT_BYTE_MASK              127
#define MQTT_BYTE_MULTIPLIER        128
#define MQTT_CONNACK_MIN_LEN        4
#define MQTT_CONNACK_RC_OFFSET      3
#define MQTT_TOPIC_BUFFER_SIZE      128
#define MQTT_PACKET_ID_SIZE         2
#define MQTT_TOPIC_LEN_SIZE         2
#define MQTT_INITIAL_PACKET_ID      1
#define MQTT_CONNECT_TIMEOUT_MS     5000
#define MQTT_RECONNECT_DELAY_MS     1000
#define MQTT_RECV_TIMEOUT_MS        2000
#define MQTT_RECV_THREAD_STACK      2048
#define MQTT_RECV_THREAD_PRIORITY   5
#define MQTT_KEEPALIVE_DIVISOR      2
#define MQTT_MS_PER_SECOND          1000

static void mqtt_recv_thread(void* arg);

static int encode_remaining_length(uint8_t* buf, size_t len) {
    int count = 0;
    do {
        uint8_t byte = len % MQTT_BYTE_MULTIPLIER;
        len /= MQTT_BYTE_MULTIPLIER;
        if (len > 0) byte |= MQTT_REMAINING_LENGTH_MASK;
        buf[count++] = byte;
    } while (len > 0);
    return count;
}

static int decode_remaining_length(const uint8_t* buf, size_t* len) {
    int multiplier = 1, count = 0;
    *len = 0;
    uint8_t byte;
    do {
        if (count >= MQTT_REMAINING_LENGTH_MAX) return -1;
        byte = buf[count++];
        *len += (byte & MQTT_BYTE_MASK) * multiplier;
        multiplier *= MQTT_BYTE_MULTIPLIER;
    } while ((byte & MQTT_REMAINING_LENGTH_MASK) != 0);
    return count;
}

static int pack_connect(uint8_t* buf, const char* client_id, const char* username,
                        const char* password, uint16_t keepalive, uint8_t clean_session) {
    int pos = 0;
    size_t cid_len = strlen(client_id);
    size_t payload_len = MQTT_TOPIC_LEN_SIZE + cid_len;
    uint8_t flags = clean_session ? MQTT_CLEAN_SESSION_FLAG : 0x00;
    
    if (username) {
        payload_len += MQTT_TOPIC_LEN_SIZE + strlen(username);
        flags |= MQTT_USERNAME_FLAG;
    }
    if (password) {
        payload_len += MQTT_TOPIC_LEN_SIZE + strlen(password);
        flags |= MQTT_PASSWORD_FLAG;
    }
    
    size_t remaining = MQTT_CONNECT_FIXED_HEADER + payload_len;
    uint8_t rem_buf[MQTT_REMAINING_LENGTH_MAX];
    int rem_len = encode_remaining_length(rem_buf, remaining);
    
    buf[pos++] = MQTT_CONNECT << MQTT_PACKET_TYPE_SHIFT;
    memcpy(buf + pos, rem_buf, rem_len);
    pos += rem_len;
    
    buf[pos++] = 0; buf[pos++] = MQTT_PROTOCOL_NAME_LEN;
    memcpy(buf + pos, MQTT_PROTOCOL_NAME, MQTT_PROTOCOL_NAME_LEN); pos += MQTT_PROTOCOL_NAME_LEN;
    buf[pos++] = MQTT_PROTOCOL_LEVEL;
    buf[pos++] = flags;
    buf[pos++] = keepalive >> 8;
    buf[pos++] = keepalive & 0xFF;
    
    buf[pos++] = cid_len >> 8;
    buf[pos++] = cid_len & 0xFF;
    memcpy(buf + pos, client_id, cid_len);
    pos += cid_len;
    
    if (username) {
        size_t len = strlen(username);
        buf[pos++] = len >> 8;
        buf[pos++] = len & 0xFF;
        memcpy(buf + pos, username, len);
        pos += len;
    }
    if (password) {
        size_t len = strlen(password);
        buf[pos++] = len >> 8;
        buf[pos++] = len & 0xFF;
        memcpy(buf + pos, password, len);
        pos += len;
    }
    
    return pos;
}

static int pack_publish(uint8_t* buf, const char* topic, const uint8_t* payload,
                        size_t payload_len, uint8_t qos, uint16_t packet_id) {
    int pos = 0;
    size_t topic_len = strlen(topic);
    size_t remaining = MQTT_TOPIC_LEN_SIZE + topic_len + payload_len;
    if (qos > 0) remaining += MQTT_PACKET_ID_SIZE;
    
    uint8_t rem_buf[MQTT_REMAINING_LENGTH_MAX];
    int rem_len = encode_remaining_length(rem_buf, remaining);
    
    buf[pos++] = (MQTT_PUBLISH << MQTT_PACKET_TYPE_SHIFT) | (qos << MQTT_QOS_SHIFT);
    memcpy(buf + pos, rem_buf, rem_len);
    pos += rem_len;
    
    buf[pos++] = topic_len >> 8;
    buf[pos++] = topic_len & 0xFF;
    memcpy(buf + pos, topic, topic_len);
    pos += topic_len;
    
    if (qos > 0) {
        buf[pos++] = packet_id >> 8;
        buf[pos++] = packet_id & 0xFF;
    }
    
    memcpy(buf + pos, payload, payload_len);
    pos += payload_len;
    
    return pos;
}

static int pack_subscribe(uint8_t* buf, const char* topic, uint8_t qos, uint16_t packet_id) {
    int pos = 0;
    size_t topic_len = strlen(topic);
    size_t remaining = MQTT_PACKET_ID_SIZE + MQTT_TOPIC_LEN_SIZE + topic_len + 1;
    
    uint8_t rem_buf[MQTT_REMAINING_LENGTH_MAX];
    int rem_len = encode_remaining_length(rem_buf, remaining);
    
    buf[pos++] = (MQTT_SUBSCRIBE << MQTT_PACKET_TYPE_SHIFT) | MQTT_SUBSCRIBE_FLAGS;
    memcpy(buf + pos, rem_buf, rem_len);
    pos += rem_len;
    
    buf[pos++] = packet_id >> 8;
    buf[pos++] = packet_id & 0xFF;
    buf[pos++] = topic_len >> 8;
    buf[pos++] = topic_len & 0xFF;
    memcpy(buf + pos, topic, topic_len);
    pos += topic_len;
    buf[pos++] = qos;
    
    return pos;
}

static int pack_pingreq(uint8_t* buf) {
    buf[0] = MQTT_PINGREQ << MQTT_PACKET_TYPE_SHIFT;
    buf[1] = 0;
    return 2;
}

static int pack_disconnect(uint8_t* buf) {
    buf[0] = MQTT_DISCONNECT << MQTT_PACKET_TYPE_SHIFT;
    buf[1] = 0;
    return 2;
}

mqtt_client_t* mqtt_client_create(const mqtt_config_t* config) {
    const mqtt_os_api_t* os = mqtt_os_get();
    const mqtt_net_api_t* net = mqtt_net_get();
    if (!os || !net) return NULL;
    
    mqtt_client_t* client = (mqtt_client_t*)os->malloc(sizeof(mqtt_client_t));
    if (!client) return NULL;
    
    memset(client, 0, sizeof(mqtt_client_t));
    memcpy(&client->config, config, sizeof(mqtt_config_t));
    client->state = MQTT_STATE_DISCONNECTED;
    client->packet_id = MQTT_INITIAL_PACKET_ID;
    client->sub_count = 0;
    
    client->mutex = os->mutex_create();
    if (!client->mutex) goto err_free_client;
    
    client->thread_exit_sem = os->sem_create(0);
    if (!client->thread_exit_sem) goto err_destroy_mutex;
    
    client->socket = net->connect(client->config.host, client->config.port, MQTT_CONNECT_TIMEOUT_MS);
    if (!client->socket) goto err_destroy_sem;
    
    int len = pack_connect(client->send_buf, client->config.client_id, client->config.username,
                           client->config.password, client->config.keepalive, client->config.clean_session);
    
    if (net->send(client->socket, client->send_buf, len) != len) goto err_disconnect;
    
    int recv_len = net->recv(client->socket, client->recv_buf, MQTT_RECV_BUF_SIZE, MQTT_RECV_TIMEOUT_MS);
    if (recv_len < MQTT_CONNACK_MIN_LEN || (client->recv_buf[0] >> MQTT_PACKET_TYPE_SHIFT) != MQTT_CONNACK || client->recv_buf[MQTT_CONNACK_RC_OFFSET] != 0) {
        goto err_disconnect;
    }
    
    client->state = MQTT_STATE_CONNECTED;
    client->last_ping_time = os->get_time_ms();
    client->running = 1;
    
    client->recv_thread = os->thread_create(mqtt_recv_thread, client, MQTT_RECV_THREAD_STACK, MQTT_RECV_THREAD_PRIORITY);
    if (!client->recv_thread) goto err_disconnect;
    
    return client;

err_disconnect:
    net->disconnect(client->socket);
err_destroy_sem:
    os->sem_destroy(client->thread_exit_sem);
err_destroy_mutex:
    os->mutex_destroy(client->mutex);
err_free_client:
    os->free(client);
    return NULL;
}

void mqtt_client_destroy(mqtt_client_t* client) {
    if (!client) return;
    
    const mqtt_os_api_t* os = mqtt_os_get();
    const mqtt_net_api_t* net = mqtt_net_get();
    
    client->running = 0;
    
    if (client->recv_thread) {
        os->sem_wait(client->thread_exit_sem);
        os->thread_destroy(client->recv_thread);
    }
    
    if (client->socket) {
        int len = pack_disconnect(client->send_buf);
        net->send(client->socket, client->send_buf, len);
        net->disconnect(client->socket);
    }
    
    if (client->thread_exit_sem) os->sem_destroy(client->thread_exit_sem);
    if (client->mutex) os->mutex_destroy(client->mutex);
    os->free(client);
}

int mqtt_client_subscribe(mqtt_client_t* client, const char* topic, uint8_t qos) {
    if (!client || client->state != MQTT_STATE_CONNECTED) return -1;
    
    const mqtt_os_api_t* os = mqtt_os_get();
    const mqtt_net_api_t* net = mqtt_net_get();
    
    os->mutex_lock(client->mutex);
    
    int len = pack_subscribe(client->send_buf, topic, qos, client->packet_id++);
    int ret = net->send(client->socket, client->send_buf, len);
    
    if (ret == len && client->sub_count < MQTT_MAX_SUBSCRIPTIONS) {
        int found = 0;
        for (int i = 0; i < client->sub_count; i++) {
            if (strcmp(client->subscriptions[i].topic, topic) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            strncpy(client->subscriptions[client->sub_count].topic, topic, sizeof(client->subscriptions[0].topic) - 1);
            client->subscriptions[client->sub_count].qos = qos;
            client->sub_count++;
        }
    }
    
    os->mutex_unlock(client->mutex);
    
    return (ret == len) ? 0 : -1;
}

int mqtt_client_publish(mqtt_client_t* client, const char* topic, const uint8_t* payload,
                        size_t len, uint8_t qos) {
    if (!client || client->state != MQTT_STATE_CONNECTED) return -1;
    
    const mqtt_os_api_t* os = mqtt_os_get();
    const mqtt_net_api_t* net = mqtt_net_get();
    
    os->mutex_lock(client->mutex);
    
    uint16_t packet_id = (qos > 0) ? client->packet_id++ : 0;
    int pkt_len = pack_publish(client->send_buf, topic, payload, len, qos, packet_id);
    int ret = net->send(client->socket, client->send_buf, pkt_len);
    
    os->mutex_unlock(client->mutex);
    
    return (ret == pkt_len) ? 0 : -1;
}

int mqtt_client_is_connected(mqtt_client_t* client) {
    return client && client->state == MQTT_STATE_CONNECTED;
}

static int mqtt_try_reconnect(mqtt_client_t* client) {
    const mqtt_net_api_t* net = mqtt_net_get();
    const mqtt_os_api_t* os = mqtt_os_get();
    int len;
    
    client->socket = net->connect(client->config.host, client->config.port, MQTT_CONNECT_TIMEOUT_MS);
    if (!client->socket) return -1;
    
    os->mutex_lock(client->mutex);
    
    len = pack_connect(client->send_buf, client->config.client_id, client->config.username,
                       client->config.password, client->config.keepalive, client->config.clean_session);
    
    if (net->send(client->socket, client->send_buf, len) != len) goto err_cleanup;
    
    int recv_len = net->recv(client->socket, client->recv_buf, MQTT_RECV_BUF_SIZE, MQTT_RECV_TIMEOUT_MS);
    if (recv_len < MQTT_CONNACK_MIN_LEN || (client->recv_buf[0] >> MQTT_PACKET_TYPE_SHIFT) != MQTT_CONNACK || client->recv_buf[MQTT_CONNACK_RC_OFFSET] != 0) {
        goto err_cleanup;
    }
    
    for (int i = 0; i < client->sub_count; i++) {
        len = pack_subscribe(client->send_buf, client->subscriptions[i].topic,
                             client->subscriptions[i].qos, client->packet_id++);
        if (net->send(client->socket, client->send_buf, len) != len) goto err_cleanup;
    }
    
    client->state = MQTT_STATE_CONNECTED;
    client->last_ping_time = os->get_time_ms();
    client->waiting_pingresp = 0;
    
    os->mutex_unlock(client->mutex);
    return 0;

err_cleanup:
    net->disconnect(client->socket);
    client->socket = NULL;
    os->mutex_unlock(client->mutex);
    return -1;
}

static int mqtt_send_ping(mqtt_client_t* client) {
    const mqtt_net_api_t* net = mqtt_net_get();
    const mqtt_os_api_t* os = mqtt_os_get();
    
    uint32_t now = os->get_time_ms();
    uint32_t keepalive_ms = client->config.keepalive * MQTT_MS_PER_SECOND;
    
    if (client->waiting_pingresp) {
        if (now - client->ping_sent_time >= keepalive_ms / MQTT_KEEPALIVE_DIVISOR) {
            os->mutex_lock(client->mutex);
            client->state = MQTT_STATE_DISCONNECTED;
            client->waiting_pingresp = 0;
            net->disconnect(client->socket);
            client->socket = NULL;
            os->mutex_unlock(client->mutex);
            return -1;
        }
        return 0;
    }
    
    if (now - client->last_ping_time < keepalive_ms / MQTT_KEEPALIVE_DIVISOR) {
        return 0;
    }
    
    os->mutex_lock(client->mutex);
    
    int len = pack_pingreq(client->send_buf);
    if (net->send(client->socket, client->send_buf, len) != len) {
        client->state = MQTT_STATE_DISCONNECTED;
        net->disconnect(client->socket);
        client->socket = NULL;
        os->mutex_unlock(client->mutex);
        return -1;
    }
    client->ping_sent_time = now;
    client->waiting_pingresp = 1;
    
    os->mutex_unlock(client->mutex);
    return 0;
}

static void mqtt_handle_publish(mqtt_client_t* client, int len) {
    if (!client->config.msg_cb) return;
    
    size_t remaining;
    int offset = 1 + decode_remaining_length(client->recv_buf + 1, &remaining);
    
    uint16_t topic_len = (client->recv_buf[offset] << 8) | client->recv_buf[offset + 1];
    offset += MQTT_TOPIC_LEN_SIZE;
    
    char topic[MQTT_TOPIC_BUFFER_SIZE];
    if (topic_len >= sizeof(topic)) return;
    
    memcpy(topic, client->recv_buf + offset, topic_len);
    topic[topic_len] = '\0';
    offset += topic_len;
    
    uint8_t qos = (client->recv_buf[0] >> MQTT_QOS_SHIFT) & MQTT_QOS_MASK;
    if (qos > 0) offset += MQTT_PACKET_ID_SIZE;
    
    size_t payload_len = len - offset;
    client->config.msg_cb(topic, client->recv_buf + offset, payload_len, client->config.user_data);
}

static void mqtt_recv_thread(void* arg) {
    mqtt_client_t* client = (mqtt_client_t*)arg;
    const mqtt_net_api_t* net = mqtt_net_get();
    const mqtt_os_api_t* os = mqtt_os_get();
    
    while (client->running) {
        if (client->state == MQTT_STATE_DISCONNECTED) {
            if (mqtt_try_reconnect(client) != 0) {
                os->sleep_ms(MQTT_RECONNECT_DELAY_MS);
            }
            continue;
        }
        
        if (mqtt_send_ping(client) != 0) {
            continue;
        }
        
        int len = net->recv(client->socket, client->recv_buf, MQTT_RECV_BUF_SIZE, MQTT_RECV_TIMEOUT_MS);
        
        if (len < 0) {
            os->mutex_lock(client->mutex);
            if (client->state == MQTT_STATE_CONNECTED) {
                client->state = MQTT_STATE_DISCONNECTED;
                net->disconnect(client->socket);
                client->socket = NULL;
            }
            os->mutex_unlock(client->mutex);
            continue;
        }
        if (len == 0) continue;
        
        uint8_t type = client->recv_buf[0] >> MQTT_PACKET_TYPE_SHIFT;
        
        if (type == MQTT_PINGRESP) {
            client->last_ping_time = os->get_time_ms();
            client->waiting_pingresp = 0;
            continue;
        }
        
        if (type == MQTT_PUBLISH) {
            mqtt_handle_publish(client, len);
        }
    }
    
    /* Signal thread exit completion */
    os->sem_post(client->thread_exit_sem);
    
    /* Call thread_exit - RTOS implementation handles cleanup properly */
    if (os->thread_exit) {
        os->thread_exit();
    }
}
