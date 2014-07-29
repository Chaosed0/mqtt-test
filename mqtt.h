
#ifndef MQTT_H
#define MQTT_H

typedef struct bufsock bufsock;

typedef struct mqtt_message {
    char type;
    char dup;
    char qos;
    char retain;
    char *payload;
    int payload_len;
} mqtt_message;

typedef enum message_type {
    RESERVED_1 = 0,
    CONNECT,
    CONNACK,
    PUBLISH,
    PUBACK,
    PUBREC,
    PUBREL,
    PUBCOMP,
    SUBSCRIBE,
    SUBACK,
    UNSUBSCRIBE,
    UNSUBACK,
    PINGREQ,
    PINGRESP,
    DISCONNECT,
    RESERVED_2
} message_type;

typedef enum conn_flags {
    USER_NAME = 1 << 7,
    PASSWORD = 1 << 6,
    QOS_L1 = 1 << 4,
    QOS_L2 = 1 << 5,
    QOS_L3 = 3 << 4,
    WILL_RETAIN = 1 << 5,
    WILL_FLAG = 1 << 2,
    CLEAN_SESSION = 1 << 1
} conn_flags;

typedef enum connack_return {
    ACCEPTED = 0,
    REFUSED_PROTOCOL = 0x1,
    REFUSED_IDENTIFIER = 0x2,
    REFUSED_UNAVAILABLE = 0x3,
    REFUSED_BAD_CREDENTIALS = 0x4,
    REFUSED_NOT_AUTH = 0x5
} connack_return;

char *mqtt_connack_strerr(mqtt_message *message);
int mqtt_send(bufsock *sock, mqtt_message *message);
int mqtt_recv(bufsock *sock, mqtt_message *message);
void mqtt_assemble_connect(mqtt_message *message, char conn_flags, int keepalive_timer, char *mac_addr);
void mqtt_assemble_publish(mqtt_message *message, char *topic_name, char *payload);
void mqtt_assemble_disconnect(mqtt_message *message);

#endif
