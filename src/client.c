#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>

#define DHCP_SERVER_PORT 67 // DHCP server port
#define DHCP_CLIENT_PORT 68 // DHCP client port
#define MAXBUFLEN 512  // Maximum buffer size for receiving data

// DHCP packet structure (simplified)
struct dhcp_packet {
    uint8_t op;         // Message type
    uint8_t htype;      // Hardware address type
    uint8_t hlen;       // Hardware address length
    uint8_t hops;       // Hops
    uint32_t xid;       // Transaction ID
    uint16_t secs;      // Seconds elapsed
    uint16_t flags;     // Flags
    uint32_t ciaddr;    // Client IP address
    uint32_t yiaddr;    // Your IP address (server fills this)
    uint32_t siaddr;    // Next server IP address
    uint32_t giaddr;    // Relay agent IP address
    unsigned char chaddr[16]; // Client hardware address
    char sname[64];     // Server name
    char file[128];     // Boot filename
    unsigned char options[312]; // Optional parameters (DHCP options)
};

// Function to create and send DHCPDISCOVER
void send_dhcpdiscover(int sockfd, struct sockaddr_in *server_addr, uint32_t xid) {
    struct dhcp_packet discover_packet;
    memset(&discover_packet, 0, sizeof(discover_packet));

    discover_packet.op = 1;  // DHCPREQUEST
    discover_packet.htype = 1;  // Ethernet
    discover_packet.hlen = 6;   // MAC length
    discover_packet.xid = xid;  // Unique transaction ID
    discover_packet.flags = htons(0x8000); // Broadcast flag

    // DHCP options: magic cookie + message type (DHCPDISCOVER)
    discover_packet.options[0] = 0x63;
    discover_packet.options[1] = 0x82;
    discover_packet.options[2] = 0x53;
    discover_packet.options[3] = 0x63;
    discover_packet.options[4] = 53;  // DHCP Message Type option
    discover_packet.options[5] = 1;   // Length
    discover_packet.options[6] = 1;   // DHCPDISCOVER

    if (sendto(sockfd, &discover_packet, sizeof(discover_packet), 0,
               (struct sockaddr *)server_addr, sizeof(*server_addr)) == -1) {
        perror("sendto");
        exit(1);
    }

    printf("DHCP client: Sent DHCPDISCOVER\n");
}

// Function to receive and process DHCPOFFER
void receive_dhcpoffer(int sockfd) {
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    struct dhcp_packet offer_packet;
    int numbytes;

    if ((numbytes = recvfrom(sockfd, &offer_packet, sizeof(offer_packet), 0,
                             (struct sockaddr *)&server_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    printf("DHCP client: Received DHCPOFFER\n");

    // Print offered IP address and other relevant information
    struct in_addr yiaddr;
    yiaddr.s_addr = offer_packet.yiaddr;
    printf("Offered IP Address: %s\n", inet_ntoa(yiaddr));

    // Additional information like subnet mask, gateway, etc., can be extracted from options
}

// Thread function for handling the renewal of the IP address
void *renew_ip(void *arg) {
    int sockfd = *(int *)arg;
    struct sockaddr_in server_addr;
    uint32_t xid = rand(); // Generate a new transaction ID

    while (1) {
        sleep(60);  // Wait for a certain time (e.g., 60 seconds) before renewing IP

        // Send a DHCPREQUEST to renew the IP
        send_dhcpdiscover(sockfd, &server_addr, xid);
        receive_dhcpoffer(sockfd);
    }

    return NULL;
}

// Main function
int main(void) {
    int sockfd;
    struct sockaddr_in client_addr, server_addr;
    uint32_t xid = rand();  // Generate a random transaction ID

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // Setup the client address structure
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(DHCP_CLIENT_PORT);
    client_addr.sin_addr.s_addr = INADDR_ANY;
    memset(client_addr.sin_zero, '\0', sizeof client_addr.sin_zero);

    // Bind the socket to the client port
    if (bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) {
        perror("bind");
        close(sockfd);
        exit(1);
    }

    // Setup the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_BROADCAST;  // Broadcast address

    // Allow broadcasting
    int broadcast = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1) {
        perror("setsockopt");
        close(sockfd);
        exit(1);
    }

    // Send DHCPDISCOVER
    send_dhcpdiscover(sockfd, &server_addr, xid);

    // Receive DHCPOFFER
    receive_dhcpoffer(sockfd);

    // Create a thread to handle the IP renewal process
    pthread_t renew_thread;
    if (pthread_create(&renew_thread, NULL, renew_ip, &sockfd) != 0) {
        perror("pthread_create");
        close(sockfd);
        exit(1);
    }

    // Wait for the thread to finish (in this simple implementation, it will run indefinitely)
    pthread_join(renew_thread, NULL);

    // Close the socket and exit
    close(sockfd);

    return 0;
}