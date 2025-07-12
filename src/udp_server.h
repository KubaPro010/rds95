#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "modulator.h"
#include "ascii_cmd.h"

int open_udp_server(int port, RDSModulator *rds_mod);

void poll_udp_server();

void close_udp_server();
