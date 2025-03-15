#include "common.h"
#include "ascii_cmd.h"
#include "control_pipe.h"
#include "rds.h"
#include "modulator.h"

static int fd;
static struct pollfd poller;

/*
 * Opens a file (pipe) to be used to control the RDS coder.
 */
int open_control_pipe(char *filename) {
    fd = open(filename, O_RDONLY | O_NONBLOCK);
    if (fd == -1) return -1;
    
    /* setup the poller */
    poller.fd = fd;
    poller.events = POLLIN;
    return 0;
}

/*
 * Polls the control file (pipe), and if a command is received,
 * calls process_ascii_cmd.
 */
void poll_control_pipe(RDSModulator* mod) {
    static unsigned char pipe_buf[CTL_BUFFER_SIZE];
    static unsigned char cmd_buf[CMD_BUFFER_SIZE];
    int bytes_read;
    char *token;
    
    /* check for new commands - return early if none */
    if (poll(&poller, 1, READ_TIMEOUT_MS) <= 0) return;
    if (!(poller.revents & POLLIN)) return;
    
    /* read data from pipe */
    memset(pipe_buf, 0, CTL_BUFFER_SIZE);
    bytes_read = read(fd, pipe_buf, CTL_BUFFER_SIZE - 1);
    
    if (bytes_read <= 0) return;
    
    /* process each command line */
    token = strtok((char *)pipe_buf, "\n");
    while (token != NULL) {
        size_t cmd_len = strlen(token);
        if (cmd_len > 0 && cmd_len < CMD_BUFFER_SIZE) {
            memset(cmd_buf, 0, CMD_BUFFER_SIZE);
            strncpy((char *)cmd_buf, token, CMD_BUFFER_SIZE - 1);
            process_ascii_cmd(mod, cmd_buf);
        }
        token = strtok(NULL, "\n");
    }
}

void close_control_pipe() {
    if (fd > 0) close(fd);
    fd = -1;
}