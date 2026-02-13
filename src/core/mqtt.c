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

static void mqtt_recv_thread(void* arg);

static int encode_remaining_length(uint8_t* buf, size_t len) {
    int count = 0;
    do {
        uint8_t byte = len % 128;
        len /= 128;
        if (len > 0) byte |= 0x80;
        buf[count++] = byte;
    } while (len > 0);
    return count;
}

static int decode_remaining_length(const uint8_t* buf, size_t* len) {
    int multiplier = 1, count = 0;
    *len = 0;
    uint8_t byte;
    do {
        if (count >= 4) return -1;
        byte = buf[count++];
        *len += (byte & 127) * multiplier;
        multiplier *= 128;
    } while ((byte & 128) != 0);
    return count;
}

static int pack_connect(uint8_t* buf, const char* client_id, const char* username,
                        const char* password, uint16_t keepalive, uint8_t clean_session) {
    int pos = 0;
    size_t cid_len = strlen(client_id);
    size_t payload_len = 2 + cid_len;
    uint8_t flags = clean_session ? 0x02 : 0x00;
    
    if (username) {
        payload_len += 2 + strlen(username);
        flags |= 0x80;
    }
    if (password) {
        payload_len += 2 + strlen(password);
        flags |= 0x40;
    }
    
    size_t remaining = 10 + payload_len;
    uint8_t rem_buf[4];
    int rem_len = encode_remaining_length(rem_buf, remaining);
    
    buf[pos++] = MQTT_CONNECT << 4;
    memcpy(buf + pos, rem_buf, rem_len);
    pos += rem_len;
    
    buf[pos++] = 0; buf[pos++] = 4;
    memcpy(buf + pos, "MQTT", 4); pos += 4;
    buf[pos++] = 4;
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
    size_t remaining = 2 + topic_len + payload_len;
    if (qos > 0) remaining += 2;
    
    uint8_t rem_buf[4];
    int rem_len = encode_remaining_length(rem_buf, remaining);
    
    buf[pos++] = (MQTT_PUBLISH << 4) | (qos << 1);
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
    size_t remaining = 2 + 2 + topic_len + 1;
    
    uint8_t rem_buf[4];
    int rem_len = encode_remaining_length(rem_buf, remaining);
    
    buf[pos++] = (MQTT_SUBSCRIBE << 4) | 0x02;
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
    buf[0] = MQTT_PINGREQ << 4;
    buf[1] = 0;
    return 2;
}

static int pack_disconnect(uint8_t* buf) {
    buf[0] = MQTT_DISCONNECT << 4;
    buf[1] = 0;
    return 2;
}

mqtt_client_t* mqtt_client_create(const mqtt_config_t* config) {
    const mqtt_os_api_t* os = mqtt_os_get();
    if (!os) return NULL;
    
    mqtt_client_t* client = (mqtt_client_t*)os->malloc(sizeof(mqtt_client_t));
    if (!client) return NULL;
    
    memset(client, 0, sizeof(mqtt_client_t));
    memcpy(&client->config, config, sizeof(mqtt_config_t));
    client->state = MQTT_STATE_DISCONNECTED;
    client->packet_id = 1;
    client->sub_count = 0;
    client->mutex = os->mutex_create();
    
    return client;
}

void mqtt_client_destroy(mqtt_client_t* client) {
    if (!client) return;
    
    const mqtt_os_api_t* os = mqtt_os_get();
    mqtt_client_disconnect(client);
    
    if (client->mutex) os->mutex_destroy(client->mutex);
    os->free(client);
}

int mqtt_client_connect(mqtt_client_t* client) {
    if (!client) return -1;
    
    const mqtt_os_api_t* os = mqtt_os_get();
    const mqtt_net_api_t* net = mqtt_net_get();
    
    os->mutex_lock(client->mutex, 0xFFFFFFFF);
    
    client->socket = net->connect(client->config.host, client->config.port, 5000);
    if (!client->socket) {
        os->mutex_unlock(client->mutex);
        return -1;
    }
    
    int len = pack_connect(client->send_buf, client->config.client_id, client->config.username,
                          client->config.password, client->config.keepalive, client->config.clean_session);
    
    if (net->send(client->socket, client->send_buf, len, 5000) != len) {
        net->disconnect(client->socket);
        os->mutex_unlock(client->mutex);
        return -1;
    }
    
    int recv_len = net->recv(client->socket, client->recv_buf, MQTT_RECV_BUF_SIZE, 5000);
    if (recv_len < 4 || (client->recv_buf[0] >> 4) != MQTT_CONNACK || client->recv_buf[3] != 0) {
        net->disconnect(client->socket);
        os->mutex_unlock(client->mutex);
        return -1;
    }
    
    client->state = MQTT_STATE_CONNECTED;
    client->last_ping_time = os->get_time_ms();
    client->running = 1;
    
    client->recv_thread = os->thread_create(mqtt_recv_thread, client, 2048, 5);
    
    os->mutex_unlock(client->mutex);
    return 0;
}

void mqtt_client_disconnect(mqtt_client_t* client) {
    if (!client || client->state == MQTT_STATE_DISCONNECTED) return;
    
    const mqtt_os_api_t* os = mqtt_os_get();
    const mqtt_net_api_t* net = mqtt_net_get();
    
    os->mutex_lock(client->mutex, 0xFFFFFFFF);
    
    client->running = 0;
    
    int len = pack_disconnect(client->send_buf);
    net->send(client->socket, client->send_buf, len, 1000);
    
    net->disconnect(client->socket);
    client->state = MQTT_STATE_DISCONNECTED;
    
    os->mutex_unlock(client->mutex);
    
    if (client->recv_thread) {
        os->thread_destroy(client->recv_thread);
        client->recv_thread = NULL;
    }
}

int mqtt_client_subscribe(mqtt_client_t* client, const char* topic, uint8_t qos) {
    if (!client || client->state != MQTT_STATE_CONNECTED) return -1;
    
    const mqtt_os_api_t* os = mqtt_os_get();
    const mqtt_net_api_t* net = mqtt_net_get();
    
    os->mutex_lock(client->mutex, 0xFFFFFFFF);
    
    int len = pack_subscribe(client->send_buf, topic, qos, client->packet_id++);
    int ret = net->send(client->socket, client->send_buf, len, 5000);
    
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
    
    os->mutex_lock(client->mutex, 0xFFFFFFFF);
    
    uint16_t packet_id = (qos > 0) ? client->packet_id++ : 0;
    int pkt_len = pack_publish(client->send_buf, topic, payload, len, qos, packet_id);
    int ret = net->send(client->socket, client->send_buf, pkt_len, 5000);
    
    os->mutex_unlock(client->mutex);
    
    return (ret == pkt_len) ? 0 : -1;
}

int mqtt_client_loop(mqtt_client_t* client) {
    if (!client) return -1;
    
    const mqtt_os_api_t* os = mqtt_os_get();
    const mqtt_net_api_t* net = mqtt_net_get();
    
    if (client->state != MQTT_STATE_CONNECTED) {
        client->socket = net->connect(client->config.host, client->config.port, 5000);
            if (!client->socket) return -1;
            
            int len = pack_connect(client->send_buf, client->config.client_id, client->config.username,
                                  client->config.password, client->config.keepalive, client->config.clean_session);
            
            if (net->send(client->socket, client->send_buf, len, 5000) != len) {
                net->disconnect(client->socket);
                return -1;
            }
            
            int recv_len = net->recv(client->socket, client->recv_buf, MQTT_RECV_BUF_SIZE, 5000);
            if (recv_len < 4 || (client->recv_buf[0] >> 4) != MQTT_CONNACK || client->recv_buf[3] != 0) {
                net->disconnect(client->socket);
                return -1;
            }
            
            client->state = MQTT_STATE_CONNECTED;
            client->last_ping_time = os->get_time_ms();
            client->running = 1;
            
            for (int i = 0; i < client->sub_count; i++) {
                len = pack_subscribe(client->send_buf, client->subscriptions[i].topic, 
                                    client->subscriptions[i].qos, client->packet_id++);
                net->send(client->socket, client->send_buf, len, 5000);
            }
            
            if (!client->recv_thread) {
                client->recv_thread = os->thread_create(mqtt_recv_thread, client, 2048, 5);
            }
        return -1;
    }
    
    uint32_t now = os->get_time_ms();
    uint32_t keepalive_ms = client->config.keepalive * 1000;
    
    if (now - client->last_ping_time >= keepalive_ms / 2) {
        os->mutex_lock(client->mutex, 0xFFFFFFFF);
        
        int len = pack_pingreq(client->send_buf);
        if (net->send(client->socket, client->send_buf, len, 1000) != len) {
            client->state = MQTT_STATE_DISCONNECTED;
            net->disconnect(client->socket);
            client->socket = NULL;
            os->mutex_unlock(client->mutex);
            return -1;
        }
        client->last_ping_time = now;
        
        os->mutex_unlock(client->mutex);
    }
    
    return 0;
}

int mqtt_client_is_connected(mqtt_client_t* client) {
    return client && client->state == MQTT_STATE_CONNECTED;
}

static void mqtt_recv_thread(void* arg) {
    mqtt_client_t* client = (mqtt_client_t*)arg;
    const mqtt_net_api_t* net = mqtt_net_get();
    const mqtt_os_api_t* os = mqtt_os_get();
    
    while (client->running) {
        int len = net->recv(client->socket, client->recv_buf, MQTT_RECV_BUF_SIZE, 1000);
        if (len < 0) {
            os->mutex_lock(client->mutex, 0xFFFFFFFF);
            if (client->state == MQTT_STATE_CONNECTED) {
                client->state = MQTT_STATE_DISCONNECTED;
                net->disconnect(client->socket);
                client->socket = NULL;
            }
            os->mutex_unlock(client->mutex);
            break;
        }
        if (len == 0) continue;
        
        uint8_t type = client->recv_buf[0] >> 4;
        
        if (type == MQTT_PUBLISH && client->config.msg_cb) {
            size_t remaining;
            int offset = 1 + decode_remaining_length(client->recv_buf + 1, &remaining);
            
            uint16_t topic_len = (client->recv_buf[offset] << 8) | client->recv_buf[offset + 1];
            offset += 2;
            
            char topic[128];
            if (topic_len < sizeof(topic)) {
                memcpy(topic, client->recv_buf + offset, topic_len);
                topic[topic_len] = '\0';
                offset += topic_len;
                
                uint8_t qos = (client->recv_buf[0] >> 1) & 0x03;
                if (qos > 0) offset += 2;
                
                size_t payload_len = len - offset;
                client->config.msg_cb(topic, client->recv_buf + offset, payload_len, client->config.user_data);
            }
        }
    }
    
    /* Call thread_exit if provided */
    if (os->thread_exit) {
        os->thread_exit();
    }
}
