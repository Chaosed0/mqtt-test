
all:
	gcc -g -O0 main.c mqtt.c bufsock.c -o iot -Wall -Wextra
