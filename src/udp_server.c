#include "udp_server.h"

#define BUF_SIZE 512
#define UDP_READ_TIMEOUT_MS 500

static int sockfd = -1;
static struct pollfd poller;
static struct sockaddr_in client_addr;
static socklen_t client_len = sizeof(client_addr);

static RDSModulator* mod = NULL;  // Store modulator pointer globally or pass it somehow

int open_udp_server(int port, RDSModulator* rds_mod) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) return -1;

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        sockfd = -1;
        return -1;
    }

    poller.fd = sockfd;
    poller.events = POLLIN;

    mod = rds_mod;  // Save mod pointer

    return 0;
}

void poll_udp_server() {
    static char buf[BUF_SIZE];
    static char cmd_buf[BUF_SIZE];
    static char cmd_output[BUF_SIZE];  // Buffer to collect output from process_ascii_cmd
    ssize_t bytes_read;

    if (poll(&poller, 1, UDP_READ_TIMEOUT_MS) <= 0) return;
    if (!(poller.revents & POLLIN)) return;

    memset(buf, 0, BUF_SIZE);
    client_len = sizeof(client_addr);
	bytes_read = recvfrom(sockfd, buf, BUF_SIZE - 1, 0,
                      (struct sockaddr *)&client_addr, &client_len);
    if (bytes_read <= 0) return;

    buf[bytes_read] = '\0';

    char *token = strtok(buf, "\r\n");
    while (token != NULL) {
        size_t len = strlen(token);
        if (len > 0 && len < BUF_SIZE) {
            memset(cmd_buf, 0, BUF_SIZE);
            strncpy(cmd_buf, token, BUF_SIZE - 1);

            memset(cmd_output, 0, BUF_SIZE);
            // Pass cmd_output buffer to collect output from the command processor
            process_ascii_cmd(mod, cmd_buf, cmd_output);

            // Send cmd_output back to client (even if empty, send it)
            size_t out_len = strlen(cmd_output);
            if (out_len > 0) {
                ssize_t sent = sendto(sockfd, cmd_output, out_len, 0,
                                      (struct sockaddr *)&client_addr, client_len);
                if (sent == -1) {
                    perror("sendto");
                }
            }
        }
        token = strtok(NULL, "\r\n");
    }
}

void close_udp_server() {
    if (sockfd >= 0) close(sockfd);
    sockfd = -1;
}
