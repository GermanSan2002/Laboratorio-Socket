#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_PORT 1022
#define BUFFER_SIZE 200
#define MAX_CLIENTS 100

typedef struct {
    int socket;
    struct sockaddr_in6 address;
} Client;

Client clients[MAX_CLIENTS];
int num_clients = 0;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg);
void broadcast_message(const char *message, int sender_sock);

int main() {
    int server_sock, client_sock;
    struct sockaddr_in6 server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(SERVER_PORT);
    server_addr.sin6_addr = in6addr_any;

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al enlazar el socket");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("Error al escuchar conexiones");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en el puerto %d...\n", SERVER_PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Error al aceptar conexión");
            continue;
        }

        printf("Nuevo cliente conectado.\n");

        pthread_t client_thread;
        int *pclient_sock = malloc(sizeof(int));
        *pclient_sock = client_sock;
        pthread_create(&client_thread, NULL, handle_client, pclient_sock);
        pthread_detach(client_thread); // Permitir que el hilo se limpie automáticamente
    }

    close(server_sock);
    return 0;
}

void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    ssize_t n;

    // Registrar cliente
    pthread_mutex_lock(&client_mutex);
    if (num_clients < MAX_CLIENTS) {
        clients[num_clients].socket = client_sock;
        num_clients++;
    } else {
        printf("Número máximo de clientes alcanzado. Conexión rechazada.\n");
        close(client_sock);
        pthread_mutex_unlock(&client_mutex);
        return NULL;
    }
    pthread_mutex_unlock(&client_mutex);

    while ((n = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[n] = '\0';
        printf("Mensaje recibido de cliente: %s\n", buffer);

        // Si el cliente envía "DESCONECTAR", eliminarlo de la lista
        if (strcmp(buffer, "DESCONECTAR\n") == 0) {
            pthread_mutex_lock(&client_mutex);
            for (int i = 0; i < num_clients; i++) {
                if (clients[i].socket == client_sock) {
                    clients[i] = clients[num_clients - 1]; // Reemplazar con el último cliente
                    num_clients--;
                    break;
                }
            }
            pthread_mutex_unlock(&client_mutex);
            printf("Cliente desconectado.\n");
            break;
        }

        // Difundir el mensaje a todos los clientes
        broadcast_message(buffer, client_sock);
    }

    close(client_sock);
    return NULL;
}

void broadcast_message(const char *message, int sender_sock) {
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].socket != sender_sock) { // No enviar al remitente
            if (send(clients[i].socket, message, strlen(message), 0) < 0) {
                perror("Error al enviar mensaje");
            }
        }
    }
    pthread_mutex_unlock(&client_mutex);
}
