#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>

#define DHCP_SERVER_IP "10.0.2.5"
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define BUFFER_SIZE 1024
#define LEASE_TIME 3600 // 1 hour

int sock;
struct sockaddr_in server_addr;

// Function to parse server response and display IP details
void parse_and_display_info(char *buffer) {
    char ip[16], subnet_mask[16], gateway[16], dns[16];
    // Assuming the server sends a CSV-like response with IP, subnet, gateway, DNS
    sscanf(buffer, "%s %s %s %s", ip, subnet_mask, gateway, dns);
    
    printf("Received DHCP Offer:\n");
    printf("IP Address: %s\n", ip);
    printf("Subnet Mask: %s\n", subnet_mask);
    printf("Default Gateway: %s\n", gateway);
    printf("DNS Server: %s\n", dns);
}

// Function to send a DHCPREQUEST for IP renewal
void request_renewal() {
    // Prepare DHCPREQUEST message
    char dhcp_request[BUFFER_SIZE];
    strcpy(dhcp_request, "DHCPREQUEST");
    
    // Send DHCPREQUEST message
    sendto(sock, dhcp_request, strlen(dhcp_request), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    
    // Receive DHCPACK message
    char buffer[BUFFER_SIZE];
    recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
    printf("Received on renewal: %s\n", buffer);
    parse_and_display_info(buffer); // Display the updated IP details
}

// Function to send a DHCPRELEASE message before exiting
void release_ip() {
    // Prepare DHCPRELEASE message
    char dhcp_release[BUFFER_SIZE];
    strcpy(dhcp_release, "DHCPRELEASE");
    
    // Send DHCPRELEASE message
    sendto(sock, dhcp_release, strlen(dhcp_release), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("IP released\n");
    close(sock);
    exit(0);
}

int main() {
    struct sockaddr_in client_addr;
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

    // Bind the socket to DHCP client port
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(DHCP_CLIENT_PORT);

    if (bind(sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Prepare DHCPDISCOVER message
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

    // Print received IP and other details
    parse_and_display_info(buffer);

    // Handle renewal when needed
    while (1) {
        sleep(LEASE_TIME / 2); // Request renewal halfway through lease time
        request_renewal();
    }

    // Register signal handler to release IP on exit
    signal(SIGINT, release_ip);

    return 0;
}
