#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 1023
#define MULTICAST_GROUP "ff02::1" // Grupo multicast para IPv6
#define BUFFER_SIZE 200

int main() {
    int sockfd;
    struct sockaddr_in6 local_addr, sender_addr;
    struct ipv6_mreq group;
    char buffer[BUFFER_SIZE];
    socklen_t sender_len = sizeof(sender_addr);

    printf("Cliente iniciado.\n");

    // Crear el socket
    sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección local
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin6_family = AF_INET6;
    local_addr.sin6_port = htons(PORT);
    local_addr.sin6_addr = in6addr_any;

    if (bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("Error al enlazar el socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Unirse al grupo multicast
    if (inet_pton(AF_INET6, MULTICAST_GROUP, &group.ipv6mr_multiaddr) <= 0) {
        perror("Error en inet_pton");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    group.ipv6mr_interface = 0; // Interface predeterminada

    if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &group, sizeof(group)) < 0) {
        perror("Error al unirse al grupo multicast");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Cliente listo para recibir mensajes en el grupo multicast %s en el puerto %d.\n", MULTICAST_GROUP, PORT);

    while (1) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&sender_addr, &sender_len);
        if (n < 0) {
            perror("Error al recibir mensaje");
            continue;
        }
        buffer[n] = '\0';

        // Obtener la fecha y hora actual
        time_t now = time(NULL);
        char *time_str = ctime(&now);
        time_str[strlen(time_str) - 1] = '\0'; // Eliminar salto de línea

        char sender_ip[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &sender_addr.sin6_addr, sender_ip, sizeof(sender_ip));

        printf("\nMensaje recibido:\n");
        printf("- Fecha y hora: %s\n", time_str);
        printf("- IP origen: %s\n", sender_ip);
        printf("- Puerto origen: %d\n", ntohs(sender_addr.sin6_port));
        printf("- Contenido: %s\n", buffer);
    }

    close(sockfd);
    return 0;
}
