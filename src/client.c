#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define DHCP_SERVER_IP "18.224.136.204"
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define BUFFER_SIZE 1024
#define LEASE_TIME 3600 // 1 hour
#define MAX_RETRIES 5
#define RETRY_INTERVAL 2 

void request_renewal(int sock, struct sockaddr_in *server_addr) {
    char dhcp_request[BUFFER_SIZE] = "DHCPREQUEST";

    // Send DHCPREQUEST message
    sendto(sock, dhcp_request, strlen(dhcp_request), 0, (struct sockaddr *)server_addr, sizeof(*server_addr));

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0'; // Null-terminate the message
        printf("Received: %s\n", buffer); // Display the DHCPACK with IP config
    } else {
        printf("No response received for DHCPREQUEST.\n");
    }
}

void release_ip(int sock, struct sockaddr_in *server_addr) {
    char dhcp_release[BUFFER_SIZE] = "DHCPRELEASE";

    // Send DHCPRELEASE message
    sendto(sock, dhcp_release, strlen(dhcp_release), 0, (struct sockaddr *)server_addr, sizeof(*server_addr));
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char dhcp_discover[BUFFER_SIZE] = "DHCPDISCOVER";
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

    int retries = 0;
    while (retries < MAX_RETRIES) {
        // Indicate that the client is attempting to find the server
        printf("Sending DHCPDISCOVER to %s (Attempt %d of %d)...\n", DHCP_SERVER_IP, retries + 1, MAX_RETRIES);
        
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
            printf("No response received. Retrying in %d seconds...\n", RETRY_INTERVAL);
            sleep(RETRY_INTERVAL); // Wait before retrying
            retries++;
            continue; // Retry sending DHCPDISCOVER
        }

        buffer[bytes_received] = '\0'; // Null-terminate the message
        printf("Received: %s\n", buffer); // Print the DHCPOFFER
        break; // Break the loop on successful reception
    }

    if (retries == MAX_RETRIES) {
        printf("Failed to receive DHCPOFFER after %d attempts. Exiting...\n", MAX_RETRIES);
        close(sock);
        return 1; // Exit with an error code
    }

    // Send DHCPREQUEST to request the offered IP
    request_renewal(sock, &server_addr);

    // renewal loop
    while (1) {
        sleep(LEASE_TIME / 2);
        request_renewal(sock, &server_addr);
    }

    // Release IP before exit
    release_ip(sock, &server_addr);
    close(sock);
    return 0;
}
