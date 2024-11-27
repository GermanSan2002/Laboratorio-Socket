#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 1022
#define MULTICAST_GROUP "ff02::1" // Grupo multicast para IPv6
#define BUFFER_SIZE 200

int main() {
    int sockfd;
    struct sockaddr_in6 multicast_addr;
    char message[BUFFER_SIZE];

    printf("Servidor iniciado.\n");

    // Crear el socket
    sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección multicast
    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin6_family = AF_INET6;
    multicast_addr.sin6_port = htons(PORT);
    if (inet_pton(AF_INET6, MULTICAST_GROUP, &multicast_addr.sin6_addr) <= 0) {
        perror("Error en inet_pton");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor listo para enviar mensajes al grupo multicast %s en el puerto %d.\n", MULTICAST_GROUP, PORT);

    while (1) {
        printf("Ingrese un mensaje (máx. %d caracteres): ", BUFFER_SIZE);
        fgets(message, BUFFER_SIZE, stdin);

        if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0) {
            perror("Error al enviar el mensaje");
        } else {
            printf("Mensaje enviado: %s\n", message);
        }
    }

    close(sockfd);
    return 0;
}
