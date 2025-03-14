#include "common.h"
#include "rds.h"
#include "modulator.h"
#include "lib.h"
#include "ascii_cmd.h"

#define CMD_MATCHES(a) (ustrcmp(cmd, (unsigned char *)a) == 0)

typedef struct {
    const char *cmd;
    void (*handler)(unsigned char *arg, RDSModulator* mod);
    uint8_t cmd_length;
} command_handler_t;

static void handle_ptyn(unsigned char *arg, RDSModulator* mod) {
    arg[PTYN_LENGTH] = 0;
    set_rds_ptyn(mod->enc, xlat(arg));
}

static void handle_afch(unsigned char *arg, RDSModulator* mod) {
    if (arg[0] == 'A' || arg[0] == 'B') {
        return;
    }
    if(arg[0] == '\0') {
        memset(&(mod->enc->data[mod->enc->program].af), 0, sizeof(mod->enc->data[mod->enc->program].af));
        return;
    }
    
    memset(&(mod->enc->data[mod->enc->program].af), 0, sizeof(mod->enc->data[mod->enc->program].af));
    uint8_t arg_count;
    RDSAFs new_af;
    uint8_t af[MAX_AFS], *af_iter;
    
    arg_count = sscanf((char *)arg,
        "%hhx,%hhx,%hhx,%hhx,%hhx,"
        "%hhx,%hhx,%hhx,%hhx,%hhx,"
        "%hhx,%hhx,%hhx,%hhx,%hhx,"
        "%hhx,%hhx,%hhx,%hhx,%hhx,"
        "%hhx,%hhx,%hhx,%hhx,%hhx",
        &af[0],  &af[1],  &af[2],  &af[3],  &af[4],
        &af[5],  &af[6],  &af[7],  &af[8],  &af[9],
        &af[10], &af[11], &af[12], &af[13], &af[14],
        &af[15], &af[16], &af[17], &af[18], &af[19],
        &af[20], &af[21], &af[22], &af[23], &af[24]);
    
    af_iter = af;
    memset(&new_af, 0, sizeof(RDSAFs));
    
    while (arg_count-- != 0) {
        uint8_t current_value = *af_iter;
        float frequency = (875.0 + current_value) / 10.0;
        add_rds_af(&new_af, frequency);
        af_iter++;
    }
    
    memcpy(&(mod->enc->data[mod->enc->program].af), &new_af, sizeof(mod->enc->data[mod->enc->program].af));
}

static void handle_tps(unsigned char *arg, RDSModulator* mod) {
    arg[PS_LENGTH * 2] = 0;
    set_rds_tps(mod->enc, xlat(arg));
}

static void handle_rt1(unsigned char *arg, RDSModulator* mod) {
    arg[RT_LENGTH * 2] = 0;
    set_rds_rt1(mod->enc, xlat(arg));
}

static void handle_pty(unsigned char *arg, RDSModulator* mod) {
    arg[2] = 0;
    mod->enc->data[mod->enc->program].pty = strtoul((char *)arg, NULL, 10);
}

static void handle_ecc(unsigned char *arg, RDSModulator* mod) {
    arg[2] = 0;
    mod->enc->data[mod->enc->program].ecc = strtoul((char *)arg, NULL, 16);
}

static void handle_lic(unsigned char *arg, RDSModulator* mod) {
    arg[2] = 0;
    mod->enc->data[mod->enc->program].lic = strtoul((char *)arg, NULL, 16);
}

static void handle_rtp(unsigned char *arg, RDSModulator* mod) {
    char tag_names[2][32];
    uint8_t tags[6];
    
    if (sscanf((char *)arg, "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu",
        &tags[0], &tags[1], &tags[2], &tags[3], &tags[4], &tags[5]) == 6) {
        set_rds_rtplus_tags(mod->enc, tags);
    } else if (sscanf((char *)arg, "%31[^,],%hhu,%hhu,%31[^,],%hhu,%hhu",
        tag_names[0], &tags[1], &tags[2], tag_names[1], &tags[4], &tags[5]) == 6) {
        tags[0] = get_rtp_tag_id(tag_names[0]);
        tags[3] = get_rtp_tag_id(tag_names[1]);
        set_rds_rtplus_tags(mod->enc, tags);
    }
}

static void handle_lps(unsigned char *arg, RDSModulator* mod) {
    arg[LPS_LENGTH] = 0;
    set_rds_lps(mod->enc, arg);
}

static void handle_pin(unsigned char *arg, RDSModulator* mod) {
    uint8_t pin[3];
    if (sscanf((char *)arg, "%hhu,%hhu,%hhu", &pin[0], &pin[1], &pin[2]) == 3) {
        for (int i = 0; i < 3; i++) {
            mod->enc->data[mod->enc->program].pin[i+1] = pin[i];
        }
    }
}

static void handle_ps(unsigned char *arg, RDSModulator* mod) {
    arg[PS_LENGTH * 2] = 0;
    set_rds_ps(mod->enc, xlat(arg));
}

static void handle_ct(unsigned char *arg, RDSModulator* mod) {
    arg[2] = 1;
    mod->enc->data[mod->enc->program].ct = arg[0];
}

static void handle_di(unsigned char *arg, RDSModulator* mod) {
    arg[2] = 0;
    mod->enc->data[mod->enc->program].di = arg[0];
}

static void handle_tp(unsigned char *arg, RDSModulator* mod) {
    arg[1] = 0;
    mod->enc->data[mod->enc->program].tp = arg[0];
}

static void handle_ta(unsigned char *arg, RDSModulator* mod) {
    arg[1] = 0;
    mod->enc->data[mod->enc->program].ta = arg[0];
}

static void handle_ms(unsigned char *arg, RDSModulator* mod) {
    arg[1] = 0;
    mod->enc->data[mod->enc->program].ms = arg[0];
}

static void handle_pi(unsigned char *arg, RDSModulator* mod) {
    arg[4] = 0;
    mod->enc->data[mod->enc->program].pi = strtoul((char *)arg, NULL, 16);
}

static void handle_af(unsigned char *arg, RDSModulator* mod) {
    if (arg[0] == 'A' || arg[0] == 'B') {
        return;
    }
    if(arg[0] == '\0') {
        memset(&(mod->enc->data[mod->enc->program].af), 0, sizeof(mod->enc->data[mod->enc->program].af)); 
        return;
    }
    
    memset(&(mod->enc->data[mod->enc->program].af), 0, sizeof(mod->enc->data[mod->enc->program].af));
    uint8_t arg_count;
    RDSAFs new_af;
    float af[MAX_AFS], *af_iter;
    
    arg_count = sscanf((char *)arg,
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

    while (arg_count-- != 0) {
        add_rds_af(&new_af, *af_iter++);
    }
    
    memcpy(&(mod->enc->data[mod->enc->program].af), &new_af, sizeof(mod->enc[mod->enc->program].data->af));
}

static void handle_g(unsigned char *arg, RDSModulator* mod) {
    uint16_t blocks[3];
    int count = sscanf((char *)arg, "%4hx%4hx%4hx", &blocks[0], &blocks[1], &blocks[2]);
    if (count == 3) {
        mod->enc->state[mod->enc->program].custom_group[0] = 1;
        mod->enc->state[mod->enc->program].custom_group[1] = blocks[0];
        mod->enc->state[mod->enc->program].custom_group[2] = blocks[1];
        mod->enc->state[mod->enc->program].custom_group[3] = blocks[2];
    }
}

static void handle_pinen(unsigned char *arg, RDSModulator* mod) {
    arg[1] = 0;
    mod->enc->data[mod->enc->program].pin[0] = arg[0];
}

static void handle_rt1en(unsigned char *arg, RDSModulator* mod) {
    arg[1] = 0;
    mod->enc->data[mod->enc->program].rt1_enabled = arg[0];
}

static void handle_ptynen(unsigned char *arg, RDSModulator* mod) {
    arg[1] = 0;
    mod->enc->data[mod->enc->program].ptyn_enabled = strtoul((char *)arg, NULL, 10);
}

static void handle_rtprun(unsigned char *arg, RDSModulator* mod) {
    arg[1] = 0;
    set_rds_rtplus_flags(mod->enc, strtoul((char *)arg, NULL, 10));
}

static void handle_eccen(unsigned char *arg, RDSModulator* mod) {
    arg[1] = 0;
    mod->enc->data[mod->enc->program].ecclic_enabled = arg[0];
}

static void handle_shortrt(unsigned char *arg, RDSModulator* mod) {
    arg[1] = 0;
    mod->enc->data[mod->enc->program].shortrt = arg[0];
}

static void handle_program(unsigned char *arg, RDSModulator* mod) {
    int16_t program = strtol((char *)arg, NULL, 10)-1;
    if(program >= PROGRAMS) program = (PROGRAMS-1);
    if(program < 0) program = 0;
    mod->enc->data[mod->enc->program].ta = 0;
    mod->enc->data[(uint8_t)program].ta = 0;
    mod->enc->program = (uint8_t)program;
}

static void handle_grpseq(unsigned char *arg, RDSModulator* mod) {
    if (arg[0] == '\0') {
        memset(&(mod->enc->data[mod->enc->program].grp_sqc), 0, 24);
        memcpy(&(mod->enc->data[mod->enc->program].grp_sqc), (unsigned char*)DEFAULT_GRPSQC, sizeof(DEFAULT_GRPSQC));
    } else {
        memset(&(mod->enc->data[mod->enc->program].grp_sqc), 0, 24);
        memcpy(&(mod->enc->data[mod->enc->program].grp_sqc), arg, 24);
    }
}

static void handle_level(unsigned char *arg, RDSModulator* mod) {
    mod->params.level = strtoul((char *)arg, NULL, 10)/255.0f;
}

static void handle_rdsgen(unsigned char *arg, RDSModulator* mod) {
    mod->params.rdsgen = strtoul((char *)arg, NULL, 10);
}

static void handle_udg1(unsigned char *arg, RDSModulator* mod) {
    uint16_t blocks[8][3];
    int sets = 0;
    unsigned char *ptr = arg;
    
    while (sets < 8) {
        int count = sscanf((char *)ptr, "%4hx%4hx%4hx", 
                          &blocks[sets][0], &blocks[sets][1], &blocks[sets][2]);
        
        if (count != 3) {
            break;
        }
        
        sets++;
        
        while (*ptr && *ptr != ',') {
            ptr++;
        }
        
        if (*ptr == ',') {
            ptr++;
        } else {
            break;
        }
    }
    
    memcpy(&(mod->enc->data[mod->enc->program].udg1), &blocks, sets * sizeof(uint16_t[3]));
    mod->enc->data[mod->enc->program].udg1_len = sets;
}
static void handle_udg2(unsigned char *arg, RDSModulator* mod) {
    uint16_t blocks[8][3];
    int sets = 0;
    unsigned char *ptr = arg;
    
    while (sets < 8) {
        int count = sscanf((char *)ptr, "%4hx%4hx%4hx", 
                          &blocks[sets][0], &blocks[sets][1], &blocks[sets][2]);
        
        if (count != 3) {
            break;
        }
        
        sets++;
        
        while (*ptr && *ptr != ',') {
            ptr++;
        }
        
        if (*ptr == ',') {
            ptr++;
        } else {
            break;
        }
    }
    
    memcpy(&(mod->enc->data[mod->enc->program].udg2), &blocks, sets * sizeof(uint16_t[3]));
    mod->enc->data[mod->enc->program].udg2_len = sets;
}

static void handle_init(unsigned char *arg, RDSModulator* mod) {
    (void)arg;
    set_rds_defaults(mod->enc, mod->enc->program);
}

static const command_handler_t commands_eq3[] = {
    {"MS", handle_ms, 2},
    {"PS", handle_ps, 2},
    {"PI", handle_pi, 2},
    {"TP", handle_tp, 2},
    {"TA", handle_ta, 2},
    {"DI", handle_di, 2},
    {"CT", handle_ct, 2},
    {"AF", handle_af, 2}
};

static const command_handler_t commands_eq4[] = {
    {"TPS", handle_tps, 3},
    {"RT1", handle_rt1, 3},
    {"PTY", handle_pty, 3},
    {"ECC", handle_ecc, 3},
    {"LIC", handle_lic, 3},
    {"RTP", handle_rtp, 3},
    {"LPS", handle_lps, 3},
    {"PIN", handle_pin, 3}
};

static const command_handler_t commands_eq5[] = {
    {"TEXT", handle_rt1, 4},
    {"PTYN", handle_ptyn, 4},
    {"AFCH", handle_afch, 4},
    {"UDG1", handle_udg1, 4},
    {"UDG2", handle_udg2, 4},
};

static const command_handler_t commands_eq2[] = {
    {"G", handle_g, 1}
};

static const command_handler_t commands_eq6[] = {
    {"PINEN", handle_pinen, 5},
    {"RT1EN", handle_rt1en, 5},
    {"ECCEN", handle_eccen, 5},
    {"LEVEL", handle_level, 5}
};

static const command_handler_t commands_eq7[] = {
    {"PTYNEN", handle_ptynen, 6},
    {"RTPRUN", handle_rtprun, 6},
    {"GRPSEQ", handle_grpseq, 6},
    {"RDSGEN", handle_rdsgen, 6}
};

static const command_handler_t commands_eq8[] = {
    {"SHORTRT", handle_shortrt, 7},
    {"PROGRAM", handle_program, 7}
};

static const command_handler_t commands_exact[] = {
    {"INIT", handle_init, 4}
    // TODO: handle help, ver, status
};

static bool process_command_table(const command_handler_t *table, int table_size, 
                                 unsigned char *cmd, unsigned char *arg, RDSModulator* mod) {
    for (int i = 0; i < table_size; i++) {
        if (ustrcmp(cmd, (unsigned char *)table[i].cmd) == 0) {
            table[i].handler(arg, mod);
            return true;
        }
    }
    return false;
}

void process_ascii_cmd(RDSModulator* mod, unsigned char *str) {
    unsigned char *cmd, *arg;
    uint16_t cmd_len = _strnlen((const char*)str, CTL_BUFFER_SIZE);

    for (size_t i = 0; i < sizeof(commands_exact) / sizeof(command_handler_t); i++) {
        const command_handler_t *handler = &commands_exact[i];
        if (cmd_len == handler->cmd_length && 
            ustrcmp(str, (unsigned char *)handler->cmd) == 0) {
            handler->handler(NULL, mod);
            return;
        }
    }

    if (str[0] == '*' && !strchr((const char*)str, '=')) {
        str++;
        char option[32] = {0};
        snprintf(option, sizeof(option), "%s", (const char*)str);
        saveToFile(mod->enc, option);
        Modulator_saveToFile(&mod->params, option);
        return;
    }

    if (cmd_len > 1 && str[1] == '=') {
        cmd = str;
        cmd[1] = 0;
        arg = str + 2;
        
        if (process_command_table(commands_eq2,
                                  sizeof(commands_eq2) / sizeof(command_handler_t),
                                  cmd, arg, mod)) {
        }
    }

    if (cmd_len > 2 && str[2] == '=') {
        cmd = str;
        cmd[2] = 0;
        arg = str + 3;
        
        if (process_command_table(commands_eq3,
                                  sizeof(commands_eq3) / sizeof(command_handler_t),
                                  cmd, arg, mod)) {
        }
    }

    if (cmd_len > 3 && str[3] == '=') {
        cmd = str;
        cmd[3] = 0;
        arg = str + 4;
        
        if (process_command_table(commands_eq4,
                                  sizeof(commands_eq4) / sizeof(command_handler_t),
                                  cmd, arg, mod)) {
        }
    }

    if (cmd_len > 4 && str[4] == '=') {
        cmd = str;
        cmd[4] = 0;
        arg = str + 5;
        
        if (process_command_table(commands_eq5,
                                  sizeof(commands_eq5) / sizeof(command_handler_t),
                                  cmd, arg, mod)) {
        }
    }

    if (cmd_len > 5 && str[5] == '=') {
        cmd = str;
        cmd[5] = 0;
        arg = str + 6;
        
        if (process_command_table(commands_eq6,
                                  sizeof(commands_eq6) / sizeof(command_handler_t),
                                  cmd, arg, mod)) {
        }
    }

    if (cmd_len > 6 && str[6] == '=') {
        cmd = str;
        cmd[6] = 0;
        arg = str + 7;
        
        if (process_command_table(commands_eq7,
                                  sizeof(commands_eq7) / sizeof(command_handler_t),
                                  cmd, arg, mod)) {
        }
    }

    if (cmd_len > 7 && str[7] == '=') {
        cmd = str;
        cmd[7] = 0;
        arg = str + 8;
        
        if (process_command_table(commands_eq8,
                                  sizeof(commands_eq8) / sizeof(command_handler_t),
                                  cmd, arg, mod)) {
        }
    }
}