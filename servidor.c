#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h> // Para if_nametoindex()

#define SERVER_PORT 1022    // Puerto en el que el servidor escucha
#define CLIENT_PORT 1023    // Puerto al que se enviarán los mensajes
#define MULTICAST_GROUP "ff02::1" // Grupo multicast para IPv6
#define BUFFER_SIZE 200

int main() {
    int sockfd_recv, sockfd_send;
    struct sockaddr_in6 server_addr, multicast_addr, client_addr;
    char message[BUFFER_SIZE];
    socklen_t client_len = sizeof(client_addr);

    printf("Servidor iniciado.\n");

    // *** Configuración del socket para escuchar en el puerto 1022 ***
    sockfd_recv = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockfd_recv < 0) {
        perror("Error al crear el socket de recepción");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(SERVER_PORT);
    server_addr.sin6_addr = in6addr_any;

    if (bind(sockfd_recv, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al enlazar el socket de recepción");
        close(sockfd_recv);
        exit(EXIT_FAILURE);
    }

    printf("Servidor listo para recibir mensajes en el puerto %d.\n", SERVER_PORT);

    // *** Configuración del socket para enviar mensajes al grupo multicast ***
    sockfd_send = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockfd_send < 0) {
        perror("Error al crear el socket de envío");
        exit(EXIT_FAILURE);
    }

    unsigned int ifindex = if_nametoindex("enp0s3"); // Cambiar "enp0s3" por el nombre de la interfaz
    if (ifindex == 0) {
        perror("Error al obtener el índice de la interfaz");
        close(sockfd_send);
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sockfd_send, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0) {
        perror("Error al configurar la interfaz multicast");
        close(sockfd_send);
        exit(EXIT_FAILURE);
    }

    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin6_family = AF_INET6;
    multicast_addr.sin6_port = htons(CLIENT_PORT);
    if (inet_pton(AF_INET6, MULTICAST_GROUP, &multicast_addr.sin6_addr) <= 0) {
        perror("Error en inet_pton");
        close(sockfd_send);
        exit(EXIT_FAILURE);
    }

    printf("Servidor listo para enviar mensajes al grupo multicast %s en el puerto %d.\n", MULTICAST_GROUP, CLIENT_PORT);

    // *** Bucle principal para enviar y recibir mensajes ***
    while (1) {
        // Comprobar si hay mensajes recibidos en el puerto 1022
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd_recv, &readfds);

        struct timeval timeout = {0, 0}; // Sin espera
        int activity = select(sockfd_recv + 1, &readfds, NULL, NULL, &timeout);

        if (activity > 0 && FD_ISSET(sockfd_recv, &readfds)) {
            // Recibir mensaje del cliente
            char buffer[BUFFER_SIZE];
            int n = recvfrom(sockfd_recv, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
            if (n < 0) {
                perror("Error al recibir mensaje");
            } else {
                buffer[n] = '\0';
                printf("Mensaje recibido del cliente: %s\n", buffer);
            }
        }

        // Enviar mensaje al grupo multicast
        printf("Ingrese un mensaje para enviar al grupo multicast (máx. %d caracteres): ", BUFFER_SIZE);
        fgets(message, BUFFER_SIZE, stdin);

        if (sendto(sockfd_send, message, strlen(message), 0, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0) {
            perror("Error al enviar el mensaje");
        } else {
            printf("Mensaje enviado: %s\n", message);
        }
    }

    close(sockfd_recv);
    close(sockfd_send);
    return 0;
}
