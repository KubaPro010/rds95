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
		memcpy(tempEncoder.data[emp->program].default_rt, emp->data[emp->program].default_rt, RT_LENGTH);
	} else if (strcmp(option, "RT2") == 0) {
		memcpy(tempEncoder.data[emp->program].rt2, emp->data[emp->program].rt2, RT_LENGTH);
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
		memcpy(tempEncoder.data[emp->program].dps1, emp->data[emp->program].dps1, DPS_LENGTH);
		tempEncoder.data[emp->program].dps1_enabled = emp->data[emp->program].dps1_enabled;
		tempEncoder.data[emp->program].dps1_numberofrepeats = emp->data[emp->program].dps1_numberofrepeats;
	} else if (strcmp(option, "DPS1EN") == 0) {
		tempEncoder.data[emp->program].dps1_enabled = emp->data[emp->program].dps1_enabled;
	} else if (strcmp(option, "DPS1MOD") == 0) {
		tempEncoder.data[emp->program].dps1_mode = emp->data[emp->program].dps1_mode;
	} else if (strcmp(option, "LABPER") == 0) {
		tempEncoder.data[emp->program].dps_label_period = emp->data[emp->program].dps_label_period;
	} else if (strcmp(option, "SPSPER") == 0) {
		tempEncoder.data[emp->program].static_ps_period = emp->data[emp->program].static_ps_period;
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
	} else if(strcmp(option, "RT2EN") == 0) {
		tempEncoder.data[emp->program].rt2_enabled = emp->data[emp->program].rt2_enabled;
	} else if(strcmp(option, "RTPER") == 0) {
		tempEncoder.data[emp->program].rt_switching_period = emp->data[emp->program].rt_switching_period;
		tempEncoder.data[emp->program].orignal_rt_switching_period = emp->data[emp->program].orignal_rt_switching_period;
	} else if(strcmp(option, "PINEN") == 0) {
		tempEncoder.data[emp->program].pin[0] = emp->data[emp->program].pin[0];
	} else if(strcmp(option, "PROGRAM") == 0) {
		tempEncoder.program = emp->program;
	} else if(strcmp(option, "EON") == 0) {
		memcpy(&(tempEncoder.data[emp->program].eon), &(emp->data[emp->program].eon), sizeof(RDSEON)*EONS);
	} else if (strcmp(option, "ALL") == 0) {
		memcpy(&(tempEncoder.data[emp->program]), &(emp->data[emp->program]), sizeof(RDSData));
		memcpy(&(tempEncoder.rtpData[emp->program]), &(emp->rtpData[emp->program]), sizeof(RDSRTPlusData));
		memcpy(&(tempEncoder.odas[emp->program]), &(emp->odas[emp->program]), sizeof(RDSODA)*MAX_ODAS);
		memcpy(&(tempEncoder.oda_state[emp->program]), &(emp->oda_state[emp->program]), sizeof(RDSODAState));
		memcpy(&(tempEncoder.encoder_data), &(emp->encoder_data), sizeof(RDSEncoderData));
		tempEncoder.program = emp->program;
	}

	RDSEncoderFile rdsEncoderfile;
	rdsEncoderfile.file_starter = 225;
	rdsEncoderfile.file_middle = 160;
	rdsEncoderfile.file_ender = 95;
	memcpy(&(rdsEncoderfile.data[emp->program]), &(tempEncoder.data[emp->program]), sizeof(RDSData));
	memcpy(&(rdsEncoderfile.rtpData[emp->program]), &(tempEncoder.rtpData[emp->program]), sizeof(RDSRTPlusData));
	memcpy(&(rdsEncoderfile.odas[emp->program]), &(tempEncoder.odas[emp->program]), sizeof(RDSODA)*MAX_ODAS);
	memcpy(&(rdsEncoderfile.oda_state[emp->program]), &(tempEncoder.oda_state[emp->program]), sizeof(RDSODAState));
	memcpy(&(rdsEncoderfile.encoder_data), &(tempEncoder.encoder_data), sizeof(RDSEncoderData));
	rdsEncoderfile.program = tempEncoder.program;

	file = fopen(encoderPath, "wb");
	if (file == NULL) {
		perror("Error opening file");
		return;
	}
	fwrite(&rdsEncoderfile, sizeof(RDSEncoderFile), 1, file);
	fclose(file);
}

void loadFromFile(RDSEncoder *enc) {
	RDSEncoderFile rdsEncoderfile;

	char encoderPath[256];
	snprintf(encoderPath, sizeof(encoderPath), "%s/.rdsEncoder", getenv("HOME"));
	FILE *file = fopen(encoderPath, "rb");
	if (file == NULL) {
		perror("Error opening file");
		return;
	}
	fread(&rdsEncoderfile, sizeof(RDSEncoderFile), 1, file);
	fclose(file);

	if (rdsEncoderfile.file_starter != 225 || rdsEncoderfile.file_ender != 95 || rdsEncoderfile.file_middle != 160) {
		fprintf(stderr, "Invalid file format\n");
		return;
	}

	for (int i = 0; i < PROGRAMS; i++) {
		memcpy(&(enc->data[i]), &(rdsEncoderfile.data[i]), sizeof(RDSData));
		memcpy(&(enc->rtpData[i]), &(rdsEncoderfile.rtpData[i]), sizeof(RDSRTPlusData));
		memcpy(&(enc->odas[i]), &(rdsEncoderfile.odas[i]), sizeof(RDSODA) * MAX_ODAS);
		memcpy(&(enc->oda_state[i]), &(rdsEncoderfile.oda_state[i]), sizeof(RDSODAState));
	}
	memcpy(&(enc->encoder_data), &(rdsEncoderfile.encoder_data), sizeof(RDSODAState));
	enc->program = rdsEncoderfile.program;
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

// static void register_oda(RDSEncoder* enc, uint8_t group, uint16_t aid, uint16_t scb) {
// 	if (enc->oda_state[enc->program].count >= MAX_ODAS) return;

// 	enc->odas[enc->program][enc->oda_state[enc->program].count].group = group;
// 	enc->odas[enc->program][enc->oda_state[enc->program].count].aid = aid;
// 	enc->odas[enc->program][enc->oda_state[enc->program].count].scb = scb;
// 	enc->oda_state[enc->program].count++;
// }

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
	uint8_t dps1_on = (enc->data[enc->program].dps1_enabled && enc->state[enc->program].dps1_len != 0);
get_ps:
	if(enc->state[enc->program].ps_csegment == 0) {
		if(enc->state[enc->program].ps_update && !dps1_on) {
			memcpy(enc->state[enc->program].ps_text, enc->data[enc->program].ps, PS_LENGTH);
			enc->state[enc->program].ps_update = 0;
		}

		if(enc->state[enc->program].tps_update && !dps1_on) {
			memcpy(enc->state[enc->program].tps_text, enc->data[enc->program].tps, PS_LENGTH);
			enc->state[enc->program].tps_update = 0;
		}

		if(enc->state[enc->program].dps1_update && dps1_on) {
			memcpy(enc->state[enc->program].dps1_text, enc->data[enc->program].dps1, DPS_LENGTH);
			enc->state[enc->program].dps1_update = 0;
			enc->state[enc->program].dps1_repeat_count = 0;
		}

		if(dps1_on) {
			char ps_text[PS_LENGTH];
			strncpy(ps_text, (enc->data[enc->program].ta) ? enc->data[enc->program].tps : enc->data[enc->program].ps, PS_LENGTH);
			ps_text[PS_LENGTH - 1] = '\0';

			if(enc->state[enc->program].dynamic_ps_state == 0) {
				memcpy(enc->state[enc->program].ps_text, ps_text, PS_LENGTH);

				if(enc->state[enc->program].static_ps_period >= enc->data[enc->program].static_ps_period) {
					enc->state[enc->program].dynamic_ps_state = 1;
					enc->state[enc->program].static_ps_period = 0;
					enc->state[enc->program].dynamic_ps_position = 0;
				}
			} else if(enc->state[enc->program].dynamic_ps_state == 1) {
				uint8_t scroll_threshold = (enc->data[enc->program].dps_speed == 0) ? 4 : 2;
				if(enc->data[enc->program].dps1_mode == 0) scroll_threshold = enc->data[enc->program].dps_label_period;

				if(enc->state[enc->program].dynamic_ps_scroll_counter >= scroll_threshold) {
					if(enc->state[enc->program].dynamic_ps_position >= enc->state[enc->program].dps1_len) {
						enc->state[enc->program].dynamic_ps_position = 0;
						enc->state[enc->program].dps1_repeat_count++;

						if(enc->state[enc->program].dps1_repeat_count >= enc->data[enc->program].dps1_numberofrepeats) {
							if(enc->state[enc->program].dps1_nexttext_len != 0 && enc->state[enc->program].dps1_len < 128 && enc->state[enc->program].dps1_nexttext_update) {
								enc->state[enc->program].dps1_nexttext_update = 0;
								enc->state[enc->program].dynamic_ps_state = 1;
								enc->state[enc->program].dynamic_ps_period = 0;
								enc->state[enc->program].dps1_repeat_count = 0;
								enc->state[enc->program].dynamic_ps_scroll_counter = 0;
								enc->state[enc->program].static_ps_period = 0;
								memcpy(enc->state[enc->program].dps1_text, enc->state[enc->program].dps1_nexttext, PS_LENGTH);
								enc->state[enc->program].dps1_len = enc->state[enc->program].dps1_nexttext_len;
								goto get_ps;
							} else {
								enc->state[enc->program].dynamic_ps_state = 0;
								enc->state[enc->program].dynamic_ps_period = 0;
								enc->state[enc->program].dps1_repeat_count = 0;
								enc->state[enc->program].dynamic_ps_scroll_counter = 0;
								enc->state[enc->program].static_ps_period = 0;
								memcpy(enc->state[enc->program].ps_text, ps_text, PS_LENGTH);
								goto encode;
							}
						}
					}

					switch(enc->data[enc->program].dps1_mode) {
						case 0:
							memcpy(enc->state[enc->program].ps_text, &(enc->state[enc->program].dps1_text[enc->state[enc->program].dynamic_ps_position]), PS_LENGTH);
							enc->state[enc->program].dynamic_ps_position += PS_LENGTH;
							break;
						case 1:
							memcpy(enc->state[enc->program].ps_text, &(enc->state[enc->program].dps1_text[enc->state[enc->program].dynamic_ps_position]), PS_LENGTH);
							enc->state[enc->program].dynamic_ps_position++;
							break;
					}
					enc->state[enc->program].dynamic_ps_scroll_counter = 0;
				} else {
					enc->state[enc->program].dynamic_ps_scroll_counter++;
				}
			}
		}
	}

encode:
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

static void get_rds_rt_group(RDSEncoder* enc, uint16_t *blocks) {
	if (enc->state[enc->program].rt_update && enc->data[enc->program].rt1_enabled && !enc->state[enc->program].current_rt) {
		memcpy(enc->state[enc->program].rt_text, enc->data[enc->program].rt1, RT_LENGTH);
		enc->state[enc->program].rt_ab ^= 1;
		enc->state[enc->program].rt_update = 0;
		enc->state[enc->program].rt_state = 0;
		enc->state[enc->program].current_rt = 0;
	}
	if(enc->state[enc->program].rt2_update && enc->data[enc->program].rt2_enabled && enc->state[enc->program].current_rt) {
		memcpy(enc->state[enc->program].rt_text, enc->data[enc->program].rt2, RT_LENGTH);
		enc->state[enc->program].rt_ab ^= 1;
		enc->state[enc->program].rt2_update = 0;
		enc->state[enc->program].rt_state = 0;
		enc->state[enc->program].current_rt = 1;
	}

	uint8_t ab = enc->state[enc->program].rt_ab;
	switch (enc->data[enc->program].rt_type)
	{
	case 0:
		ab = 0;
		break;
	case 1:
		ab = (enc->state[enc->program].current_rt == 0) ? 0 : 1;
		break;
	case 2:
	default:
		break;
	}

	blocks[1] |= 2 << 12;
	blocks[1] |= ab << 4;
	blocks[1] |= enc->state[enc->program].rt_state;
	blocks[2] =  enc->state[enc->program].rt_text[enc->state[enc->program].rt_state * 4    ] << 8;
	blocks[2] |= enc->state[enc->program].rt_text[enc->state[enc->program].rt_state * 4 + 1];
	blocks[3] =  enc->state[enc->program].rt_text[enc->state[enc->program].rt_state * 4 + 2] << 8;
	blocks[3] |= enc->state[enc->program].rt_text[enc->state[enc->program].rt_state * 4 + 3];

	enc->state[enc->program].rt_state++;
	uint8_t segments = (enc->state[enc->program].current_rt == 1) ? enc->state[enc->program].rt2_segments : enc->state[enc->program].rt_segments;
	if (enc->state[enc->program].rt_state >= segments) enc->state[enc->program].rt_state = 0;
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

static void get_rds_rtp_oda_group(RDSEncoder* enc, uint16_t *blocks) {
	blocks[1] |= 3 << 12;

	blocks[1] |= GET_GROUP_TYPE(enc->rtpData[enc->program].group) << 1;
	blocks[1] |= GET_GROUP_VER(enc->rtpData[enc->program].group);
	blocks[3] = ODA_AID_RTPLUS;
}

static void get_rds_ct_group(RDSEncoder* enc, uint16_t *blocks) {
	(void)enc;
	struct tm *utc, *local_time;
	time_t now;
	uint8_t l;
	uint32_t mjd;
	int16_t offset;

	now = time(NULL);
	utc = gmtime(&now);

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
	blocks[1] |= enc->rtpState[enc->program].toggle << 4 | enc->rtpData[enc->program].running << 3;
	blocks[1] |= (enc->rtpData[enc->program].type[0] & 0xf8) >> 3;

	blocks[2] =  (enc->rtpData[enc->program].type[0] & 0x07) << 13;
	blocks[2] |= (enc->rtpData[enc->program].start[0] & 0x3f) << 7;
	blocks[2] |= (enc->rtpData[enc->program].len[0] & 0x3f) << 1;
	blocks[2] |= (enc->rtpData[enc->program].type[1] & 0xe0) >> 5;

	blocks[3] =  (enc->rtpData[enc->program].type[1] & 0x1f) << 11;
	blocks[3] |= (enc->rtpData[enc->program].start[1] & 0x3f) << 5;
	blocks[3] |= enc->rtpData[enc->program].len[1] & 0x1f;
}

static void get_rds_eon_group(RDSEncoder* enc, uint16_t *blocks) {
	RDSEON eon;
get_eon:
	eon = enc->data[enc->program].eon[enc->state[enc->program].eon_index];
	blocks[1] |= 14 << 12;
	blocks[1] |= eon.tp << 4;

	switch (enc->state[enc->program].eon_state)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		blocks[2] = eon.ps[enc->state[enc->program].eon_state*2] << 8;
		blocks[2] |= eon.ps[enc->state[enc->program].eon_state*2 + 1];
		blocks[1] |= enc->state[enc->program].eon_state;
		break;
	case 4: // 13
		if(eon.pty == 0 && eon.tp == 0) {
			enc->state[enc->program].eon_state++;
			goto get_eon;
		}
		blocks[2] = eon.pty << 11;
		if(eon.tp) blocks[2] |= eon.ta;
		blocks[1] |= 13;
		break;
	case 5: // 14
		if(eon.pin[0] == 0) {
			break;
		}
		blocks[2] = eon.pin[1] << 11;
		blocks[2] |= eon.pin[2] << 6;
		blocks[2] |= eon.pin[3];
		blocks[1] |= 14;
		break;
	// TODO: Add AF
	}

	blocks[3] = eon.pi;

	if(enc->state[enc->program].eon_state == 5) {
		enc->state[enc->program].eon_index++;

		uint8_t i = 0;
		while(i < EONS && !enc->data[enc->program].eon[enc->state[enc->program].eon_index].enabled) {
			enc->state[enc->program].eon_index++;
			if(enc->state[enc->program].eon_index >= EONS) {
				enc->state[enc->program].eon_index = 0;
			}
			i++;
		}

		enc->state[enc->program].eon_state = 0;
		goto get_eon;
	} else {
		enc->state[enc->program].eon_state++;
	}
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

static void get_rds_group(RDSEncoder* enc, uint16_t *blocks, uint8_t stream) {
	blocks[0] = enc->data[enc->program].pi;
	if(stream != 0) blocks[0] = 0;
	blocks[1] = 0;
	blocks[2] = 0;
	blocks[3] = 0;

	struct tm *utc;
	time_t now;
	time(&now);
	utc = gmtime(&now);

	if (utc->tm_min != enc->state[enc->program].last_minute) {
		enc->state[enc->program].last_minute = utc->tm_min;
		uint8_t eon_has_ta = 0;
		for (int i = 0; i < EONS; i++) {
			if (enc->data[enc->program].eon[i].enabled && enc->data[enc->program].eon[i].ta) {
				eon_has_ta = 1;
				break;
			}
		}
		if(enc->data[enc->program].tp && enc->data[enc->program].ta && enc->state[enc->program].ta_timeout && !eon_has_ta) {
			enc->state[enc->program].ta_timeout--;
			if(enc->state[enc->program].ta_timeout == 0) {
				enc->data[enc->program].ta = 0;
				enc->state[enc->program].ta_timeout = enc->state[enc->program].original_ta_timeout;
			};
		}

		if(enc->data[enc->program].rt1_enabled && enc->data[enc->program].rt2_enabled && enc->data[enc->program].rt_switching_period != 0) {
			enc->data[enc->program].rt_switching_period--;
			if(enc->data[enc->program].rt_switching_period == 0) {
				enc->state[enc->program].current_rt ^= 1;
				if (enc->state[enc->program].current_rt == 1) {
					memcpy(enc->state[enc->program].rt_text, enc->data[enc->program].rt2, RT_LENGTH);
				} else {
					memcpy(enc->state[enc->program].rt_text, enc->data[enc->program].rt1, RT_LENGTH);
				}
				enc->state[enc->program].rt_state = 0;
				enc->data[enc->program].rt_switching_period = enc->data[enc->program].orignal_rt_switching_period;
			}
		}

		if(enc->data[enc->program].ct) {
			get_rds_ct_group(enc, blocks);
			goto group_coded;
		}
	}

	if(get_rds_custom_groups(enc, blocks)) {
		goto group_coded;
	}

	uint8_t good_group = 0;
	uint8_t cant_find_group = 0;
	char grp;

	while(good_group == 0) {
		uint8_t grp_sqc_idx = enc->state[enc->program].grp_seq_idx[0];
		if(enc->data[enc->program].grp_sqc[grp_sqc_idx] == '\0') {
			enc->state[enc->program].grp_seq_idx[0] = 0;
			grp_sqc_idx = 0;
		}
		grp = enc->data[enc->program].grp_sqc[grp_sqc_idx];

		if(grp == '0') good_group = 1;
		if(grp == '1' && enc->data[enc->program].ecclic_enabled) good_group = 1;
		if(grp == '2' && (enc->data[enc->program].rt1_enabled || enc->data[enc->program].rt2_enabled)) good_group = 1;
		if(grp == 'A' && enc->data[enc->program].ptyn_enabled) good_group = 1;
		if(grp == 'E') {
			for (int i = 0; i < EONS; i++) {
				if (enc->data[enc->program].eon[i].enabled) {
					good_group = 1;
					break;
				}
			}
		}
		if(grp == 'X' && enc->data[enc->program].udg1_len != 0) good_group = 1;
		if(grp == 'Y' && enc->data[enc->program].udg2_len != 0) good_group = 1;
		if(grp == 'R' && enc->rtpData[enc->program].enabled) good_group = 1;
		if(grp == '3' && enc->oda_state[enc->program].count != 0) good_group = 1;
		if(grp == 'F' && enc->data[enc->program].lps[0] != '\0') good_group = 1;

		if(!good_group) cant_find_group++;
		else cant_find_group = 0;
		if(!good_group && cant_find_group == 23) {
			cant_find_group = 0;
			break;
		}

		enc->state[enc->program].grp_seq_idx[0]++;
	}
	if(!good_group) grp = '0';

	uint8_t udg_idx;
	switch (grp)
	{
		default:
		case '0':
			if(enc->state[enc->program].grp_seq_idx[1] < 4) enc->state[enc->program].grp_seq_idx[0]--;
			else {
				enc->state[enc->program].grp_seq_idx[1] = 0;

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
			}

			enc->state[enc->program].grp_seq_idx[1]++;
			get_rds_ps_group(enc, blocks);
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
		case 'E':
			get_rds_eon_group(enc, blocks);
			goto group_coded;
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
				get_rds_rtp_oda_group(enc, blocks);
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
	blocks[1] |= enc->data[enc->program].tp << 10;
	blocks[1] |= enc->data[enc->program].pty << 5;
	if (IS_TYPE_B(blocks)) {
		blocks[2] = enc->data[enc->program].pi;
	}
}

void get_rds_bits(RDSEncoder* enc, uint8_t *bits, uint8_t stream) {
	static uint16_t out_blocks[GROUP_LENGTH];
	get_rds_group(enc, out_blocks, stream);
	add_checkwords(out_blocks, bits);
}

static void init_rtplus(RDSEncoder* enc, uint8_t group, uint8_t program) {
	enc->rtpData[program].group = group;
	enc->rtpData[program].enabled = 0;
}

void reset_rds_state(RDSEncoder* enc, uint8_t program) {
	RDSEncoder tempCoder;
	tempCoder.program = program;
	memset(&(tempCoder.state[program]), 0, sizeof(RDSState));
	memset(&(tempCoder.rtpState[program]), 0, sizeof(RDSRTPlusState));

	tempCoder.state[program].rt_ab = 1;
	tempCoder.state[program].ptyn_ab = 1;
	set_rds_rt1(&tempCoder, enc->data[program].rt1);
	set_rds_rt2(&tempCoder, enc->data[program].rt2);
	set_rds_ps(&tempCoder, enc->data[program].ps);
	set_rds_dps1(&tempCoder, enc->data[program].dps1);
	set_rds_tps(&tempCoder, enc->data[program].tps);
	set_rds_ptyn(&tempCoder, enc->data[program].ptyn);
	set_rds_lps(&tempCoder, enc->data[program].lps);
	set_rds_grpseq(&tempCoder, enc->data[program].grp_sqc);

	struct tm *utc;
	time_t now;
	time(&now);
	utc = gmtime(&now);
	tempCoder.state[program].last_minute = utc->tm_min;

	tempCoder.rtpState[program].toggle = 0;

	for(int i = 0; i < EONS; i++) {
		tempCoder.data[program].eon[i].ta = 0;
	}

	memcpy(&(enc->state[program]), &(tempCoder.state[program]), sizeof(RDSState));
	memcpy(&(enc->rtpState[program]), &(tempCoder.rtpState[program]), sizeof(RDSRTPlusState));
}

void set_rds_defaults(RDSEncoder* enc, uint8_t program) {
	memset(&(enc->data[program]), 0, sizeof(RDSData));
	memset(&(enc->oda_state[program]), 0, sizeof(RDSODAState));
	memset(&(enc->odas[program]), 0, sizeof(RDSODA)*MAX_ODAS);
	memset(&(enc->rtpData[program]), 0, sizeof(RDSRTPlusData));
	memset(&(enc->encoder_data), 0, sizeof(RDSEncoderData));

	enc->encoder_data.encoder_addr[0] = 255;
	enc->encoder_data.encoder_addr[1] = 255;

	enc->data[program].ct = 1;
	enc->data[program].di = 1;
	enc->data[program].ecclic_enabled = 1;
	strcpy((char *)enc->data[program].grp_sqc, DEFAULT_GRPSQC);
	enc->data[program].ms = 1;
	enc->data[program].tp = 1;
	enc->data[program].pi = 0xFFFF;
	strcpy((char *)enc->data[program].ps, "* RDS * ");
	enc->data[program].rt1_enabled = 1;

	memset(enc->data[program].rt1, ' ', 59);

	enc->data[program].static_ps_period = 10;
	enc->data[program].dps_label_period = 8;
	enc->data[program].dps1_numberofrepeats = 1;

	enc->data[program].rt_type = 2;

	reset_rds_state(enc, program);

	init_rtplus(enc, GROUP_11A, program);

	enc->encoder_data.ascii_data.expected_encoder_addr = 255;
	enc->encoder_data.ascii_data.expected_site_addr = 255;
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

	for(int i = 0; i < PROGRAMS; i++) {
		reset_rds_state(enc, i);
	}
}

void set_rds_rt1(RDSEncoder* enc, char *rt1) {
	uint8_t i = 0, len = 0;

	enc->state[enc->program].rt_update = 1;

	memset(enc->data[enc->program].rt1, ' ', RT_LENGTH);
	while (*rt1 != 0 && len < RT_LENGTH) enc->data[enc->program].rt1[len++] = *rt1++;

	while (len > 0 && enc->data[enc->program].rt1[len - 1] == ' ' && enc->data[enc->program].shortrt) {
        len--;
    }

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

void set_rds_rt2(RDSEncoder* enc, char *rt2) {
	uint8_t i = 0, len = 0;

	enc->state[enc->program].rt2_update = 1;

	memset(enc->data[enc->program].rt2, ' ', RT_LENGTH);
	while (*rt2 != 0 && len < RT_LENGTH) enc->data[enc->program].rt2[len++] = *rt2++;

	while (len > 0 && enc->data[enc->program].rt2[len - 1] == ' ' && enc->data[enc->program].shortrt) {
		len--;
	}

	if (len < RT_LENGTH && enc->data[enc->program].shortrt) {
		enc->state[enc->program].rt2_segments = 0;

		enc->data[enc->program].rt2[len++] = '\r';

		while (i < len) {
			i += 4;
			enc->state[enc->program].rt2_segments++;
		}
	} else {
		enc->state[enc->program].rt2_segments = 16;
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

	enc->state[enc->program].dps1_update = 1;
	memset(enc->data[enc->program].dps1, ' ', DPS_LENGTH);
	while (*dps1 != 0 && len < DPS_LENGTH) enc->data[enc->program].dps1[len++] = *dps1++;
	enc->state[enc->program].dps1_len = len;

	enc->state[enc->program].dynamic_ps_position = 0;
	enc->state[enc->program].dynamic_ps_period = 0;
	enc->state[enc->program].static_ps_period = 0;
	enc->state[enc->program].dynamic_ps_scroll_counter = 0;
}

void set_rds_next_dps1(RDSEncoder* enc, char *dps1) {
	uint8_t len = 0;

	enc->state[enc->program].dps1_nexttext_update = 1;
	memset(enc->state[enc->program].dps1_nexttext, ' ', 127);
	while (*dps1 != 0 && len < 127) enc->state[enc->program].dps1_nexttext[len++] = *dps1++;
	enc->state[enc->program].dps1_nexttext_len = len;
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

	enc->rtpState[enc->program].toggle ^= 1;
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

void set_rds_grpseq(RDSEncoder* enc, char *grpseq) {
	uint8_t len = 0;

	if(grpseq[0] == '\0') {
		while (DEFAULT_GRPSQC[len] != 0 && len < 24) {
			enc->data[enc->program].grp_sqc[len] = DEFAULT_GRPSQC[len];
			len++;
		}
		return;
	}

	memset(enc->data[enc->program].grp_sqc, 0, 24);
	while (*grpseq != 0 && len < 24) enc->data[enc->program].grp_sqc[len++] = *grpseq++;
}