#include "modulator.h"
#include "rds.h"
#define CMD_BUFFER_SIZE	255
#define CTL_BUFFER_SIZE	(CMD_BUFFER_SIZE * 2)
#define READ_TIMEOUT_MS	200

extern void process_ascii_cmd(RDSModulator* mod, char *str, char *cmd_output);