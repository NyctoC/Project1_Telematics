#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

#define PORT 67  // Standard DHCP server port
#define BUFFER_SIZE 1024  // Size of buffer for receiving messages
#define MAX_CLIENTS 100  // Maximum number of clients

#define LOG_FILE_PATH "dhcp_log.txt"
FILE *log_file;

// Define a structure for IP lease information
struct ip_lease {
    char ip_address[16];  // IP address in string format
    time_t lease_start;
    time_t lease_duration;
    int in_use;  // 0 if not in use, 1 if in use
};

// Global IP pool (range of IPs to assign)
struct ip_lease ip_pool[MAX_CLIENTS];

// Function prototypes
void log_ip_assignment(const char *ip_address, const char *action);

// Initialize a pool of IP addresses to offer (for example purposes)
void init_ip_pool() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        snprintf(ip_pool[i].ip_address, sizeof(ip_pool[i].ip_address), "192.168.1.%d", i + 100);
        ip_pool[i].in_use = 0;  // Mark all IPs as available
    }
}

// Find an available IP address from the pool
struct ip_lease* get_available_ip() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!ip_pool[i].in_use) {
            return &ip_pool[i];
        }
    }
    return NULL;  // No available IPs
}

// Function to handle each client request in a new thread
void* handle_client(void* client_sock) {
    int sock = *(int*)client_sock;
    free(client_sock);  // Free the dynamically allocated socket reference
    char buffer[BUFFER_SIZE];
    
    // Receive the client's DHCP message
    ssize_t received_len = recv(sock, buffer, BUFFER_SIZE, 0);
    if (received_len < 0) {
        perror("Error receiving data");
        close(sock);
        return NULL;
    }

    // For demonstration, let's assume the message type is simply passed as a string (e.g., "DISCOVER")
    if (strncmp(buffer, "DISCOVER", 8) == 0) {
        printf("Received DHCPDISCOVER\n");

        // Find an available IP address to offer
        struct ip_lease* offered_ip = get_available_ip();
        if (offered_ip == NULL) {
            printf("No available IPs to offer\n");
            close(sock);
            return NULL;
        }

        // Mark the IP as in use and set lease start time
        offered_ip->in_use = 1;
        offered_ip->lease_start = time(NULL);
        offered_ip->lease_duration = 3600;  // 1 hour lease

        // Send a DHCPOFFER message to the client
        char offer_message[BUFFER_SIZE];
        snprintf(offer_message, sizeof(offer_message), "OFFER %s", offered_ip->ip_address);
        send(sock, offer_message, strlen(offer_message), 0);
        log_ip_assignment(offered_ip->ip_address, "OFFER");
        printf("Sent DHCPOFFER: %s\n", offered_ip->ip_address);
    } else if (strncmp(buffer, "REQUEST", 7) == 0) {
        printf("Received DHCPREQUEST\n");

        // For simplicity, assume client requests the same IP we offered
        char requested_ip[16];
        sscanf(buffer, "REQUEST %s", requested_ip);

        // Confirm if the requested IP is valid and already offered
        struct ip_lease* confirmed_ip = NULL;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (strcmp(ip_pool[i].ip_address, requested_ip) == 0 && ip_pool[i].in_use) {
                confirmed_ip = &ip_pool[i];
                break;
            }
        }

        if (confirmed_ip) {
            // Send DHCPACK to confirm the assignment
            char ack_message[BUFFER_SIZE];
            snprintf(ack_message, sizeof(ack_message), "ACK %s", confirmed_ip->ip_address);
            send(sock, ack_message, strlen(ack_message), 0);
            log_ip_assignment(confirmed_ip->ip_address, "ACK");
            printf("Sent DHCPACK: %s\n", confirmed_ip->ip_address);
        } else {
            printf("Requested IP is not available\n");
        }
    }

    close(sock);  // Close the socket for this client
    return NULL;
}

// Function to periodically check for expired IP leases
void* lease_expiration_check(void* arg) {
    while (1) {
        time_t current_time = time(NULL);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (ip_pool[i].in_use && current_time > ip_pool[i].lease_start + ip_pool[i].lease_duration) {
                // Lease expired, release the IP
                printf("Lease expired for IP: %s\n", ip_pool[i].ip_address);
                log_ip_assignment(ip_pool[i].ip_address, "RELEASE");
                ip_pool[i].in_use = 0;
            }
        }

        sleep(60);  // Check every 60 seconds
    }
    return NULL;
}

// Function to log IP assignments, renewals, and releases
void log_ip_assignment(const char *ip_address, const char *action) {
    time_t current_time = time(NULL);
    fprintf(log_file, "%s: %s - %s\n", ctime(&current_time), action, ip_address);
    fflush(log_file);  // Ensure the log is written to file immediately
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Open the log file for appending
    log_file = fopen(LOG_FILE_PATH, "a");
    if (!log_file) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }

    // Initialize the IP pool
    init_ip_pool();

    // Create UDP socket
    if ((server_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address parameters
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on any network interface
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the server address
    if (bind(server_sock, (const struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    pthread_t lease_check_thread;
    if (pthread_create(&lease_check_thread, NULL, lease_expiration_check, NULL) != 0) {
        perror("Failed to create lease check thread");
        exit(EXIT_FAILURE);
    }

    printf("DHCP server listening on port %d\n", PORT);

    // Main loop to handle incoming requests
    while (1) {
        // Accept a new client request (blocking call)
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;  // Continue to next iteration if accept fails
        }

        // Create a new thread to handle the client
        pthread_t thread_id;
        int* new_sock = malloc(sizeof(int));  // Allocate memory for socket
        *new_sock = client_sock;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)new_sock) != 0) {
            perror("Thread creation failed");
            close(client_sock);
            free(new_sock);  // Free memory in case of failure
        }

        pthread_detach(thread_id);  // Detach thread to prevent memory leaks
    }

    close(server_sock);  // Close the server socket when done
    fclose(log_file);  // Close the log file
    return 0;
}
