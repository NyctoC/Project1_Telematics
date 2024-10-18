#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define BUFFER_SIZE 1024

#define IP_POOL_SIZE 10
#define LEASE_TIME 3600 // 1 hour

typedef struct {
    char ip[16]; // IP address in string format
    time_t lease_time; // Timestamp of when the lease was granted
    int is_allocated; // 0 if free, 1 if allocated
} IpLease;

IpLease ip_pool[IP_POOL_SIZE]; // Array to store IP leases

void init_ip_pool() {
    for (int i = 0; i < IP_POOL_SIZE; i++) {
        sprintf(ip_pool[i].ip, "192.168.1.%d", i + 10); // Adjusted subnet
        ip_pool[i].is_allocated = 0;
    }
}

int allocate_ip(char* assigned_ip) {
    for (int i = 0; i < IP_POOL_SIZE; i++) {
        if (ip_pool[i].is_allocated == 0) {
            ip_pool[i].is_allocated = 1;
            ip_pool[i].lease_time = time(NULL); // Assign current time
            strcpy(assigned_ip, ip_pool[i].ip);
            return 0; // Success
        }
    }
    return -1; // No IP available
}

void release_ip(const char* ip) {
    for (int i = 0; i < IP_POOL_SIZE; i++) {
        if (strcmp(ip_pool[i].ip, ip) == 0) {
            ip_pool[i].is_allocated = 0; // Free the IP
            printf("Released IP: %s\n", ip);
            break;
        }
    }
}

void *handle_client(void *arg) {
    int sock = *(int *)arg;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    char offered_ip[16] = "";

    while (1) {
        // Receive DHCP message
        ssize_t bytes_received = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
        if (bytes_received < 0) {
            perror("recvfrom failed");
            continue;
        }

        buffer[bytes_received] = '\0'; // Null-terminate the message

        if (strcmp(buffer, "DHCPDISCOVER") == 0) {
            printf("Received DHCPDISCOVER from %s\n", inet_ntoa(client_addr.sin_addr));

            // Try to allocate an IP address
            if (allocate_ip(offered_ip) == 0) {
                // Prepare DHCPOFFER message
                char dhcp_offer[BUFFER_SIZE];
                snprintf(dhcp_offer, sizeof(dhcp_offer), "DHCPOFFER IP:%s Subnet:255.255.255.0 Gateway:192.168.1.1 DNS:8.8.8.8", offered_ip);

                // Send DHCPOFFER message
                ssize_t bytes_sent = sendto(sock, dhcp_offer, strlen(dhcp_offer), 0, (struct sockaddr *)&client_addr, client_len);
                if (bytes_sent < 0) {
                    perror("sendto failed");
                } else {
                    printf("Sent DHCPOFFER to %s with IP %s\n", inet_ntoa(client_addr.sin_addr), offered_ip);
                }
            } else {
                printf("No IP available for %s\n", inet_ntoa(client_addr.sin_addr));
            }

        } else if (strcmp(buffer, "DHCPREQUEST") == 0) {
            printf("Received DHCPREQUEST from %s\n", inet_ntoa(client_addr.sin_addr));

            // Prepare DHCPACK message
            char dhcp_ack[BUFFER_SIZE];
            snprintf(dhcp_ack, sizeof(dhcp_ack), "DHCPACK IP:%s Subnet:255.255.255.0 Gateway:192.168.1.1 DNS:8.8.8.8 Lease:%d", offered_ip, LEASE_TIME);

            // Send DHCPACK message
            ssize_t bytes_sent = sendto(sock, dhcp_ack, strlen(dhcp_ack), 0, (struct sockaddr *)&client_addr, client_len);
            if (bytes_sent < 0) {
                perror("sendto failed");
            } else {
                printf("Sent DHCPACK to %s with IP %s\n", inet_ntoa(client_addr.sin_addr), offered_ip);
            }

        } else if (strcmp(buffer, "DHCPRELEASE") == 0) {
            printf("Received DHCPRELEASE from %s\n", inet_ntoa(client_addr.sin_addr));

            // Release the IP address
            release_ip(offered_ip);
        }
    }

    close(sock);
    return NULL;
}

int main() {
    init_ip_pool();

    int sock;
    struct sockaddr_in server_addr;

    // Create UDP socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the DHCP server port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("DHCP server listening on port %d...\n", DHCP_SERVER_PORT);

    // Create a thread to handle clients
    pthread_t tid;
    pthread_create(&tid, NULL, handle_client, &sock);
    pthread_detach(tid);

    // Keep the main thread running
    while (1) {
        sleep(1);
    }

    return 0;
}
