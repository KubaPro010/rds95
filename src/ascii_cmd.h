#include "common.h"
#include "rds.h"
#include "modulator.h"
#include "lib.h"
#define CMD_BUFFER_SIZE	255
#define CTL_BUFFER_SIZE	(CMD_BUFFER_SIZE * 2)
#define READ_TIMEOUT_MS	200

void process_ascii_cmd(RDSModulator* mod, char *str, char *cmd_output);