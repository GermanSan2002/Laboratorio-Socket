#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_PORT 1022
#define SERVER_IP "2001:db8::1" // Reemplaza con la dirección IPv6 del servidor
#define BUFFER_SIZE 200

void *receive_messages(void *arg);

int main() {
    int sock;
    struct sockaddr_in6 server_addr;
    char buffer[BUFFER_SIZE];

    sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(SERVER_PORT);
    inet_pton(AF_INET6, SERVER_IP, &server_addr.sin6_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al conectar al servidor");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Conectado al servidor.\n");

    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_messages, &sock);

    while (1) {
        printf("Ingrese un mensaje: ");
        fgets(buffer, BUFFER_SIZE, stdin);

        if (strcmp(buffer, "DESCONECTAR\n") == 0) {
            send(sock, buffer, strlen(buffer), 0);
            break;
        }

        send(sock, buffer, strlen(buffer), 0);
    }

    close(sock);
    return 0;
}

void *receive_messages(void *arg) {
    int sock = *(int *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t n;

    while ((n = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[n] = '\0';
        printf("Mensaje del servidor: %s\n", buffer);
    }

    printf("Conexión cerrada por el servidor.\n");
    pthread_exit(NULL);
}
