#include "common.h"
#include "ascii_cmd.h"
#include "control_pipe.h"

static int fd = -1;
static struct pollfd poller;

/*
 * Opens a file (pipe) to be used to control the RDS coder.
 * Returns 0 on success, -1 on failure.
 */
int open_control_pipe(char *filename) {
    if (!filename) return -1;
    
    // Close existing pipe if open
    if (fd >= 0) close(fd);
    
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
void poll_control_pipe() {
    static unsigned char pipe_buf[CTL_BUFFER_SIZE];
    static unsigned char cmd_buf[CMD_BUFFER_SIZE];
    char *token, *saveptr;
    int read_bytes;

    // Return early if file descriptor is invalid
    if (fd < 0) return;

    // Check for new commands with a single poll call
    if (poll(&poller, 1, READ_TIMEOUT_MS) <= 0) return;

    // Return early if there are no new commands
    if (!(poller.revents & POLLIN)) return;

    // Clear buffer before reading
    memset(pipe_buf, 0, CTL_BUFFER_SIZE);
    
    // Read data directly - select is redundant with poll already used above
    read_bytes = read(fd, pipe_buf, CTL_BUFFER_SIZE - 1);
    if (read_bytes <= 0) return;
    
    // Ensure null-termination
    pipe_buf[read_bytes] = '\0';

    /* Process commands line by line */
    token = strtok_r((char *)pipe_buf, "\n", &saveptr);
    while (token != NULL) {
        size_t cmd_len = strlen(token);
        if (cmd_len > 0 && cmd_len < CMD_BUFFER_SIZE) {
            memset(cmd_buf, 0, CMD_BUFFER_SIZE);
            memcpy(cmd_buf, token, cmd_len);
            process_ascii_cmd(cmd_buf);
        }
        token = strtok_r(NULL, "\n", &saveptr);
    }
}

/*
 * Closes the control pipe.
 */
void close_control_pipe() {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}