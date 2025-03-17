#include "common.h"
#include "rds.h"
#include "modulator.h"
#include "lib.h"
#include <time.h>

void saveToFile(RDSEncoder *emp, const char *option) {
    char encoderPath[256];
    snprintf(encoderPath, sizeof(encoderPath), "%s/.rdsEncoder", getenv("HOME"));
    FILE *file;
    
    RDSEncoder tempEncoder;
    file = fopen(encoderPath, "rb");
    if (file != NULL) {
        fread(&tempEncoder, sizeof(RDSEncoder), 1, file);
        fclose(file);
    } else {
        memcpy(&tempEncoder, emp, sizeof(RDSEncoder));
    }
    
    if (strcmp(option, "MS") == 0) {
        tempEncoder.data[emp->program].ms = emp->data[emp->program].ms;
    } else if (strcmp(option, "PS") == 0) {
        memcpy(tempEncoder.data[emp->program].ps, emp->data[emp->program].ps, PS_LENGTH);
    } else if (strcmp(option, "PI") == 0) {
        tempEncoder.data[emp->program].pi = emp->data[emp->program].pi;
    } else if (strcmp(option, "PTY") == 0) {
        tempEncoder.data[emp->program].pty = emp->data[emp->program].pty;
    } else if (strcmp(option, "TP") == 0) {
        tempEncoder.data[emp->program].tp = emp->data[emp->program].tp;
    } else if (strcmp(option, "TA") == 0) {
        tempEncoder.data[emp->program].ta = emp->data[emp->program].ta;
    } else if (strcmp(option, "DI") == 0) {
        tempEncoder.data[emp->program].di = emp->data[emp->program].di;
    } else if (strcmp(option, "CT") == 0) {
        tempEncoder.data[emp->program].ct = emp->data[emp->program].ct;
    } else if (strcmp(option, "RT1") == 0 || strcmp(option, "TEXT") == 0) {
        memcpy(tempEncoder.data[emp->program].rt1, emp->data[emp->program].rt1, RT_LENGTH);
        tempEncoder.data[emp->program].rt1_enabled = emp->data[emp->program].rt1_enabled;
    } else if (strcmp(option, "PTYN") == 0) {
        memcpy(tempEncoder.data[emp->program].ptyn, emp->data[emp->program].ptyn, PTYN_LENGTH);
        tempEncoder.data[emp->program].ptyn_enabled = emp->data[emp->program].ptyn_enabled;
    } else if (strcmp(option, "AF") == 0 || strcmp(option, "AFCH") == 0) {
        memcpy(&(tempEncoder.data[emp->program].af), &(emp->data[emp->program].af), sizeof(emp->data[emp->program].af));
    } else if (strcmp(option, "ECC") == 0) {
        tempEncoder.data[emp->program].ecc = emp->data[emp->program].ecc;
    } else if (strcmp(option, "LIC") == 0) {
        tempEncoder.data[emp->program].lic = emp->data[emp->program].lic;
    } else if (strcmp(option, "ECCEN") == 0) {
        tempEncoder.data[emp->program].ecclic_enabled = emp->data[emp->program].ecclic_enabled;
    } else if (strcmp(option, "TPS") == 0) {
        memcpy(tempEncoder.data[emp->program].tps, emp->data[emp->program].tps, PS_LENGTH);
	} else if (strcmp(option, "DPS1") == 0) {
        memcpy(tempEncoder.data[emp->program].dps1, emp->data[emp->program].dps1, PS_LENGTH);
        tempEncoder.data[emp->program].dps1_enabled = emp->data[emp->program].dps1_enabled;
        tempEncoder.data[emp->program].dps1_len = emp->data[emp->program].dps1_len;
        tempEncoder.data[emp->program].dps1_numberofrepeats = emp->data[emp->program].dps1_numberofrepeats;
        tempEncoder.data[emp->program].dps1_numberofrepeats_clear = emp->data[emp->program].dps1_numberofrepeats_clear;
    } else if (strcmp(option, "DPS1EN") == 0) {
        tempEncoder.data[emp->program].dps1_enabled = emp->data[emp->program].dps1_enabled;
    } else if (strcmp(option, "LPS") == 0) {
        memcpy(tempEncoder.data[emp->program].lps, emp->data[emp->program].lps, LPS_LENGTH);
    } else if (strcmp(option, "SHORTRT") == 0) {
        tempEncoder.data[emp->program].shortrt = emp->data[emp->program].shortrt;
    } else if (strcmp(option, "PIN") == 0 || strcmp(option, "PINEN") == 0) {
        memcpy(tempEncoder.data[emp->program].pin, emp->data[emp->program].pin, sizeof(emp->data[emp->program].pin));
    } else if (strcmp(option, "GRPSEQ") == 0) {
        memcpy(tempEncoder.data[emp->program].grp_sqc, emp->data[emp->program].grp_sqc, sizeof(emp->data[emp->program].grp_sqc));
    } else if (strcmp(option, "RTP") == 0) {
		tempEncoder.rtpData[emp->program].group = emp->rtpData[emp->program].group;
        memcpy(tempEncoder.rtpData[emp->program].len, emp->rtpData[emp->program].len, sizeof(emp->rtpData[emp->program].len));
        memcpy(tempEncoder.rtpData[emp->program].start, emp->rtpData[emp->program].start, sizeof(emp->rtpData[emp->program].start));
        memcpy(tempEncoder.rtpData[emp->program].type, emp->rtpData[emp->program].type, sizeof(emp->rtpData[emp->program].type));
		memcpy(&(tempEncoder.odas[emp->program]), &(emp->odas[emp->program]), sizeof(RDSODA)*MAX_ODAS);
		memcpy(&(tempEncoder.oda_state[emp->program]), &(emp->oda_state[emp->program]), sizeof(RDSODAState));
		tempEncoder.rtpData[emp->program].toggle = emp->rtpData[emp->program].toggle;
    } else if (strcmp(option, "UDG1") == 0) {
        memcpy(tempEncoder.data[emp->program].udg1, emp->data[emp->program].udg1, sizeof(emp->data[emp->program].udg1));
        tempEncoder.data[emp->program].udg1_len = emp->data[emp->program].udg1_len;
    } else if (strcmp(option, "UDG2") == 0) {
        memcpy(tempEncoder.data[emp->program].udg2, emp->data[emp->program].udg2, sizeof(emp->data[emp->program].udg2));
        tempEncoder.data[emp->program].udg2_len = emp->data[emp->program].udg2_len;
	} else if(strcmp(option, "RTPRUN") == 0) {
		tempEncoder.rtpData[emp->program].running = emp->rtpData[emp->program].running;
		tempEncoder.rtpData[emp->program].enabled = emp->rtpData[emp->program].enabled;
		memcpy(&(tempEncoder.odas[emp->program]), &(emp->odas[emp->program]), sizeof(RDSODA)*MAX_ODAS);
		memcpy(&(tempEncoder.oda_state[emp->program]), &(emp->oda_state[emp->program]), sizeof(RDSODAState));
	} else if(strcmp(option, "PTYNEN") == 0) {
		tempEncoder.data[emp->program].ptyn_enabled = emp->data[emp->program].ptyn_enabled;
	} else if(strcmp(option, "ECCEN") == 0) {
		tempEncoder.data[emp->program].ecclic_enabled = emp->data[emp->program].ecclic_enabled;
	} else if(strcmp(option, "RT1EN") == 0) {
		tempEncoder.data[emp->program].rt1_enabled = emp->data[emp->program].rt1_enabled;
	} else if(strcmp(option, "PINEN") == 0) {
		tempEncoder.data[emp->program].pin[0] = emp->data[emp->program].pin[0];
	} else if(strcmp(option, "PROGRAM") == 0) {
        tempEncoder.program = emp->program;
	} else if(strcmp(option, "EON") == 0) {
        memcpy(&(tempEncoder.data[emp->program].eon), &(emp->data[emp->program].eon), sizeof(RDSEONs)*4);
	} else if (strcmp(option, "ALL") == 0) {
        memcpy(&(tempEncoder.data[emp->program]), &(emp->data[emp->program]), sizeof(RDSData));
        memcpy(&(tempEncoder.rtpData[emp->program]), &(emp->rtpData[emp->program]), sizeof(RDSRTPlusData));
		memcpy(&(tempEncoder.odas[emp->program]), &(emp->odas[emp->program]), sizeof(RDSODA)*MAX_ODAS);
		memcpy(&(tempEncoder.oda_state[emp->program]), &(emp->oda_state[emp->program]), sizeof(RDSODAState));
		memcpy(&(tempEncoder.encoder_data[emp->program]), &(emp->encoder_data[emp->program]), sizeof(RDSODAState));
		tempEncoder.program = emp->program;
    }
    
	memcpy(&(tempEncoder.state[emp->program]), &(emp->state[emp->program]), sizeof(RDSState));

	file = fopen(encoderPath, "wb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    fwrite(&tempEncoder, sizeof(RDSEncoder), 1, file);
    fclose(file);
}

void loadFromFile(RDSEncoder *emp) {
	char encoderPath[256];
	snprintf(encoderPath, sizeof(encoderPath), "%s/.rdsEncoder", getenv("HOME"));
    FILE *file = fopen(encoderPath, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    fread(emp, sizeof(RDSEncoder), 1, file);
    fclose(file);
}

int rdssaved() {
	char encoderPath[256];
	snprintf(encoderPath, sizeof(encoderPath), "%s/.rdsEncoder", getenv("HOME"));
    FILE *file = fopen(encoderPath, "rb");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

static void register_oda(RDSEncoder* enc, uint8_t group, uint16_t aid, uint16_t scb) {
	if (enc->oda_state[enc->program].count >= MAX_ODAS) return;

	enc->odas[enc->program][enc->oda_state[enc->program].count].group = group;
	enc->odas[enc->program][enc->oda_state[enc->program].count].aid = aid;
	enc->odas[enc->program][enc->oda_state[enc->program].count].scb = scb;
	enc->oda_state[enc->program].count++;
}

static uint16_t get_next_af(RDSEncoder* enc) {
	static uint8_t af_state;
	uint16_t out;

	if (enc->data[enc->program].af.num_afs) {
		if (af_state == 0) {
			out = (AF_CODE_NUM_AFS_BASE + enc->data[enc->program].af.num_afs) << 8;
			out |= enc->data[enc->program].af.afs[0];
			af_state += 1;
		} else {
			out = enc->data[enc->program].af.afs[af_state] << 8;
			if (enc->data[enc->program].af.afs[af_state + 1])
				out |= enc->data[enc->program].af.afs[af_state + 1];
			else
				out |= AF_CODE_FILLER;
			af_state += 2;
		}
		if (af_state >= enc->data[enc->program].af.num_entries) af_state = 0;
	} else {
		out = AF_CODE_NO_AF << 8 | AF_CODE_FILLER;
	}

	return out;
}
// #endregion

// #region Group encoding
static void get_rds_ps_group(RDSEncoder* enc, uint16_t *blocks) {
	if (enc->state[enc->program].ps_csegment == 0 && enc->state[enc->program].ps_update && !enc->data[enc->program].dps1_enabled) {
		memcpy(enc->state[enc->program].ps_text, enc->data[enc->program].ps, PS_LENGTH);
		enc->state[enc->program].ps_update = 0;
	}
	if(enc->state[enc->program].ps_csegment == 0 && enc->state[enc->program].tps_update) {
		memcpy(enc->state[enc->program].tps_text, enc->data[enc->program].tps, PS_LENGTH);
		enc->state[enc->program].tps_update = 0;
	}
	if(enc->state[enc->program].ps_csegment == 0 && enc->state[enc->program].dps1_update && enc->data[enc->program].dps1_enabled) {
		memcpy(enc->state[enc->program].dps1_text, enc->data[enc->program].dps1, PS_LENGTH);
		enc->state[enc->program].dps1_update= 0;
	}

	if(enc->data[enc->program].dps1_enabled &&
		 enc->state[enc->program].ps_csegment == 0 && enc->state[enc->program].dynamic_ps_state != 0) {
		// Copy Static PS
		memcpy(enc->state[enc->program].ps_text, enc->data[enc->program].ps, PS_LENGTH);

		if(enc->state[enc->program].static_ps_period == enc->data[enc->program].static_ps_period) {
			enc->state[enc->program].dynamic_ps_state = 1;
			enc->state[enc->program].static_ps_period = 0;
		}
	}
	if(enc->data[enc->program].dps1_enabled &&
		 enc->state[enc->program].ps_csegment == 0 && enc->state[enc->program].dynamic_ps_state != 1) {
		// Copy DPS1
		memcpy(enc->state[enc->program].ps_text, &(enc->state[enc->program].dps1_text[enc->state->dynamic_ps_position]), PS_LENGTH);
		switch (enc->data[enc->program].dps1_mode)
		{
		case 0:
			enc->state[enc->program].dynamic_ps_position += PS_LENGTH;
			break;
		case 1:
			enc->state[enc->program].dynamic_ps_position += 1;
			break;
		// TODO: Mode 2 and 3
		}
		if(enc->state[enc->program].dynamic_ps_position >= enc->data[enc->program].dps1_len) enc->state[enc->program].dynamic_ps_position = 0;
		if(enc->state[enc->program].dynamic_ps_period == enc->data[enc->program].dynamic_ps_period) {
			enc->state[enc->program].dynamic_ps_state = 0; // Static
			enc->state[enc->program].dynamic_ps_period = 0;
		}
	}

	blocks[1] |= enc->data[enc->program].ta << 4;
	blocks[1] |= enc->data[enc->program].ms << 3;
	blocks[1] |= ((enc->data[enc->program].di >> (3 - enc->state[enc->program].ps_csegment)) & 1) << 2;
	blocks[1] |= enc->state[enc->program].ps_csegment;
	blocks[2] = get_next_af(enc);
	if(enc->data[enc->program].ta && enc->state[enc->program].tps_text[0] != '\0') {
		blocks[3] = enc->state[enc->program].tps_text[enc->state[enc->program].ps_csegment * 2] << 8 | enc->state[enc->program].tps_text[enc->state[enc->program].ps_csegment * 2 + 1];
	} else {
		blocks[3] = enc->state[enc->program].ps_text[enc->state[enc->program].ps_csegment * 2] << 8 |  enc->state[enc->program].ps_text[enc->state[enc->program].ps_csegment * 2 + 1];
	}

	enc->state[enc->program].ps_csegment++;
	if (enc->state[enc->program].ps_csegment >= 4) enc->state[enc->program].ps_csegment = 0;
}

static uint8_t get_rds_rt_group(RDSEncoder* enc, uint16_t *blocks) {
	if (enc->state[enc->program].rt_update) {
		memcpy(enc->state[enc->program].rt_text, enc->data[enc->program].rt1, RT_LENGTH);
		enc->state[enc->program].rt_ab ^= 1;
		enc->state[enc->program].rt_update = 0;
		enc->state[enc->program].rt_state = 0;
	}

	blocks[1] |= 2 << 12;
	blocks[1] |= enc->state[enc->program].rt_ab << 4;
	blocks[1] |= enc->state[enc->program].rt_state;
	blocks[2] =  enc->state[enc->program].rt_text[enc->state[enc->program].rt_state * 4    ] << 8;
	blocks[2] |= enc->state[enc->program].rt_text[enc->state[enc->program].rt_state * 4 + 1];
	blocks[3] =  enc->state[enc->program].rt_text[enc->state[enc->program].rt_state * 4 + 2] << 8;
	blocks[3] |= enc->state[enc->program].rt_text[enc->state[enc->program].rt_state * 4 + 3];

	enc->state[enc->program].rt_state++;
	if (enc->state[enc->program].rt_state >= enc->state[enc->program].rt_segments) enc->state[enc->program].rt_state = 0;
	return 1;
}

static void get_rds_oda_group(RDSEncoder* enc, uint16_t *blocks) {
	RDSODA this_oda = enc->odas[enc->program][enc->oda_state[enc->program].current];

	blocks[1] |= 3 << 12;

	blocks[1] |= GET_GROUP_TYPE(this_oda.group) << 1;
	blocks[1] |= GET_GROUP_VER(this_oda.group);
	blocks[2] = this_oda.scb;
	blocks[3] = this_oda.aid;

	enc->oda_state[enc->program].current++;
	if (enc->oda_state[enc->program].current >= enc->oda_state[enc->program].count) enc->oda_state[enc->program].current = 0;
}

static uint8_t get_rds_ct_group(RDSEncoder* enc, uint16_t *blocks) {
	struct tm *utc, *local_time;
	time_t now;
	uint8_t l;
	uint32_t mjd;
	int16_t offset;

	now = time(NULL);
	utc = gmtime(&now);

	if (utc->tm_min != enc->state[enc->program].last_ct_minute) {
		enc->state[enc->program].last_ct_minute = utc->tm_min;

		l = utc->tm_mon <= 1 ? 1 : 0;
		mjd = 14956 + utc->tm_mday +
			(uint32_t)((utc->tm_year - l) * 365.25f) +
			(uint32_t)((utc->tm_mon + (1+1) + l * 12) * 30.6001f);

		blocks[1] |= 4 << 12 | (mjd >> 15);
		blocks[2] = (mjd << 1) | (utc->tm_hour >> 4);
		blocks[3] = (utc->tm_hour & 0xf) << 12 | utc->tm_min << 6;

		local_time = localtime(&now);

		offset = local_time->__tm_gmtoff / (30 * 60);
		if (offset < 0) blocks[3] |= 1 << 5;
		blocks[3] |= abs(offset);

		return 1;
	}

	return 0;
}

static void get_rds_ptyn_group(RDSEncoder* enc, uint16_t *blocks) {
	if (enc->state[enc->program].ptyn_state == 0 && enc->state[enc->program].ptyn_update) {
		memcpy(enc->state[enc->program].ptyn_text, enc->data[enc->program].ptyn, PTYN_LENGTH);
		enc->state[enc->program].ptyn_ab ^= 1;
		enc->state[enc->program].ptyn_update = 0;
	}

	blocks[1] |= 10 << 12 | enc->state[enc->program].ptyn_state;
	blocks[1] |= enc->state[enc->program].ptyn_ab << 4;
	blocks[2] =  enc->state[enc->program].ptyn_text[enc->state[enc->program].ptyn_state * 4 + 0] << 8;
	blocks[2] |= enc->state[enc->program].ptyn_text[enc->state[enc->program].ptyn_state * 4 + 1];
	blocks[3] =  enc->state[enc->program].ptyn_text[enc->state[enc->program].ptyn_state * 4 + 2] << 8;
	blocks[3] |= enc->state[enc->program].ptyn_text[enc->state[enc->program].ptyn_state * 4 + 3];

	enc->state[enc->program].ptyn_state++;
	if (enc->state[enc->program].ptyn_state == 2) enc->state[enc->program].ptyn_state = 0;
}

static void get_rds_lps_group(RDSEncoder* enc, uint16_t *blocks) {
	if (enc->state[enc->program].lps_state == 0 && enc->state[enc->program].lps_update) {
		memcpy(enc->state[enc->program].lps_text, enc->data[enc->program].lps, LPS_LENGTH);
		enc->state[enc->program].lps_update = 0;
	}

	blocks[1] |= 15 << 12 | enc->state[enc->program].lps_state;
	blocks[2] =  enc->state[enc->program].lps_text[enc->state[enc->program].lps_state * 4    ] << 8;
	blocks[2] |= enc->state[enc->program].lps_text[enc->state[enc->program].lps_state * 4 + 1];
	blocks[3] =  enc->state[enc->program].lps_text[enc->state[enc->program].lps_state * 4 + 2] << 8;
	blocks[3] |= enc->state[enc->program].lps_text[enc->state[enc->program].lps_state * 4 + 3];

	enc->state[enc->program].lps_state++;
	if (enc->state[enc->program].lps_state == enc->state[enc->program].lps_segments) enc->state[enc->program].lps_state = 0;
}

static void get_rds_ecc_group(RDSEncoder* enc, uint16_t *blocks) {
	blocks[1] |= 1 << 12;
	blocks[2] = enc->data[enc->program].ecc;

	if(enc->data[enc->program].pin[0]) {
		blocks[3] = enc->data[enc->program].pin[1] << 11;
		blocks[3] |= enc->data[enc->program].pin[2] << 6;
		blocks[3] |= enc->data[enc->program].pin[3];
	}
}

static void get_rds_lic_group(RDSEncoder* enc, uint16_t *blocks) {
	blocks[1] |= 1 << 12;
	blocks[2] = 0x3000 | enc->data[enc->program].lic;

	if(enc->data[enc->program].pin[0]) {
		blocks[3] = enc->data[enc->program].pin[1] << 11; // day
		blocks[3] |= enc->data[enc->program].pin[2] << 6; // hour
		blocks[3] |= enc->data[enc->program].pin[3]; // minute
	}
}
static void get_rds_rtplus_group(RDSEncoder* enc, uint16_t *blocks) {
	blocks[1] |= GET_GROUP_TYPE(enc->rtpData[enc->program].group) << 12;
	blocks[1] |= GET_GROUP_VER(enc->rtpData[enc->program].group) << 11;
	blocks[1] |= enc->rtpData[enc->program].toggle << 4 | enc->rtpData[enc->program].running << 3;
	blocks[1] |= (enc->rtpData[enc->program].type[0] & 0xf8) >> 3;

	blocks[2] =  (enc->rtpData[enc->program].type[0] & 0x07) << 13;
	blocks[2] |= (enc->rtpData[enc->program].start[0] & 0x3f) << 7;
	blocks[2] |= (enc->rtpData[enc->program].len[0] & 0x3f) << 1;
	blocks[2] |= (enc->rtpData[enc->program].type[1] & 0xe0) >> 5;

	blocks[3] =  (enc->rtpData[enc->program].type[1] & 0x1f) << 11;
	blocks[3] |= (enc->rtpData[enc->program].start[1] & 0x3f) << 5;
	blocks[3] |= enc->rtpData[enc->program].len[1] & 0x1f;
}
// #endregion

static uint8_t get_rds_custom_groups(RDSEncoder* enc, uint16_t *blocks) {
	if(enc->state[enc->program].custom_group[0] == 1) {
		enc->state[enc->program].custom_group[0] = 0;
		blocks[0] = enc->data[enc->program].pi;
		blocks[1] = enc->state[enc->program].custom_group[1];
		blocks[2] = enc->state[enc->program].custom_group[2];
		blocks[3] = enc->state[enc->program].custom_group[3];
		return 1;
	}
	return 0;
}

static void get_rds_group(RDSEncoder* enc, uint16_t *blocks) {
	blocks[0] = enc->data[enc->program].pi;
	blocks[1] = enc->data[enc->program].tp << 10;
	blocks[1] |= enc->data[enc->program].pty << 5;
	blocks[2] = 0;
	blocks[3] = 0;

	if (enc->data[enc->program].ct && get_rds_ct_group(enc, blocks)) {
		goto group_coded;
	}

	if(get_rds_custom_groups(enc, blocks)) {
		goto group_coded;
	}

	uint8_t good_group = 0;
	uint8_t cant_find_group = 0;
	char grp;

	while(good_group == 0) {
		uint8_t grp_sqc_idx = enc->state[enc->program].grp_seq_idx[0]++;
		if(enc->data[enc->program].grp_sqc[grp_sqc_idx] == '\0') {
			enc->state[enc->program].grp_seq_idx[0] = 0;
			grp_sqc_idx = 0;
		}
		grp = enc->data[enc->program].grp_sqc[grp_sqc_idx];

		if(grp == '0') good_group = 1;
		if(grp == '1' && enc->data[enc->program].ecclic_enabled) good_group = 1;
		if(grp == '2' && enc->data[enc->program].rt1_enabled) good_group = 1;
		if(grp == 'A' && enc->data[enc->program].ptyn_enabled) good_group = 1;
		if(grp == 'X' && enc->data[enc->program].udg1_len != 0) good_group = 1;
		if(grp == 'Y' && enc->data[enc->program].udg2_len != 0) good_group = 1;
		if(grp == 'R' && enc->rtpData[enc->program].enabled) good_group = 1;
		if(grp == '3' && enc->oda_state[enc->program].count != 0) good_group = 1;
		if(grp == 'F' && enc->data[enc->program].lps[0] != '\0') good_group = 1;

		if(!good_group) cant_find_group++;
		else cant_find_group = 0;
		if(!good_group && cant_find_group == 23) {
			cant_find_group = 0;
			get_rds_ps_group(enc, blocks);
			break;
		}
	}

	uint8_t udg_idx;
	switch (grp)
	{
		default:
		case '0':
			if(enc->state[enc->program].grp_seq_idx[1] != 3) enc->state[enc->program].grp_seq_idx[0]--;
			else enc->state[enc->program].grp_seq_idx[1] = 0;
			enc->state[enc->program].grp_seq_idx[1]++;
			get_rds_ps_group(enc, blocks);
			if(enc->data[enc->program].dps1_enabled || enc->data[enc->program].dps2_enabled) {
				switch (enc->state[enc->program].dynamic_ps_state)
				{
				case 0:
					enc->state[enc->program].static_ps_period++;
					break;
				case 1:
				case 2:
					enc->state[enc->program].dynamic_ps_period++;
					break;
				}
			}
			goto group_coded;
		case '1':
			if(enc->data[enc->program].ecc && enc->data[enc->program].lic) {
				if(enc->state[enc->program].ecc_or_lic == 0) {
					get_rds_ecc_group(enc, blocks);
				} else {
					get_rds_lic_group(enc, blocks);
				}
				enc->state[enc->program].ecc_or_lic ^= 1;
			} else if(enc->data[enc->program].lic) {
				get_rds_lic_group(enc, blocks);
			} else {
				get_rds_ecc_group(enc, blocks);
			}
			goto group_coded;
		case '2':
			get_rds_rt_group(enc, blocks);
			goto group_coded;
		case 'A':
			get_rds_ptyn_group(enc, blocks);
			goto group_coded;
		// TODO: Add EON
		case 'X':
			udg_idx = enc->state[enc->program].udg_idxs[0];
			for(int i = 0; i < 3; i++) blocks[i+1] = enc->data[enc->program].udg1[udg_idx][i];
			enc->state[enc->program].udg_idxs[0]++;
			if(enc->state[enc->program].udg_idxs[0] == enc->data[enc->program].udg1_len) enc->state[enc->program].udg_idxs[0] = 0;
			goto group_coded;
		case 'Y':
			udg_idx = enc->state[enc->program].udg_idxs[1];
			for(int i = 0; i < 3; i++) blocks[i+1] = enc->data[enc->program].udg2[udg_idx][i];
			enc->state[enc->program].udg_idxs[1]++;
			if(enc->state[enc->program].udg_idxs[1] == enc->data[enc->program].udg2_len) enc->state[enc->program].udg_idxs[1] = 0;
			goto group_coded;
		case 'R':
			if(enc->state[enc->program].rtp_oda == 0) {
				get_rds_rtplus_group(enc, blocks);
			} else {
				get_rds_oda_group(enc, blocks);
			}
			enc->state[enc->program].rtp_oda ^= 1;
			goto group_coded;
		// TODO: add uecp
		case '3':
			get_rds_oda_group(enc, blocks);
			goto group_coded;
		case 'F':
			get_rds_lps_group(enc, blocks);
			goto group_coded;
	}


group_coded:
	if (IS_TYPE_B(blocks)) {
		blocks[2] = enc->data[enc->program].pi;
	}
}

void get_rds_bits(RDSEncoder* enc, uint8_t *bits) {
	static uint16_t out_blocks[GROUP_LENGTH];
	get_rds_group(enc, out_blocks);
	add_checkwords(out_blocks, bits);
}

static void init_rtplus(RDSEncoder* enc, uint8_t group, uint8_t program) {
	register_oda(enc, group, ODA_AID_RTPLUS, 0);
	enc->rtpData[program].group = group;
	enc->rtpData[program].enabled = 0;
}

void set_rds_defaults(RDSEncoder* enc, uint8_t program) {
	memset(&(enc->data[program]), 0, sizeof(RDSData));
	memset(&(enc->state[program]), 0, sizeof(RDSState));
	memset(&(enc->oda_state[program]), 0, sizeof(RDSODAState));
	memset(&(enc->odas[program]), 0, sizeof(RDSODA)*MAX_ODAS);
	memset(&(enc->rtpData[program]), 0, sizeof(RDSRTPlusData));
	memset(&(enc->encoder_data[program]), 0, sizeof(RDSEncoderData));

	enc->encoder_data[program].encoder_addr[0] = 255;
	enc->encoder_data[program].encoder_addr[1] = 255;

	enc->data[program].ct = 1;
	enc->data[program].di = 1;
	enc->data[program].ecclic_enabled = 1;
	strcpy((char *)enc->data[program].grp_sqc, DEFAULT_GRPSQC);
	enc->data[program].ms = 1;
	enc->data[program].tp = 1;
	enc->data[program].pi = 0xFFFF;
	strcpy((char *)enc->data[program].ps, "* RDS * ");
	enc->data[program].rt1_enabled = 1;

	memset(enc->data[program].rt1, ' ', 64);
	enc->data[program].rt1[0] = '\r';

	enc->data[program].static_ps_period = 16;
	enc->data[program].dps_label_period = 8;

	enc->state[program].rt_ab = 1;
	enc->state[program].ptyn_ab = 1;

	enc->state[program].rt_update = 1;
	enc->state[program].ps_update = 1;
	enc->state[program].tps_update = 1;
	enc->state[program].ptyn_update = 1;
	enc->state[program].lps_update = 1;

	init_rtplus(enc, GROUP_11A, program);
}

void init_rds_encoder(RDSEncoder* enc) {
	for(int i = 0; i < PROGRAMS; i++) {
		set_rds_defaults(enc, i);
	}

	if (rdssaved()) {
		loadFromFile(enc);
	} else {
		saveToFile(enc, "ALL");
	}
}

void set_rds_rt1(RDSEncoder* enc, char *rt1) {
	uint8_t i = 0, len = 0;

	enc->state[enc->program].rt_update = 1;

	memset(enc->data[enc->program].rt1, ' ', RT_LENGTH);
	while (*rt1 != 0 && len < RT_LENGTH) enc->data[enc->program].rt1[len++] = *rt1++;

	if (len < RT_LENGTH && enc->data[enc->program].shortrt) {
		enc->state[enc->program].rt_segments = 0;

		enc->data[enc->program].rt1[len++] = '\r';

		while (i < len) {
			i += 4;
			enc->state[enc->program].rt_segments++;
		}
	} else {
		enc->state[enc->program].rt_segments = 16;
	}
}

void set_rds_ps(RDSEncoder* enc, char *ps) {
	uint8_t len = 0;

	enc->state[enc->program].ps_update = 1;
	memset(enc->data[enc->program].ps, ' ', PS_LENGTH);
	while (*ps != 0 && len < PS_LENGTH) enc->data[enc->program].ps[len++] = *ps++;
}
void set_rds_dps1(RDSEncoder* enc, char *dps1) {
	uint8_t len = 0;

	memset(enc->data[enc->program].dps1, ' ', RT_LENGTH);
	while (*dps1 != 0 && len < RT_LENGTH) enc->data[enc->program].rt1[len++] = *dps1++;
	enc->data[enc->program].dps1_len = len;
}

void set_rds_tps(RDSEncoder* enc, char *tps) {
	uint8_t len = 0;

	enc->state[enc->program].tps_update = 1;
	if(tps[0] == '\0') {
		memset(enc->data[enc->program].tps, 0, PS_LENGTH);
		return;
	}

	memset(enc->data[enc->program].tps, ' ', PS_LENGTH);
	while (*tps != 0 && len < PS_LENGTH) enc->data[enc->program].tps[len++] = *tps++;
}

void set_rds_lps(RDSEncoder* enc, char *lps) {
	uint8_t i = 0, len = 0;

	enc->state[enc->program].lps_update = 1;
	if(lps[0] == '\0') {
		memset(enc->data[enc->program].lps, 0, LPS_LENGTH);
		return;
	}
	memset(enc->data[enc->program].lps, '\r', LPS_LENGTH);
	while (*lps != 0 && len < LPS_LENGTH) enc->data[enc->program].lps[len++] = *lps++;

	if (len < LPS_LENGTH) {
		enc->state[enc->program].lps_segments = 0;

		len++;

		while (i < len) {
			i += 4;
			enc->state[enc->program].lps_segments++;
		}
	} else {
		enc->state[enc->program].lps_segments = 8;
	}
}

void set_rds_rtplus_flags(RDSEncoder* enc, uint8_t flags) {
	enc->rtpData[enc->program].enabled = (flags==2);
	enc->rtpData[enc->program].running	= flags & 1;
}

void set_rds_rtplus_tags(RDSEncoder* enc, uint8_t *tags) {
	enc->rtpData[enc->program].type[0]	= tags[0] & 0x3f;
	enc->rtpData[enc->program].start[0]	= tags[1] & 0x3f;
	enc->rtpData[enc->program].len[0]	= tags[2] & 0x3f;
	enc->rtpData[enc->program].type[1]	= tags[3] & 0x3f;
	enc->rtpData[enc->program].start[1]	= tags[4] & 0x3f;
	enc->rtpData[enc->program].len[1]	= tags[5] & 0x1f;

	enc->rtpData[enc->program].toggle ^= 1;
	enc->rtpData[enc->program].running = 1;
	enc->rtpData[enc->program].enabled = 1;
}

void set_rds_ptyn(RDSEncoder* enc, char *ptyn) {
	uint8_t len = 0;

	enc->state[enc->program].ptyn_update = 1;

	if(ptyn[0] == '\0') {
		memset(enc->data[enc->program].ptyn, 0, PTYN_LENGTH);
		return;
	}

	memset(enc->data[enc->program].ptyn, ' ', PTYN_LENGTH);
	while (*ptyn != 0 && len < PTYN_LENGTH) enc->data[enc->program].ptyn[len++] = *ptyn++;
}