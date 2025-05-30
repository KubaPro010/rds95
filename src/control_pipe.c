#include "control_pipe.h"

static int fd;
static struct pollfd poller;

int open_control_pipe(char *filename) {
	fd = open(filename, O_RDONLY | O_NONBLOCK);
	if (fd == -1) return -1;
	
	poller.fd = fd;
	poller.events = POLLIN;
	return 0;
}

void poll_control_pipe(RDSModulator* mod) {
	static char pipe_buf[CTL_BUFFER_SIZE];
	static char cmd_buf[CMD_BUFFER_SIZE];
	int bytes_read;
	char *token;
	
	if (poll(&poller, 1, READ_TIMEOUT_MS) <= 0) return;
	if (!(poller.revents & POLLIN)) return;
	
	memset(pipe_buf, 0, CTL_BUFFER_SIZE);
	bytes_read = read(fd, pipe_buf, CTL_BUFFER_SIZE - 1);
	
	if (bytes_read <= 0) return;
	
	token = strtok((char *)pipe_buf, "\r\n");
	if(token == NULL) token = strtok((char *)pipe_buf, "\r");
	while (token != NULL) {
		size_t cmd_len = strlen(token);
		if (cmd_len > 0 && cmd_len < CMD_BUFFER_SIZE) {
			memset(cmd_buf, 0, CMD_BUFFER_SIZE);
			strncpy((char *)cmd_buf, token, CMD_BUFFER_SIZE - 1);
			process_ascii_cmd(mod, cmd_buf, NULL);
		}
		token = strtok(NULL, "\r\n");
		if(token == NULL) token = strtok(NULL, "\r");
	}
}

void close_control_pipe() {
	if (fd > 0) close(fd);
	fd = -1;
}