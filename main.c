#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bufsock.h"
#include "mqtt.h"

int main(int argc, char *argv[]) {
    //char *hostname = "messaging.quickstart.internetofthings.ibmcloud.com";
    char *hostname = "184.172.124.189";
    char *port = "1883";
    char *message_type = "random_data";
    char *type_id = "mqtt_test";
    char *device_name = "Some Laptop";
    char *device_id = NULL;
    char *mac_addr = NULL;

    char *err;
    char topic_name[100];
    char test_payload[4096];
    int device_id_size;
    int i;

    if(argc < 2) {
        fprintf(stderr, "Please provide a mac address without delimiters as the first argument");
        return EXIT_FAILURE;
    } else if(strlen(argv[1]) != 12) {
        fprintf(stderr, "First argument doesn't look like a mac addresss");
        return EXIT_FAILURE;
    }

    mac_addr = argv[1];
    device_id_size = 26 + strlen(type_id);
    device_id = malloc(sizeof(char)*device_id_size);
    snprintf(device_id, device_id_size, "d:quickstart:%s:%s", type_id, mac_addr);

    /* Topic name specific to quickstart */
    snprintf(topic_name, 100, "iot-2/evt/%s/fmt/json", message_type);

    printf("Connecting to %s:%s as %s\n", hostname, port, device_id);

    bufsock *sock = malloc(sizeof(bufsock));
    mqtt_message *message = malloc(sizeof(mqtt_message));

    bufsock_init(sock, hostname, port);

    mqtt_assemble_connect(message, CLEAN_SESSION, 10, device_id);

    if(!mqtt_send(sock, message)) {
        fprintf(stderr, "Error sending mqtt connect message\n");
        return EXIT_FAILURE;
    }

    if(!mqtt_recv(sock, message)) {
        fprintf(stderr, "Error receiving mqtt connack message\n");
        return EXIT_FAILURE;
    }

    if(message->type != CONNACK) {
        fprintf(stderr, "Didn't get a connack back from the server after connection\n");
        return EXIT_FAILURE;
    }

    if((err = mqtt_connack_strerr(message)) != NULL) {
        fprintf(stderr, "Connection was not accepted: %s\n", err);
        return EXIT_FAILURE;
    }

    printf("Attempting to publish to topic %s", topic_name);

    for(i = 0; i < 100; i++) {
        snprintf(test_payload, 4096, "{\"d\":{\"myName\":\"%s\",\"data\":%d}}", device_name, (i-50)*(i-50));
        printf("Publishing: %s\n", test_payload);

        mqtt_assemble_publish(message, topic_name, test_payload);
        mqtt_send(sock, message);
        sleep(1);
    }

    mqtt_assemble_disconnect(message);
    mqtt_send(sock, message);

    bufsock_close(sock);

    return 0;
}
