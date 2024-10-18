#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define DHCP_SERVER_IP "10.0.2.5"
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define BUFFER_SIZE 1024
#define IP_POOL_SIZE 10
#define LEASE_TIME 3600 // 1 hour

void request_renewal(int sock, struct sockaddr_in *server_addr) {
    // Prepare DHCPREQUEST message (simplified)
    char dhcp_request[BUFFER_SIZE];
    strcpy(dhcp_request, "DHCPREQUEST");
    
    // Send DHCPREQUEST message
    sendto(sock, dhcp_request, strlen(dhcp_request), 0, (struct sockaddr *)server_addr, sizeof(*server_addr));
    
    // Receive DHCPACK message
    char buffer[BUFFER_SIZE];
    recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
    printf("Received: %s\n", buffer);
}

void release_ip(int sock, struct sockaddr_in *server_addr) {
    // Prepare DHCPRELEASE message (simplified)
    char dhcp_release[BUFFER_SIZE];
    strcpy(dhcp_release, "DHCPRELEASE");
    
    // Send DHCPRELEASE message
    sendto(sock, dhcp_release, strlen(dhcp_release), 0, (struct sockaddr *)server_addr, sizeof(*server_addr));
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char dhcp_discover[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];

    // Create UDP socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);
    inet_pton(AF_INET, DHCP_SERVER_IP, &server_addr.sin_addr);

    // Prepare DHCPDISCOVER message (simplified)
    strcpy(dhcp_discover, "DHCPDISCOVER");

    // Send DHCPDISCOVER message
    ssize_t bytes_sent = sendto(sock, dhcp_discover, strlen(dhcp_discover), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (bytes_sent < 0) {
        perror("sendto failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Receive DHCPOFFER message
    ssize_t bytes_received = recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
    if (bytes_received < 0) {
        perror("recvfrom failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Print received DHCPOFFER
    printf("Received: %s\n", buffer);

    // Check for renewal condition
    while (1) {
        sleep(LEASE_TIME / 2); // Example: request renewal halfway through the lease time
        request_renewal(sock, &server_addr);
    }

    // Release IP before exit
    release_ip(sock, &server_addr);
    close(sock);
    return 0;
    return 0;
}
