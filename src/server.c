#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 67
#define MAXBUFLEN 512
#define MAX_CLIENTS 100
#define LEASE_TIME 3600  // Lease time in seconds

struct dhcp_packet {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    unsigned char chaddr[16];
    char sname[64];
    char file[128];
    unsigned char options[312];
};

// Structure to keep track of IP lease information
struct ip_lease {
    uint32_t ip;
    unsigned char mac[16];
    time_t lease_start;
};

// Array to store IP leases
struct ip_lease leases[MAX_CLIENTS];
int lease_count = 0;
uint32_t available_ips[MAX_CLIENTS];  // Store available IPs
int available_ip_count = 0;
pthread_mutex_t lease_mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialize available IP pool (hardcoded for simplicity)
void init_ip_pool() {
    available_ip_count = 5;
    available_ips[0] = inet_addr("192.168.1.100");
    available_ips[1] = inet_addr("192.168.1.101");
    available_ips[2] = inet_addr("192.168.1.102");
    available_ips[3] = inet_addr("192.168.1.103");
    available_ips[4] = inet_addr("192.168.1.104");
}

void print_mac_address(unsigned char *mac, size_t len) {
    printf("Client MAC Address: ");
    for (size_t i = 0; i < len; i++) {
        printf("%02x", mac[i]);
        if (i < len - 1) printf(":");
    }
    printf("\n");
}

// Check if a MAC address already has a lease
int find_lease_by_mac(unsigned char *mac) {
    for (int i = 0; i < lease_count; i++) {
        if (memcmp(mac, leases[i].mac, 16) == 0) {
            return i;
        }
    }
    return -1;
}

// Allocate an IP address from the available pool
uint32_t allocate_ip(unsigned char *mac) {
    pthread_mutex_lock(&lease_mutex);
    if (available_ip_count > 0) {
        int index = find_lease_by_mac(mac);
        if (index != -1) {
            // Client already has a lease
            pthread_mutex_unlock(&lease_mutex);
            return leases[index].ip;
        }

        uint32_t ip = available_ips[--available_ip_count];  // Allocate last available IP
        struct ip_lease lease;
        lease.ip = ip;
        memcpy(lease.mac, mac, 16);
        lease.lease_start = time(NULL);
        leases[lease_count++] = lease;

        pthread_mutex_unlock(&lease_mutex);
        return ip;
    }
    pthread_mutex_unlock(&lease_mutex);
    return 0;  // No available IPs
}

void send_dhcpoffer(int sockfd, struct dhcp_packet *discover_packet, struct sockaddr_in *client_addr) {
    struct dhcp_packet offer_packet;
    memset(&offer_packet, 0, sizeof(offer_packet));

    offer_packet.op = 2;
    offer_packet.htype = discover_packet->htype;
    offer_packet.hlen = discover_packet->hlen;
    offer_packet.xid = discover_packet->xid;
    offer_packet.flags = discover_packet->flags;
    offer_packet.yiaddr = allocate_ip(discover_packet->chaddr);
    if (offer_packet.yiaddr == 0) {
        printf("No available IP addresses to offer\n");
        return;
    }
    offer_packet.siaddr = inet_addr("192.168.1.1");
    memcpy(offer_packet.chaddr, discover_packet->chaddr, 16);

    // DHCP options (simplified)
    offer_packet.options[0] = 0x63;
    offer_packet.options[1] = 0x82;
    offer_packet.options[2] = 0x53;
    offer_packet.options[3] = 0x63;

    offer_packet.options[4] = 53;
    offer_packet.options[5] = 1;
    offer_packet.options[6] = 2;

    if (sendto(sockfd, &offer_packet, sizeof(offer_packet), 0, (struct sockaddr *)client_addr, sizeof(*client_addr)) == -1) {
        perror("sendto");
    }

    printf("Sent DHCPOFFER to client\n");
}

void *handle_dhcp_request(void *arg) {
    int sockfd = *(int *)arg;
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    char buf[MAXBUFLEN];
    int numbytes;
    struct dhcp_packet *dhcp;

    addr_len = sizeof client_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1, 0, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
        perror("recvfrom");
        pthread_exit(NULL);
    }

    buf[numbytes] = '\0';
    dhcp = (struct dhcp_packet *)buf;

    if (dhcp->op == 1) {
        printf("Received DHCPDISCOVER\n");
        print_mac_address(dhcp->chaddr, dhcp->hlen);
        send_dhcpoffer(sockfd, dhcp, &client_addr);
    } else {
        printf("Received non-DHCPDISCOVER message\n");
    }

    pthread_exit(NULL);
}

int main(void) {
    int sockfd;
    struct sockaddr_in server_addr;
    pthread_t threads[MAX_CLIENTS];
    int thread_count = 0;

    init_ip_pool();

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(sockfd);
        exit(1);
    }

    printf("DHCP server: waiting for requests...\n");

    while (1) {
        pthread_create(&threads[thread_count++], NULL, handle_dhcp_request, (void *)&sockfd);
        if (thread_count >= MAX_CLIENTS) thread_count = 0;
    }

    close(sockfd);
    return 0;
}
