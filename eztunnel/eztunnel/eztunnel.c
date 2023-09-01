#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 12345
#define BUF_SIZE 1500
#define LOG_FILE "/var/log/daemon_tun.log"

// Logging function
void log_message(const char *message) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file) {
        fprintf(log_file, "%s\n", message);
        fclose(log_file);
    }
}

// Error handling and logging
void handle_error(const char *message) {
    log_message(message);
    perror(message);
    exit(EXIT_FAILURE);
}

int tun_alloc(char *dev) {
    struct ifreq ifr;
    int fd;
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) handle_error("Opening /dev/net/tun");
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN;
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
        close(fd);
        handle_error("Setting TUN interface");
    }
    return fd;
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) handle_error("Fork failed");
    if (pid > 0) exit(EXIT_SUCCESS);
    umask(0);
    if (setsid() < 0) handle_error("Setsid failed");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: eztunnel <LocalBindAddress> <IP1> <IP2> ...\n");
        exit(EXIT_FAILURE);
    }

    daemonize();

    char buffer[BUF_SIZE];
    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0) handle_error("UDP socket creation failed");
    int tun_fds[argc - 2];

    for (int i = 2; i < argc; i++) {
        char tun_name[10];
        sprintf(tun_name, "tun%d", i - 2);
        tun_fds[i - 2] = tun_alloc(tun_name);
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    if (inet_pton(AF_INET, argv[1], &(server.sin_addr)) != 1) {
        handle_error("Invalid local bind address");
    }
    if (bind(udp_fd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        handle_error("Bind failed");
    }

    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(udp_fd, &read_fds);
        for (int i = 0; i < argc - 2; i++)
            FD_SET(tun_fds[i], &read_fds);

        if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) < 0) {
            handle_error("Select failed");
        }

        for (int i = 0; i < argc - 2; i++) {
            if (FD_ISSET(tun_fds[i], &read_fds)) {
                int len = read(tun_fds[i], buffer, sizeof(buffer));
                if (len < 0) {
                    log_message("Failed reading from TUN interface");
                    continue;
                }
                struct sockaddr_in dest;
                dest.sin_family = AF_INET;
                dest.sin_port = htons(PORT);
                inet_pton(AF_INET, argv[i + 2], &(dest.sin_addr));
                if (sendto(udp_fd, buffer, len, 0, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
                    log_message("Failed sending UDP packet");
                }
            }
        }

        if (FD_ISSET(udp_fd, &read_fds)) {
            struct sockaddr_in client;
            socklen_t slen = sizeof(client);
            int len = recvfrom(udp_fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&client, &slen);
            if (len < 0) {
                log_message("Failed receiving UDP packet");
                continue;
            }
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client.sin_addr), client_ip, INET_ADDRSTRLEN);
            for (int i = 2; i < argc; i++) {
                if (strcmp(client_ip, argv[i]) == 0) {
                    if (write(tun_fds[i - 2], buffer, len) < 0) {
                        log_message("Failed writing to TUN interface");
                    }
                }
            }
        }
    }

    for (int i = 0; i < argc - 2; i++)
        close(tun_fds[i]);
    close(udp_fd);

    return 0;
}

