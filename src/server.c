#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 67  // DHCP server port
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

void print_mac_address(unsigned char *mac, size_t len) {
    printf("Client MAC Address: ");
    for (size_t i = 0; i < len; i++) {
        printf("%02x", mac[i]);
        if (i < len - 1) printf(":");
    }
    printf("\n");
}

int main(void)
{
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;
    char buf[MAXBUFLEN];
    int numbytes;
    struct dhcp_packet *dhcp;

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // Setup the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);

    // Bind the socket to the specified port
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(sockfd);
        exit(1);
    }

    printf("DHCP server: waiting for requests on port %d...\n", PORT);

    while (1) {
        addr_len = sizeof client_addr;
        // Receive a DHCP request from a client
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1, 0,
                (struct sockaddr *)&client_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        buf[numbytes] = '\0';
        dhcp = (struct dhcp_packet *)buf;  // Cast the received data to DHCP packet

        // Check if it's a DHCPDISCOVER message (op = 1 for request)
        if (dhcp->op == 1) {
            printf("DHCP server: received DHCPDISCOVER message\n");
            print_mac_address(dhcp->chaddr, dhcp->hlen);
        } else {
            printf("DHCP server: received non-DHCPDISCOVER message\n");
        }
    }

    close(sockfd);
    return 0;
}
