
#include "mqtt.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "bufsock.h"

#define BUF_SIZE 1024

static char *protocol = "MQTT";
static int protocol_size = 4;
static char version = 4;

/* We truncate num to 16 bits and return 2 chars */
static char *get16word(int num) {
    static char word[2];
    word[1] = num%128;
    word[0] = num/128;
    return (char*)word;
}

int mqtt_send(bufsock *sock, mqtt_message *message) {
    char *raw_message;
    unsigned char rem_len[5];
    unsigned len_digits = 0;
    unsigned total_message_len;
    unsigned i;
    char header;
    int bytes_sent;
    int len = message->payload_len;

    header = (message->type << 4) |
             (message->dup << 3) |
             (message->qos << 1) |
             (message->retain);

    for(i = 0; i < 4; i++) {
        unsigned char digit = len % 128;
        len = len / 128;
        if(len > 0) {
            digit = digit | 0x80;
        }
        if(i == 0 || digit > 0) {
            ++len_digits;
        }
        rem_len[i] = digit;
    }
    rem_len[4] = '\0';

    /* payload, plus remaining length digits, plus header */
    total_message_len = message->payload_len + len_digits + 1;
    raw_message = malloc(total_message_len);

    sprintf(raw_message, "%c%s",
            header, rem_len);
    memcpy(raw_message + 1 + len_digits, message->payload, message->payload_len);

    bytes_sent = buf_send(sock, raw_message, total_message_len, 0);

    printf("sent %d bytes: ", bytes_sent);
    for(i = 0; i < total_message_len; i++) {
        printf("%02X ", raw_message[i]);
    }
    printf("\n");

    free(raw_message);

    return bytes_sent;
}

int mqtt_recv(bufsock *sock, mqtt_message *message) {
    char header;
    char digit[4];
    int rcvd_bytes = 0;
    int multiplier = 1;
    int digits;
    int i;

    rcvd_bytes += buf_recv(sock, &header, 1, 0);

    message->type = (header >> 4);
    message->dup = (header & 0x8) >> 3;
    message->qos = (header & 0x6) >> 1;
    message->retain = header & 0x1;

    digits = 0;
    message->payload_len = 0;
    do {
        rcvd_bytes += buf_recv(sock, digit+digits, 1, 0);
        message->payload_len += (digit[digits] & 0x7F) * multiplier;
        multiplier *= 128;
        ++digits;
    } while ((digit[digits] & 0x80) != 0);

    rcvd_bytes += buf_recv(sock, message->payload, message->payload_len, 0);

    printf("rcvd %d bytes: ", rcvd_bytes);
    printf("%02X ", header);
    for(i = 0; i < digits; i++) {
        printf("%02X ", digit[i]);
    }
    for(i = 0; i < message->payload_len; i++) {
        printf("%02X ", message->payload[i]);
    }
    printf("\n");

    return 1;
}

void mqtt_assemble_connect(mqtt_message *message, char conn_flags, int keepalive_timer, char *device_id) {
    static int var_header_size = 10;

    int payload_size = strlen(device_id);
    char *var_header = malloc(var_header_size);
    char *payload = malloc(payload_size+2);

    memcpy(var_header, get16word(protocol_size), 2);
    memcpy(var_header+2, protocol, protocol_size);
    var_header[protocol_size+2] = version;
    var_header[protocol_size+3] = conn_flags;
    memcpy(var_header+protocol_size+4, get16word(keepalive_timer), 2);
    memcpy(payload, get16word(payload_size), 2);
    memcpy(payload+2, device_id, payload_size);

    message->type = CONNECT;
    message->dup = 0;
    message->qos = 0;
    message->retain = 0;
    message->payload_len = payload_size + var_header_size + 2;
    message->payload = malloc((message->payload_len) * sizeof(char));
    memcpy(message->payload, var_header, var_header_size);
    memcpy(message->payload + var_header_size, payload, payload_size + 2);

    free(var_header);
}

void mqtt_assemble_publish(mqtt_message *message, char *topic_name, char *payload) {
    int topic_len;
    int payload_len;
    int actual_payload_len;
    char *actual_payload;

    topic_len = strlen(topic_name);
    payload_len = strlen(payload);

    //len + 2-byte length identifier
    // apparently the payload has no length identifier? bizzare
    actual_payload_len = strlen(topic_name) + strlen(payload) + 2;
    actual_payload = malloc(actual_payload_len * sizeof(char));
    memcpy(actual_payload, get16word(topic_len), 2);
    memcpy(actual_payload + 2, topic_name, topic_len);
    memcpy(actual_payload + topic_len + 2, payload, payload_len);

    message->type = PUBLISH;
    message->dup = 0;
    message->qos = 0;
    message->retain = 0;
    message->payload = actual_payload;
    message->payload_len = actual_payload_len;
}

void mqtt_assemble_disconnect(mqtt_message *message) {
    message->type = DISCONNECT;
    message->dup = 0;
    message->qos = 0;
    message->retain = 0;
    message->payload = "";
    message->payload_len = 0;
}

char *mqtt_connack_strerr(mqtt_message *message) {
    if(message->type != CONNACK) {
        return NULL;
    } else if(message->payload_len < 2) {
        return "Malformed CONNACK message";
    } else if(message->payload[1] == REFUSED_PROTOCOL) {
        return "Connection was refused (unacceptable protocol version)";
    } else if(message->payload[1] == REFUSED_IDENTIFIER) {
        return "Connection was refused (identifier rejected)";
    } else if(message->payload[1] == REFUSED_UNAVAILABLE) {
        return "Connection was refused (server unavailable)";
    } else if(message->payload[1] == REFUSED_BAD_CREDENTIALS) {
        return "Connection was refused (bad username or password)";
    } else if(message->payload[1] == REFUSED_NOT_AUTH) {
        return "Connection was refused (not authorized)";
    } else {
        return NULL;
    }
}
