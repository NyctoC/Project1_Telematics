#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>

#define SERVER_PORT 67  // Standard DHCP server port
#define BUFFER_SIZE 1024

char assigned_ip[16];  // Store the assigned IP address for release

// Function to send DHCPDISCOVER message
void send_dhcp_discover(int sock, struct sockaddr_in* server_addr) {
    char message[] = "DISCOVER";
    ssize_t sent_len = sendto(sock, message, strlen(message), 0, 
                              (struct sockaddr*)server_addr, sizeof(*server_addr));
    if (sent_len < 0) {
        perror("Failed to send DHCPDISCOVER");
        exit(EXIT_FAILURE);
    }
    printf("Sent DHCPDISCOVER to server\n");
}

// Function to receive DHCPOFFER and display full configuration
void receive_dhcp_offer(int sock) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);

    ssize_t received_len = recvfrom(sock, buffer, BUFFER_SIZE, 0, 
                                    (struct sockaddr*)&server_addr, &server_addr_len);
    if (received_len < 0) {
        perror("Failed to receive DHCPOFFER");
        exit(EXIT_FAILURE);
    }

    buffer[received_len] = '\0';  // Null-terminate the received message
    printf("Received DHCPOFFER: %s\n", buffer);

    // Parse and display the assigned IP address, subnet mask, gateway, and DNS
    char subnet_mask[16], gateway[16], dns_server[16];
    sscanf(buffer, "OFFER %15s %15s %15s %15s", assigned_ip, subnet_mask, gateway, dns_server);

    // Display the configuration received from the server
    printf("Assigned IP address: %s\n", assigned_ip);
    printf("Subnet Mask: %s\n", subnet_mask);
    printf("Gateway: %s\n", gateway);
    printf("DNS Server: %s\n", dns_server);
}

// Function to send DHCPREQUEST for renewal
void send_dhcp_request(int sock, struct sockaddr_in* server_addr) {
    char request_message[BUFFER_SIZE];
    snprintf(request_message, sizeof(request_message), "REQUEST %s", assigned_ip);
    ssize_t sent_len = sendto(sock, request_message, strlen(request_message), 0, 
                              (struct sockaddr*)server_addr, sizeof(*server_addr));
    if (sent_len < 0) {
        perror("Failed to send DHCPREQUEST");
        exit(EXIT_FAILURE);
    }
    printf("Sent DHCPREQUEST for IP: %s\n", assigned_ip);
}

// Function to release the IP address
void release_ip(int sock, struct sockaddr_in* server_addr) {
    char release_message[BUFFER_SIZE];
    snprintf(release_message, sizeof(release_message), "RELEASE %s", assigned_ip);
    ssize_t sent_len = sendto(sock, release_message, strlen(release_message), 0, 
                              (struct sockaddr*)server_addr, sizeof(*server_addr));
    if (sent_len < 0) {
        perror("Failed to send DHCPRELEASE");
        exit(EXIT_FAILURE);
    }
    printf("Sent DHCPRELEASE for IP: %s\n", assigned_ip);
}

// Signal handler for cleanup on exit
void handle_exit(int sig) {
    printf("Exiting client...\n");
    // Cleanup code if necessary
    exit(0);
}

int main() {
    int sock;
    struct sockaddr_in server_addr;

    // Set up signal handling for clean exit
    signal(SIGINT, handle_exit);

    // Create UDP socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address (broadcast IP and DHCP server port)
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("255.255.255.255");  // Broadcast address

    // Enable broadcast option for the socket
    int broadcast_enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("Failed to enable broadcast option");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Step 1: Send DHCPDISCOVER
    send_dhcp_discover(sock, &server_addr);

    // Step 2: Receive DHCPOFFER and display full configuration
    receive_dhcp_offer(sock);

    // Step 3: Request IP renewal (could be triggered by user input or after some time)
    send_dhcp_request(sock, &server_addr);

    // Step 4: Release the IP before exiting
    release_ip(sock, &server_addr);

    // Close the socket after processing
    close(sock);
    return 0;
}
