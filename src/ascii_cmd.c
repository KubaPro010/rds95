#include "ascii_cmd.h"

typedef struct {
	const char *cmd;
	void (*handler)(char *arg, RDSModulator* mod, char* output);
	uint8_t cmd_length;
} command_handler_t;

typedef struct {
	const char *prefix;
	const char *suffix;
	void (*handler)(char *arg, char *pattern, RDSModulator* mod, char* output);
} pattern_command_handler_t;

#define SIMPLE_INT_HANDLER(name, field) \
static void handle_##name(char *arg, RDSModulator* mod, char* output) { \
    mod->enc->data[mod->enc->program].field = atoi(arg); \
    strcpy(output, "+"); \
}
#define STRING_HANDLER(name, length, function) \
static void handle_##name(char *arg, RDSModulator* mod, char* output) { \
    arg[length*2] = 0; \
    function(mod->enc, convert_to_rdscharset(arg)); \
    strcpy(output, "+"); \
}
#define RAW_STRING_HANDLER(name, length, function) \
static void handle_##name(char *arg, RDSModulator* mod, char* output) { \
    arg[length*2] = 0; \
    function(mod->enc, arg); \
    strcpy(output, "+"); \
}
#define HEX_HANDLER(name, field) \
static void handle_##name(char *arg, RDSModulator* mod, char* output) { \
	mod->enc->data[mod->enc->program].field = strtoul(arg, NULL, 16); \
	strcpy(output, "+"); \
}

#define AF_HANDLER(name, af_struct, af_entry, add_func) \
static void handle_##name(char *arg, RDSModulator* mod, char* output) { \
	if (arg[0] == 0) { \
		memset(&(mod->enc->data[mod->enc->program].af_entry), 0, sizeof(mod->enc->data[mod->enc->program].af_entry)); \
		return; \
	} \
	\
	uint8_t arg_count; \
	af_struct new_af; \
	float af[MAX_AFS], *af_iter; \
	\
	arg_count = sscanf(arg, \
		"%f,%f,%f,%f,%f," \
		"%f,%f,%f,%f,%f," \
		"%f,%f,%f,%f,%f," \
		"%f,%f,%f,%f,%f," \
		"%f,%f,%f,%f,%f", \
		&af[0],  &af[1],  &af[2],  &af[3],  &af[4], \
		&af[5],  &af[6],  &af[7],  &af[8],  &af[9], \
		&af[10], &af[11], &af[12], &af[13], &af[14], \
		&af[15], &af[16], &af[17], &af[18], &af[19], \
		&af[20], &af[21], &af[22], &af[23], &af[24]); \
	\
	if (arg_count <= 0 || arg_count > MAX_AFS) { \
		strcpy(output, "-"); \
		return; \
	} \
	\
	memset(&new_af, 0, sizeof(af_struct)); \
	af_iter = af; \
	while (arg_count-- != 0) add_func(&new_af, *af_iter++); \
	\
	memcpy(&(mod->enc->data[mod->enc->program].af_entry), &new_af, sizeof(new_af)); \
	strcpy(output, "+"); \
}

AF_HANDLER(af, RDSAFs, af, add_rds_af)
AF_HANDLER(afo, RDSAFsODA, af_oda, add_rds_af_oda)

SIMPLE_INT_HANDLER(pty, pty)
SIMPLE_INT_HANDLER(ct, ct)
SIMPLE_INT_HANDLER(dpty, dpty)
SIMPLE_INT_HANDLER(tp, tp)
SIMPLE_INT_HANDLER(ta, ta)
SIMPLE_INT_HANDLER(rt1en, rt1_enabled)
SIMPLE_INT_HANDLER(rt2en, rt2_enabled)
SIMPLE_INT_HANDLER(ptynen, ptyn_enabled)
SIMPLE_INT_HANDLER(rttype, rt_type)

static void handle_rds2mod(char *arg, RDSModulator* mod, char* output) {
	mod->enc->encoder_data.rds2_mode = atoi(arg);
	strcpy(output, "+");
}
static void handle_rdsgen(char *arg, RDSModulator* mod, char* output) {
	mod->params.rdsgen = atoi(arg);
	strcpy(output, "+");
}

STRING_HANDLER(ptyn, PTYN_LENGTH, set_rds_ptyn)
STRING_HANDLER(ps, PS_LENGTH, set_rds_ps)
STRING_HANDLER(tps, PS_LENGTH, set_rds_tps)
STRING_HANDLER(rt1, RT_LENGTH, set_rds_rt1)
STRING_HANDLER(rt2, RT_LENGTH, set_rds_rt2)

RAW_STRING_HANDLER(lps, LPS_LENGTH, set_rds_lps)
RAW_STRING_HANDLER(ert, ERT_LENGTH, set_rds_ert)

HEX_HANDLER(ecc, ecc)
HEX_HANDLER(slcd, slc_data)

static void handle_udg(char *arg, char *pattern, RDSModulator* mod, char* output) {
	uint8_t all_scanned = 1, bad_format = 0;
	uint16_t blocks[8][3];
	int sets = 0;
	char *ptr = arg;

	while (sets < 8) {
		int count = sscanf(ptr, "%4hx%4hx%4hx", &blocks[sets][0], &blocks[sets][1], &blocks[sets][2]);
		if (count != 3) {
			all_scanned = 0;
			break;
		}
		sets++;
		while (*ptr && *ptr != ',') ptr++;
		if (*ptr == ',') ptr++;
		else {
			bad_format = 1;
			break;
		}
	}

	if (strcmp(pattern, "1") == 0) {
		memcpy(&(mod->enc->data[mod->enc->program].udg1), &blocks, sets * sizeof(uint16_t[3]));
		mod->enc->data[mod->enc->program].udg1_len = sets;
	} else if(strcmp(pattern, "2") == 0) {
		memcpy(&(mod->enc->data[mod->enc->program].udg2), &blocks, sets * sizeof(uint16_t[3]));
		mod->enc->data[mod->enc->program].udg2_len = sets;
	} else strcpy(output, "!");
	if(bad_format) strcpy(output, "-");
	else if(all_scanned) strcpy(output, "+");
	else strcpy(output, "/");
}

static void handle_udg2(char *arg, char *pattern, RDSModulator* mod, char* output) {
	uint8_t all_scanned = 1, bad_format = 0;
	uint16_t blocks[8][4];
	int sets = 0;
	char *ptr = arg;

	while (sets < 8) {
		int count = sscanf(ptr, "%4hx%4hx%4hx%4hx", &blocks[sets][0], &blocks[sets][1], &blocks[sets][2], &blocks[sets][3]);
		if (count != 4) {
			all_scanned = 0;
			break;
		}
		sets++;
		while (*ptr && *ptr != ',') ptr++;
		if (*ptr == ',') ptr++;
		else {
			bad_format = 1;
			break;
		}
	}

	if (strcmp(pattern, "1") == 0) {
		memcpy(&(mod->enc->data[mod->enc->program].udg1_rds2), &blocks, sets * sizeof(uint16_t[4]));
		mod->enc->data[mod->enc->program].udg1_len_rds2 = sets;
	} else if(strcmp(pattern, "2") == 0) {
		memcpy(&(mod->enc->data[mod->enc->program].udg2_rds2), &blocks, sets * sizeof(uint16_t[4]));
		mod->enc->data[mod->enc->program].udg2_len_rds2 = sets;
	} else strcpy(output, "!");
	if(bad_format) strcpy(output, "-");
	else if(all_scanned) strcpy(output, "+");
	else strcpy(output, "/");
}

static void handle_rtp(char *arg, RDSModulator* mod, char* output) {
	uint8_t tags[6];

	if (sscanf(arg, "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu", &tags[0], &tags[1], &tags[2], &tags[3], &tags[4], &tags[5]) == 6) {
		set_rds_rtplus_tags(mod->enc, tags);
		strcpy(output, "+");
	} else strcpy(output, "-");
}

static void handle_ertp(char *arg, RDSModulator* mod, char* output) {
	uint8_t tags[6];

	if (sscanf(arg, "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu", &tags[0], &tags[1], &tags[2], &tags[3], &tags[4], &tags[5]) == 6) {
		set_rds_ertplus_tags(mod->enc, tags);
		strcpy(output, "+");
	} else strcpy(output, "-");
}

static void handle_link(char *arg, RDSModulator* mod, char* output) {
	if(arg[0] == 0) {
		mod->enc->state[mod->enc->program].eon_linkage = 0;
		return;
	}

	mod->enc->state[mod->enc->program].eon_linkage = atoi(arg);
	strcpy(output, "+");
}

static void handle_pi(char *arg, RDSModulator* mod, char* output) {
	uint16_t pi_value = strtoul(arg, NULL, 16);
	if ((pi_value & 0xF000) == 0) {
		strcpy(output, "-");
		return;
	}
	mod->enc->data[mod->enc->program].pi = pi_value;
	strcpy(output, "+");
}

static void handle_adr(char *arg, RDSModulator* mod, char* output) {
	uint16_t ids[2];
	int count = sscanf(arg, "%4hu,%4hu", &ids[0], &ids[1]);
	if(count == 1) mod->enc->encoder_data.encoder_addr[0] = ids[0];
	else if(count == 2) {
		mod->enc->encoder_data.encoder_addr[0] = ids[0];
		mod->enc->encoder_data.encoder_addr[1] = ids[1];
	} else {
		strcpy(output, "-");
		return;
	}
	strcpy(output, "+");
}

static void handle_site(char *arg, RDSModulator* mod, char* output) {
	uint16_t ids[2];
	int count = sscanf(arg, "%4hu,%4hu", &ids[0], &ids[1]);
	if(count == 1) mod->enc->encoder_data.site_addr[0] = ids[0];
	else if(count == 2) {
		mod->enc->encoder_data.site_addr[0] = ids[0];
		mod->enc->encoder_data.site_addr[1] = ids[1];
	} else {
		strcpy(output, "-");
		return;
	}
	strcpy(output, "+");
}

static void handle_g(char *arg, RDSModulator* mod, char* output) {
	uint16_t blocks[4];
	int count = sscanf(arg, "%4hx%4hx%4hx%4hx", &blocks[0], &blocks[1], &blocks[2], &blocks[3]);
	if (count == 3) {
		mod->enc->state[mod->enc->program].custom_group[0] = 1;
		mod->enc->state[mod->enc->program].custom_group[1] = blocks[0];
		mod->enc->state[mod->enc->program].custom_group[2] = blocks[1];
		mod->enc->state[mod->enc->program].custom_group[3] = blocks[2];
	} else if(count == 4) {
		mod->enc->state[mod->enc->program].custom_group2[0] = 1;
		mod->enc->state[mod->enc->program].custom_group2[1] = blocks[0];
		mod->enc->state[mod->enc->program].custom_group2[2] = blocks[1];
		mod->enc->state[mod->enc->program].custom_group2[3] = blocks[2];
		mod->enc->state[mod->enc->program].custom_group2[4] = blocks[3];
	} else strcpy(output, "-");
	strcpy(output, "+");
}

static void handle_rtper(char *arg, RDSModulator* mod, char* output) {
	mod->enc->data[mod->enc->program].rt_switching_period = atoi(arg);
	mod->enc->state[mod->enc->program].rt_switching_period_state = mod->enc->data[mod->enc->program].rt_switching_period;
	strcpy(output, "+");
}

static void handle_rtprun(char *arg, RDSModulator *mod, char *output) {
	int flag1 = 0, flag2 = 0;
	if (sscanf(arg, "%d,%d", &flag1, &flag2) < 1) flag1 = atoi(arg);
	mod->enc->rtpData[mod->enc->program][0].enabled = (flag1 == 2);
	mod->enc->rtpData[mod->enc->program][0].running = flag1 & 1;
	if (flag2) mod->enc->rtpState[mod->enc->program][0].toggle ^= 1;
	strcpy(output, "+");
}

static void handle_ertprun(char *arg, RDSModulator* mod, char* output) {
	int flag1 = 0, flag2 = 0;
	if (sscanf(arg, "%d,%d", &flag1, &flag2) < 1) flag1 = atoi(arg);
	mod->enc->rtpData[mod->enc->program][1].enabled = (flag1 == 2);
	mod->enc->rtpData[mod->enc->program][1].running = flag1 & 1;
	if (flag2) mod->enc->rtpState[mod->enc->program][1].toggle ^= 1;
	strcpy(output, "+");
}

static void handle_program(char *arg, RDSModulator* mod, char* output) {
	int16_t program = atoi(arg)-1;
	if(program == 0) {
		strcpy(output, "-");
		return;
	}
	if(program >= PROGRAMS) program = (PROGRAMS-1);
	if(program < 0) program = 0;
	mod->enc->data[mod->enc->program].ta = 0;
	mod->enc->data[(uint8_t)program].ta = 0;
	mod->enc->program = (uint8_t)program;
	strcpy(output, "+");
}

static void handle_grpseq(char *arg, RDSModulator* mod, char* output) {
	if (arg[0] == 0) set_rds_grpseq(mod->enc, DEFAULT_GRPSQC);
	else set_rds_grpseq(mod->enc, arg);
	strcpy(output, "+");
}
static void handle_grpseq2(char *arg, RDSModulator* mod, char* output) {
	if (arg[0] == 0) set_rds_grpseq2(mod->enc, "\0");
	else set_rds_grpseq2(mod->enc, arg);
	strcpy(output, "+");
}

static void handle_dttmout(char *arg, RDSModulator* mod, char* output) {
	mod->enc->data[mod->enc->program].rt_text_timeout = atoi(arg);
	mod->enc->state[mod->enc->program].rt_text_timeout_state = mod->enc->data[mod->enc->program].rt_text_timeout;
	strcpy(output, "+");
}

static void handle_level(char *arg, RDSModulator* mod, char* output) {
	mod->params.level = atoi(arg)/255.0f;
	strcpy(output, "+");
}

static void handle_reset(char *arg, RDSModulator* mod, char* output) {
	(void)arg;
	loadFromFile(mod->enc);
	for(int i = 0; i < PROGRAMS; i++) reset_rds_state(mod->enc, i);
	Modulator_loadFromFile(&mod->params);
	strcpy(output, "+");
}

static void handle_init(char *arg, RDSModulator* mod, char* output) {
	(void)arg;
	set_rds_defaults(mod->enc, mod->enc->program);
	strcpy(output, "+");
}

static void handle_ver(char *arg, RDSModulator* mod, char* output) {
	(void)arg;
	(void)mod;
	sprintf(output, "rds95 v. %s - (C) 2025 radio95", VERSION);
}

static void handle_eonen(char *arg, char *pattern, RDSModulator* mod, char* output) {
	mod->enc->data[mod->enc->program].eon[atoi(pattern)-1].enabled = atoi(arg);
	strcpy(output, "+");
}

static void handle_eonpi(char *arg, char *pattern, RDSModulator* mod, char* output) {
	mod->enc->data[mod->enc->program].eon[atoi(pattern)-1].pi = strtoul(arg, NULL, 16);
	strcpy(output, "+");
}

static void handle_eonps(char *arg, char *pattern, RDSModulator* mod, char* output) {
	arg[PS_LENGTH * 2] = 0;

	RDSEON *eon = &mod->enc->data[mod->enc->program].eon[atoi(pattern)-1];
	memset(eon->ps, ' ', sizeof(eon->ps));

	uint16_t len = 0;
	while (*arg != 0 && len < 24) eon->ps[len++] = *arg++;

	strcpy(output, "+");
}

static void handle_eonpty(char *arg, char *pattern, RDSModulator* mod, char* output) {
	mod->enc->data[mod->enc->program].eon[atoi(pattern)-1].pty = atoi(arg);
	strcpy(output, "+");
}

static void handle_eonta(char *arg, char *pattern, RDSModulator* mod, char* output) {
	if (!mod->enc->data[mod->enc->program].eon[atoi(pattern)-1].enabled ||
		!mod->enc->data[mod->enc->program].eon[atoi(pattern)-1].tp) {
		strcpy(output, "-");
		return;
	}
	mod->enc->data[mod->enc->program].eon[atoi(pattern)-1].ta = atoi(arg);
	if(mod->enc->data[mod->enc->program].eon[atoi(pattern)-1].ta) mod->enc->data[mod->enc->program].ta = 1;
	strcpy(output, "+");
}

static void handle_eontp(char *arg, char *pattern, RDSModulator* mod, char* output) {
	mod->enc->data[mod->enc->program].eon[atoi(pattern)-1].tp = atoi(arg);
	strcpy(output, "+");
}

static void handle_eonaf(char *arg, char *pattern, RDSModulator* mod, char* output) {
	if (arg[0] == '') {
		memset(&(mod->enc->data[mod->enc->program].eon[atoi(pattern)-1].af), 0, sizeof(mod->enc->data[mod->enc->program].eon[atoi(pattern)-1].af));
		strcpy(output, "+");
		return;
	}

	memset(&(mod->enc->data[mod->enc->program].eon[atoi(pattern)-1].af), 0, sizeof(mod->enc->data[mod->enc->program].eon[atoi(pattern)-1].af));
	uint8_t arg_count;
	RDSAFs new_af;
	float af[MAX_AFS], *af_iter;

	arg_count = sscanf(arg,
		"%f,%f,%f,%f,%f,"
		"%f,%f,%f,%f,%f,"
		"%f,%f,%f,%f,%f,"
		"%f,%f,%f,%f,%f,"
		"%f,%f,%f,%f,%f",
		&af[0],  &af[1],  &af[2],  &af[3],  &af[4],
		&af[5],  &af[6],  &af[7],  &af[8],  &af[9],
		&af[10], &af[11], &af[12], &af[13], &af[14],
		&af[15], &af[16], &af[17], &af[18], &af[19],
		&af[20], &af[21], &af[22], &af[23], &af[24]);

	af_iter = af;
	memset(&new_af, 0, sizeof(RDSAFs));

	while (arg_count-- != 0) add_rds_af(&new_af, *af_iter++);

	memcpy(&(mod->enc->data[mod->enc->program].eon[atoi(pattern)-1].af), &new_af, sizeof(mod->enc->data[mod->enc->program].eon[atoi(pattern)-1].af));
	strcpy(output, "+");
}

static void handle_eondt(char *arg, char *pattern, RDSModulator* mod, char* output) {
	mod->enc->data[mod->enc->program].eon[atoi(pattern)-1].data = strtoul(arg, NULL, 16);
	strcpy(output, "+");
}

static const command_handler_t commands_eq3[] = {
	{"PS", handle_ps, 2},
	{"PI", handle_pi, 2},
	{"TP", handle_tp, 2},
	{"TA", handle_ta, 2},
	{"CT", handle_ct, 2},
	{"AF", handle_af, 2}
};

static const command_handler_t commands_eq4[] = {
	{"TPS", handle_tps, 3},
	{"RT1", handle_rt1, 3},
	{"RT2", handle_rt2, 3},
	{"PTY", handle_pty, 3},
	{"ECC", handle_ecc, 3},
	{"RTP", handle_rtp, 3},
	{"LPS", handle_lps, 3},
	{"ERT", handle_ert, 3},
	{"AFO", handle_afo, 3},
	{"ADR", handle_adr, 3}
};

static const command_handler_t commands_eq5[] = {
	{"TEXT", handle_rt1, 4},
	{"PTYN", handle_ptyn, 4},
	{"DPTY", handle_dpty, 4},
	{"SLCD", handle_slcd, 4},
	{"ERTP", handle_ertp, 4},
	{"LINK", handle_link, 4},
	{"SITE", handle_site, 4}
};

static const command_handler_t commands_eq2[] = {
	{"G", handle_g, 1}
};

static const command_handler_t commands_eq6[] = {
	{"RT1EN", handle_rt1en, 5},
	{"RT2EN", handle_rt2en, 5},
	{"RTPER", handle_rtper, 5},
	{"LEVEL", handle_level, 5},
};

static const command_handler_t commands_eq7[] = {
	{"PTYNEN", handle_ptynen, 6},
	{"RTPRUN", handle_rtprun, 6},
	{"GRPSEQ", handle_grpseq, 6},
	{"RDSGEN", handle_rdsgen, 6},
	{"RTTYPE", handle_rttype, 6},
};

static const command_handler_t commands_eq8[] = {
	{"PROGRAM", handle_program, 7},
	{"RDS2MOD", handle_rds2mod, 7},
	{"GRPSEQ2", handle_grpseq2, 7},
	{"DTTMOUT", handle_dttmout, 7},
	{"ERTPRUN", handle_ertprun, 7},
};
static const command_handler_t commands_exact[] = {
	{"INIT", handle_init, 4},
	{"VER", handle_ver, 3},
	{"RESET", handle_reset, 5},
};

static const pattern_command_handler_t pattern_commands[] = {
	{"EON", "EN", handle_eonen},
	{"EON", "PI", handle_eonpi},
	{"EON", "PS", handle_eonps},
	{"EON", "PTY", handle_eonpty},
	{"EON", "TA", handle_eonta},
	{"EON", "TP", handle_eontp},
	{"EON", "AF", handle_eonaf},
	{"EON", "DT", handle_eondt},
	{"UDG", "", handle_udg},
	{"2UDG", "", handle_udg2},
};

static bool process_command_table(const command_handler_t *table, int table_size, char *cmd, char *arg, char *output, RDSModulator* mod) {
	for (int i = 0; i < table_size; i++) {
		if (strcmp(cmd, table[i].cmd) == 0) {
			table[i].handler(arg, mod, output);
			return true;
		}
	}
	return false;
}

static bool process_pattern_commands(char *cmd, char *arg, char *output, RDSModulator* mod) {
	size_t cmd_len = strlen(cmd);
	char pattern_buffer[16] = {0};
	for (size_t i = 0; i < sizeof(pattern_commands) / sizeof(pattern_command_handler_t); i++) {
		const pattern_command_handler_t *handler = &pattern_commands[i];
		size_t prefix_len = strlen(handler->prefix);
		size_t suffix_len = strlen(handler->suffix);
		if (cmd_len > (prefix_len + suffix_len) && strncmp(cmd, handler->prefix, prefix_len) == 0 && strcmp(cmd + cmd_len - suffix_len, handler->suffix) == 0) {
			size_t pattern_len = cmd_len - prefix_len - suffix_len;
			if (pattern_len < sizeof(pattern_buffer)) {
				strncpy(pattern_buffer, cmd + prefix_len, pattern_len);
				pattern_buffer[pattern_len] = 0;
				handler->handler(arg, pattern_buffer, mod, output);
				return true;
			}
		}
	}
	return false;
}

void process_ascii_cmd(RDSModulator* mod, char *str, char *cmd_output) {
	if(mod->enc->encoder_data.ascii_data.expected_encoder_addr != 0 && mod->enc->encoder_data.ascii_data.expected_encoder_addr != 255) {
		uint8_t reached = 0;
		for(int i = 0; i < 2; i++) {
			if(mod->enc->encoder_data.encoder_addr[i] == mod->enc->encoder_data.ascii_data.expected_encoder_addr) {
				reached = 1;
				break;
			}
		}
		if(!reached) return;
	}
	if(mod->enc->encoder_data.ascii_data.expected_site_addr != 0) {
		uint8_t reached = 0;
		for(int i = 0; i < 2; i++) {
			if(mod->enc->encoder_data.site_addr[i] == mod->enc->encoder_data.ascii_data.expected_site_addr) {
				reached = 1;
				break;
			}
		}
		if(!reached) return;
	}

	char *cmd, *arg;

	char output[255];
	memset(output, 0, sizeof(output));

	char upper_str[CTL_BUFFER_SIZE];
	uint16_t cmd_len = _strnlen((const char*)str, CTL_BUFFER_SIZE);
	for(uint16_t i = 0; i < cmd_len; i++) if(str[i] == '\t') str[i] = ' ';

	strncpy(upper_str, str, CTL_BUFFER_SIZE);
	upper_str[CTL_BUFFER_SIZE-1] = 0;

	for(uint16_t i = 0; i < cmd_len && upper_str[i] != '='; i++) {
		if(upper_str[i] >= 'a' && upper_str[i] <= 'z') upper_str[i] = upper_str[i] - 'a' + 'A';
	}

	for (size_t i = 0; i < sizeof(commands_exact) / sizeof(command_handler_t); i++) {
		const command_handler_t *handler = &commands_exact[i];
		if (cmd_len == handler->cmd_length && strcmp(upper_str, handler->cmd) == 0) {
			handler->handler(NULL, mod, output);
			return;
		}
	}

	if (upper_str[0] == '*' && !strchr((const char*)upper_str, '=')) {
		const char* option_str = upper_str + 1;
		char option[32] = {0};
		size_t copy_len = strlen(option_str);
		if (copy_len >= sizeof(option)) copy_len = sizeof(option) - 1;
		memcpy(option, option_str, copy_len);
		option[copy_len] = 0;
		saveToFile(mod->enc, option);
		Modulator_saveToFile(&mod->params, option);
		return;
	}

	char *equals_pos = strchr(upper_str, '=');
	if (equals_pos != NULL) {
		cmd = upper_str;
		cmd[equals_pos - upper_str] = 0;
		arg = equals_pos + 1;
		process_pattern_commands(cmd, arg, output, mod);
	}

	uint8_t cmd_reached = 0;

	for (int eq_pos = 1; eq_pos <= 7; ++eq_pos) {
		if (cmd_len > eq_pos && str[eq_pos] == '=') {
			cmd = upper_str;
			cmd[eq_pos] = 0;
			arg = str + eq_pos + 1;

			const command_handler_t* table = NULL;
			size_t table_size = 0;

			switch (eq_pos) {
				case 1:
					table = commands_eq2;
					table_size = sizeof(commands_eq2) / sizeof(command_handler_t);
					break;
				case 2:
					table = commands_eq3;
					table_size = sizeof(commands_eq3) / sizeof(command_handler_t);
					break;
				case 3:
					table = commands_eq4;
					table_size = sizeof(commands_eq4) / sizeof(command_handler_t);
					break;
				case 4:
					table = commands_eq5;
					table_size = sizeof(commands_eq5) / sizeof(command_handler_t);
					break;
				case 5:
					table = commands_eq6;
					table_size = sizeof(commands_eq6) / sizeof(command_handler_t);
					break;
				case 6:
					table = commands_eq7;
					table_size = sizeof(commands_eq7) / sizeof(command_handler_t);
					break;
				case 7:
					table = commands_eq8;
					table_size = sizeof(commands_eq8) / sizeof(command_handler_t);
					break;
			}

			process_command_table(table, table_size, cmd, arg, output, mod);
			cmd_reached = 1;
			break;
		}
	}

	if (cmd_output != NULL && cmd_reached) strcpy(cmd_output, output);
	if (!cmd_reached) strcpy(output, "?");
}