#include <fcntl.h>
#include <poll.h>
#include <sys/select.h>
#include "common.h"
#include "ascii_cmd.h"
#include "rds.h"
#include "modulator.h"

int open_control_pipe(char *filename);
void close_control_pipe();
void poll_control_pipe(RDSModulator* mod);
