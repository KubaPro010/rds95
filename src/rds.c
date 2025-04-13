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

	if(strcmp(option, "PROGRAM") == 0) {
		memcpy(&(tempEncoder.data[emp->program]), &(emp->data[emp->program]), sizeof(RDSData));
		memcpy(&(tempEncoder.rtpData[emp->program]), &(emp->rtpData[emp->program]), sizeof(RDSRTPlusData)*2);
		tempEncoder.program = emp->program;
	} else if (strcmp(option, "ALL") == 0) {
		memcpy(&tempEncoder.data, &emp->data, sizeof(RDSData)*PROGRAMS);
		memcpy(&tempEncoder.rtpData, &emp->rtpData, sizeof(RDSRTPlusData)*PROGRAMS*2);
		memcpy(&tempEncoder.encoder_data, &emp->encoder_data, sizeof(RDSEncoderData));
		tempEncoder.program = emp->program;
	} else {
		return;
	}

	RDSEncoderFile rdsEncoderfile;
	rdsEncoderfile.file_starter = 225;
	rdsEncoderfile.file_middle = 160;
	rdsEncoderfile.file_ender = 95;
	memcpy(&(rdsEncoderfile.data[emp->program]), &(tempEncoder.data[emp->program]), sizeof(RDSData));
	memcpy(&(rdsEncoderfile.rtpData[emp->program]), &(tempEncoder.rtpData[emp->program]), sizeof(RDSRTPlusData)*2);
	memcpy(&(rdsEncoderfile.encoder_data), &(tempEncoder.encoder_data), sizeof(RDSEncoderData));
	rdsEncoderfile.program = tempEncoder.program;
	rdsEncoderfile.crc = crc16_ccitt((char*)&rdsEncoderfile, sizeof(RDSEncoderFile) - sizeof(uint16_t));

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
		fprintf(stderr, "[RDSENCODER-FILE] Invalid file format\n");
		return;
	}

    uint16_t calculated_crc = crc16_ccitt((char*)&rdsEncoderfile, sizeof(RDSEncoderFile) - sizeof(uint16_t));

    if (calculated_crc != rdsEncoderfile.crc) {
        fprintf(stderr, "[RDSENCODER-FILE] CRC mismatch! Data may be corrupted\n");
        return;
    }

	for (int i = 0; i < PROGRAMS; i++) {
		memcpy(&(enc->data[i]), &(rdsEncoderfile.data[i]), sizeof(RDSData));
		memcpy(&(enc->rtpData[i]), &(rdsEncoderfile.rtpData[i]), sizeof(RDSRTPlusData)*2);
	}
	memcpy(&(enc->encoder_data), &(rdsEncoderfile.encoder_data), sizeof(RDSEncoderData));
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

static uint16_t get_next_af(RDSEncoder* enc) {
	uint16_t out;

	if (enc->data[enc->program].af.num_afs) {
		uint8_t* afs = enc->data[enc->program].af.afs;
		uint8_t num_afs = enc->data[enc->program].af.num_afs;
		uint8_t* af_state = &enc->state[enc->program].af_state;

		if (*af_state < num_afs) {
			out = afs[*af_state];
			(*af_state)++;
		} else {
			out = AF_CODE_FILLER;
		}

		if (*af_state >= enc->data[enc->program].af.num_entries) {
			*af_state = 0;
		}
	} else {
		out = AF_CODE_NUM_AFS_BASE << 8 | AF_CODE_FILLER;
	}

	return out;
}

static void get_next_af_oda(RDSEncoder* enc, uint16_t* af_group) {
	uint16_t* afs = enc->data[enc->program].af_oda.afs;
	uint16_t num_afs = enc->data[enc->program].af_oda.num_afs;
	uint16_t* af_state = &enc->state[enc->program].af_oda_state;

	for (int i = 0; i < 4; ++i) {
		if (*af_state < num_afs) {
			af_group[i] = afs[*af_state];
			(*af_state)++;
		} else {
			af_group[i] = AF_CODE_FILLER;
		}
	}

	if (*af_state >= enc->data[enc->program].af_oda.num_entries) {
		*af_state = 0;
	}
}

static uint16_t get_next_af_eon(RDSEncoder* enc, uint8_t eon_index) {
	uint16_t out;

	if (enc->data[enc->program].eon[eon_index].af.num_afs) {
		if (enc->state[enc->program].eon_states[eon_index].af_state == 0) {
			out = (AF_CODE_NUM_AFS_BASE + enc->data[enc->program].af.num_afs) << 8;
			out |= enc->data[enc->program].eon[eon_index].af.afs[0];
			enc->state[enc->program].eon_states[eon_index].af_state += 1;
		} else {
			out = enc->data[enc->program].eon[eon_index].af.afs[enc->state[enc->program].eon_states[eon_index].af_state] << 8;
			if (enc->data[enc->program].eon[eon_index].af.afs[enc->state[enc->program].eon_states[eon_index].af_state + 1]) {
				out |= enc->data[enc->program].eon[eon_index].af.afs[enc->state[enc->program].eon_states[eon_index].af_state + 1];
			} else {
				out |= AF_CODE_FILLER;
				enc->state[enc->program].eon_states[eon_index].af_state += 2;
			}
		}
		if (enc->state[enc->program].eon_states[eon_index].af_state >= enc->data[enc->program].eon[eon_index].af.num_entries) enc->state[enc->program].eon_states[eon_index].af_state = 0;
	} else {
		out = AF_CODE_NUM_AFS_BASE << 8 | AF_CODE_FILLER;
	}

	return out;
}
// #endregion

// #region Group encoding
static void get_rds_ps_group(RDSEncoder* enc, RDSGroup *group) {
	if(enc->state[enc->program].ps_csegment == 0) {
		if(enc->state[enc->program].ps_update) {
			memcpy(enc->state[enc->program].ps_text, enc->data[enc->program].ps, PS_LENGTH);
			enc->state[enc->program].ps_update = 0;
		}

		if(enc->state[enc->program].tps_update) {
			memcpy(enc->state[enc->program].tps_text, enc->data[enc->program].tps, PS_LENGTH);
			enc->state[enc->program].tps_update = 0;
		}
	}

	group->b |= enc->data[enc->program].ta << 4;
	if(enc->state[enc->program].ps_csegment == 0) group->b |= enc->data[enc->program].dpty << 2;
	group->b |= enc->state[enc->program].ps_csegment;
	group->c = get_next_af(enc);
	if(enc->data[enc->program].ta && enc->state[enc->program].tps_text[0] != '\0') {
		group->d = enc->state[enc->program].tps_text[enc->state[enc->program].ps_csegment * 2] << 8 | enc->state[enc->program].tps_text[enc->state[enc->program].ps_csegment * 2 + 1];
	} else {
		group->d = enc->state[enc->program].ps_text[enc->state[enc->program].ps_csegment * 2] << 8 |  enc->state[enc->program].ps_text[enc->state[enc->program].ps_csegment * 2 + 1];
	}

	enc->state[enc->program].ps_csegment++;
	if (enc->state[enc->program].ps_csegment == 4) enc->state[enc->program].ps_csegment = 0;
}

static void get_rds_fasttuning_group(RDSEncoder* enc, RDSGroup *group) {
	group->b |= 15 << 12;
	group->b |= 1 << 11;

	group->b |= enc->data[enc->program].ta << 4;
	if(enc->state[enc->program].fasttuning_state == 0) group->b |= enc->data[enc->program].dpty << 2;
	group->b |= enc->state[enc->program].fasttuning_state;

	// are blocks b and d the same or not?
	group->d = group->b;

	enc->state[enc->program].fasttuning_state++;
	if (enc->state[enc->program].fasttuning_state == 4) enc->state[enc->program].fasttuning_state = 0;

	group->is_type_b = 1;
}

static void get_rds_rt_group(RDSEncoder* enc, RDSGroup *group) {
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

	group->b |= 2 << 12;
	group->b |= ab << 4;
	group->b |= enc->state[enc->program].rt_state;
	group->c =  enc->state[enc->program].rt_text[enc->state[enc->program].rt_state * 4    ] << 8;
	group->c |= enc->state[enc->program].rt_text[enc->state[enc->program].rt_state * 4 + 1];
	group->d =  enc->state[enc->program].rt_text[enc->state[enc->program].rt_state * 4 + 2] << 8;
	group->d |= enc->state[enc->program].rt_text[enc->state[enc->program].rt_state * 4 + 3];

	uint8_t segments = (enc->state[enc->program].current_rt == 1) ? enc->state[enc->program].rt2_segments : enc->state[enc->program].rt_segments;
	enc->state[enc->program].rt_state++;
	if (enc->state[enc->program].rt_state == segments) enc->state[enc->program].rt_state = 0;
}
static void get_rds_rtp_oda_group(RDSEncoder* enc, RDSGroup *group) {
	(void)enc;
	group->b |= 3 << 12;

	group->b |= 11 << 1;
	group->d = ODA_AID_RTPLUS;
}

static void get_rds_ertp_oda_group(RDSEncoder* enc, RDSGroup *group) {
	(void)enc;
	group->b |= 3 << 12;

	group->b |= 13 << 1;
	group->d = ODA_AID_ERTPLUS;
}

static void get_rds_ert_oda_group(RDSEncoder* enc, RDSGroup *group) {
	(void)enc;
	group->b |= 3 << 12;

	group->b |= 12 << 1;
	group->c = 1; // UTF-8
	group->d = ODA_AID_ERT;
}

static void get_oda_af_oda_group(RDSEncoder* enc, RDSGroup *group) {
	(void)enc;
	group->b |= 3 << 12;

	group->b |= 9 << 1;
	group->c = ODA_AID_ODAAF;
}

static void get_oda_af_group(RDSEncoder* enc, RDSGroup *group) {
	uint16_t af[4];
	get_next_af_oda(enc, &af);

	group->b |= 9 << 12;
	for (int i = 0; i < 4; i++) {
		group->b |= ((af[i] >> 15) & 1) << i;
	}

	group->c = af[0] & 0xFFFF;
	group->c <<= 8;
	group->c |= af[1] & 0xFFFF;

	group->d = af[2] & 0xFFFF;
	group->d <<= 8;
	group->d |= af[3] & 0xFFFF;
}

static void get_rds_ct_group(RDSEncoder* enc, RDSGroup *group) {
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

	group->b |= 4 << 12 | (mjd >> 15);
	group->c = (mjd << 1) | (utc->tm_hour >> 4);
	group->d = (utc->tm_hour & 0xf) << 12 | utc->tm_min << 6;

	local_time = localtime(&now);

	offset = local_time->__tm_gmtoff / (30 * 60);
	if (offset < 0) group->d |= 1 << 5;
	group->d |= abs(offset);
}

static void get_rds_ptyn_group(RDSEncoder* enc, RDSGroup *group) {
	if (enc->state[enc->program].ptyn_state == 0 && enc->state[enc->program].ptyn_update) {
		memcpy(enc->state[enc->program].ptyn_text, enc->data[enc->program].ptyn, PTYN_LENGTH);
		enc->state[enc->program].ptyn_ab ^= 1;
		enc->state[enc->program].ptyn_update = 0;
	}

	group->b |= 10 << 12 | enc->state[enc->program].ptyn_state;
	group->b |= enc->state[enc->program].ptyn_ab << 4;
	group->c =  enc->state[enc->program].ptyn_text[enc->state[enc->program].ptyn_state * 4 + 0] << 8;
	group->c |= enc->state[enc->program].ptyn_text[enc->state[enc->program].ptyn_state * 4 + 1];
	group->d =  enc->state[enc->program].ptyn_text[enc->state[enc->program].ptyn_state * 4 + 2] << 8;
	group->d |= enc->state[enc->program].ptyn_text[enc->state[enc->program].ptyn_state * 4 + 3];

	enc->state[enc->program].ptyn_state++;
	if (enc->state[enc->program].ptyn_state == 2) enc->state[enc->program].ptyn_state = 0;
}

static void get_rds_lps_group(RDSEncoder* enc, RDSGroup *group) {
	if (enc->state[enc->program].lps_state == 0 && enc->state[enc->program].lps_update) {
		memcpy(enc->state[enc->program].lps_text, enc->data[enc->program].lps, LPS_LENGTH);
		enc->state[enc->program].lps_update = 0;
	}

	group->b |= 15 << 12 | enc->state[enc->program].lps_state;
	group->c =  enc->state[enc->program].lps_text[enc->state[enc->program].lps_state * 4    ] << 8;
	group->c |= enc->state[enc->program].lps_text[enc->state[enc->program].lps_state * 4 + 1];
	group->d =  enc->state[enc->program].lps_text[enc->state[enc->program].lps_state * 4 + 2] << 8;
	group->d |= enc->state[enc->program].lps_text[enc->state[enc->program].lps_state * 4 + 3];

	enc->state[enc->program].lps_state++;
	if (enc->state[enc->program].lps_state == enc->state[enc->program].lps_segments) enc->state[enc->program].lps_state = 0;
}

static void get_rds_ecc_group(RDSEncoder* enc, RDSGroup *group) {
	group->b |= 1 << 12;
	group->c = enc->data[enc->program].ecc;
}

static void get_rds_slcdata_group(RDSEncoder* enc, RDSGroup *group) {
	group->b |= 1 << 12;
	group->c = 0x6000;
	group->c |= enc->data[enc->program].slc_data;
}

static void get_rds_rtplus_group(RDSEncoder* enc, RDSGroup *group) {
	group->b |= 11 << 12;
	group->b |= enc->rtpState[enc->program][0].toggle << 4 | enc->rtpData[enc->program][0].running << 3;
	group->b |= (enc->rtpData[enc->program][0].type[0] & 0xf8) >> 3;

	group->c =  (enc->rtpData[enc->program][0].type[0] & 0x07) << 13;
	group->c |= (enc->rtpData[enc->program][0].start[0] & 0x3f) << 7;
	group->c |= (enc->rtpData[enc->program][0].len[0] & 0x3f) << 1;
	group->c |= (enc->rtpData[enc->program][0].type[1] & 0xe0) >> 5;

	group->d =  (enc->rtpData[enc->program][0].type[1] & 0x1f) << 11;
	group->d |= (enc->rtpData[enc->program][0].start[1] & 0x3f) << 5;
	group->d |= enc->rtpData[enc->program][0].len[1] & 0x1f;
}

static void get_rds_ertplus_group(RDSEncoder* enc, RDSGroup *group) {
	group->b |= 13 << 12;
	group->b |= enc->rtpState[enc->program][1].toggle << 4 | enc->rtpData[enc->program][1].running << 3;
	group->b |= (enc->rtpData[enc->program][1].type[0] & 0xf8) >> 3;

	group->c =  (enc->rtpData[enc->program][1].type[0] & 0x07) << 13;
	group->c |= (enc->rtpData[enc->program][1].start[0] & 0x3f) << 7;
	group->c |= (enc->rtpData[enc->program][1].len[0] & 0x3f) << 1;
	group->c |= (enc->rtpData[enc->program][1].type[1] & 0xe0) >> 5;

	group->d =  (enc->rtpData[enc->program][1].type[1] & 0x1f) << 11;
	group->d |= (enc->rtpData[enc->program][1].start[1] & 0x3f) << 5;
	group->d |= enc->rtpData[enc->program][1].len[1] & 0x1f;
}

static void get_rds_eon_group(RDSEncoder* enc, RDSGroup *group) {
	RDSEON eon;
get_eon:
	eon = enc->data[enc->program].eon[enc->state[enc->program].eon_index];
	group->b |= 14 << 12;
	group->b |= eon.tp << 4;

	switch (enc->state[enc->program].eon_state)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		group->c = eon.ps[enc->state[enc->program].eon_state*2] << 8;
		group->c |= eon.ps[enc->state[enc->program].eon_state*2 + 1];
		group->b |= enc->state[enc->program].eon_state;
		break;
	case 4:
		group->c = get_next_af_eon(enc, enc->state[enc->program].eon_index);
		group->b |= enc->state[enc->program].eon_state;
		break;
	case 5: // 13
		group->c = eon.pty << 11;
		if(eon.tp) group->c |= eon.ta;
		group->b |= 13;
		break;
	case 6: // 15
		group->c = eon.data;
		group->b |= 15;
		break;
	}

	group->d = eon.pi;

	if(enc->state[enc->program].eon_state == 6) {
		enc->state[enc->program].eon_index++;

		uint8_t i = 0;
		while(i < 4 && !enc->data[enc->program].eon[enc->state[enc->program].eon_index].enabled) {
			enc->state[enc->program].eon_index++;
			if(enc->state[enc->program].eon_index >= 4) {
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

static void get_rds_ert_group(RDSEncoder* enc, RDSGroup *group) {
	if (enc->state[enc->program].ert_state == 0 && enc->state[enc->program].ert_update) {
		memcpy(enc->state[enc->program].ert_text, enc->data[enc->program].ert, ERT_LENGTH);
		enc->state[enc->program].ert_update = 0;
	}
	
	group->b |= 12 << 12 | (enc->state[enc->program].ert_state & 31);
	group->c =  enc->state[enc->program].ert_text[enc->state[enc->program].ert_state * 4    ] << 8;
	group->c |= enc->state[enc->program].ert_text[enc->state[enc->program].ert_state * 4 + 1];
	group->d =  enc->state[enc->program].ert_text[enc->state[enc->program].ert_state * 4 + 2] << 8;
	group->d |= enc->state[enc->program].ert_text[enc->state[enc->program].ert_state * 4 + 3];

	enc->state[enc->program].ert_state++;
	if (enc->state[enc->program].ert_state == enc->state[enc->program].ert_segments) enc->state[enc->program].ert_state = 0;
}
// #endregion

static uint8_t get_rds_custom_groups(RDSEncoder* enc, RDSGroup *group) {
	if(enc->state[enc->program].custom_group[0] == 1) {
		enc->state[enc->program].custom_group[0] = 0;
		group->a = enc->data[enc->program].pi;
		group->b = enc->state[enc->program].custom_group[1];
		group->c = enc->state[enc->program].custom_group[2];
		group->d = enc->state[enc->program].custom_group[3];
		group->is_type_b = (IS_TYPE_B(group->b) != 0);
		return 1;
	}
	return 0;
}
static uint8_t get_rds_custom_groups2(RDSEncoder* enc, RDSGroup *group) {
	if(enc->state[enc->program].custom_group2[0] == 1) {
		enc->state[enc->program].custom_group2[0] = 0;
		group->a = enc->state[enc->program].custom_group2[1];
		group->b = enc->state[enc->program].custom_group2[2];
		group->c = enc->state[enc->program].custom_group2[3];
		group->d = enc->state[enc->program].custom_group2[4];
		group->is_type_b = (group->a == 0 && IS_TYPE_B(group->b));
		return 1;
	}
	return 0;
}

static void get_rds_sequence_group(RDSEncoder* enc, RDSGroup *group, char* grp, uint8_t stream) {
	uint8_t udg_idx;
	uint8_t idx_seq = (stream == 0) ? 1 : 3;
	uint8_t idx2_seq = (stream == 0) ? 0 : 2;
	switch (*grp)
	{
		default:
		case '0':
			if(enc->state[enc->program].grp_seq_idx[idx_seq] < 4) enc->state[enc->program].grp_seq_idx[idx2_seq]--;
			else enc->state[enc->program].grp_seq_idx[idx_seq] = 0;

			enc->state[enc->program].grp_seq_idx[idx_seq]++;
			get_rds_ps_group(enc, group);
			goto group_coded;
		case '1':
			if(enc->state[enc->program].data_ecc == 0 && enc->data[enc->program].slc_data != 0) {
				get_rds_slcdata_group(enc, group);
			} else {
				get_rds_ecc_group(enc, group);
			}
			enc->state[enc->program].data_ecc ^= 1;
			goto group_coded;
		case '2':
			get_rds_rt_group(enc, group);
			goto group_coded;
		case 'A':
			get_rds_ptyn_group(enc, group);
			goto group_coded;
		case 'E':
			get_rds_eon_group(enc, group);
			goto group_coded;
		case 'X':
			udg_idx = enc->state[enc->program].udg_idxs[0];
			group->b = enc->data[enc->program].udg1[udg_idx][0];
			group->c = enc->data[enc->program].udg1[udg_idx][1];
			group->d = enc->data[enc->program].udg1[udg_idx][2];
			enc->state[enc->program].udg_idxs[0]++;
			if(enc->state[enc->program].udg_idxs[0] == enc->data[enc->program].udg1_len) enc->state[enc->program].udg_idxs[0] = 0;
			goto group_coded;
		case 'Y':
			udg_idx = enc->state[enc->program].udg_idxs[1];
			group->b = enc->data[enc->program].udg2[udg_idx][0];
			group->c = enc->data[enc->program].udg2[udg_idx][1];
			group->d = enc->data[enc->program].udg2[udg_idx][2];
			enc->state[enc->program].udg_idxs[1]++;
			if(enc->state[enc->program].udg_idxs[1] == enc->data[enc->program].udg2_len) enc->state[enc->program].udg_idxs[1] = 0;
			goto group_coded;
		case 'R':
			if(enc->state[enc->program].rtp_oda == 0) {
				get_rds_rtplus_group(enc, group);
			} else {
				get_rds_rtp_oda_group(enc, group);
			}
			enc->state[enc->program].rtp_oda ^= 1;
			goto group_coded;
		case 'P':
			if(enc->state[enc->program].ert_oda == 0) {
				get_rds_ertplus_group(enc, group);
			} else {
				get_rds_ertp_oda_group(enc, group);
			}
			enc->state[enc->program].ert_oda ^= 1;
			goto group_coded;
		case 'S':
			if(enc->state[enc->program].ert_oda == 0) {
				get_rds_ert_group(enc, group);
			} else {
				get_rds_ert_oda_group(enc, group);
			}
			enc->state[enc->program].ert_oda ^= 1;
			goto group_coded;
		// TODO: add uecp
		// TODO: readd ODA
		case 'F':
			get_rds_lps_group(enc, group);
			goto group_coded;
		case 'T':
			get_rds_fasttuning_group(enc, group);
			goto group_coded;
		case 'U':
			if(enc->state[enc->program].af_oda == 0) {
				get_oda_af_group(enc, group);
			} else {
				get_oda_af_oda_group(enc, group);
			}
			enc->state[enc->program].af_oda ^= 1;
			goto group_coded;
	}
group_coded:
	return;
}

static uint8_t check_rds_good_group(RDSEncoder* enc, char* grp, uint8_t stream) {
	(void)stream;
	uint8_t good_group = 0;
	if(*grp == '0') good_group = 1;
	if(*grp == '1' && (enc->data[enc->program].ecc != 0 || enc->data[enc->program].slc_data != 0)) good_group = 1;
	if(*grp == '2' && (enc->data[enc->program].rt1_enabled || enc->data[enc->program].rt2_enabled)) good_group = 1;
	if(*grp == 'A' && enc->data[enc->program].ptyn_enabled) good_group = 1;
	if(*grp == 'E') {
		for (int i = 0; i < 4; i++) {
			if (enc->data[enc->program].eon[i].enabled) {
				good_group = 1;
				break;
			}
		}
	}
	if(*grp == 'X' && enc->data[enc->program].udg1_len != 0) good_group = 1;
	if(*grp == 'Y' && enc->data[enc->program].udg2_len != 0) good_group = 1;
	if(*grp == 'R' && enc->rtpData[enc->program][0].enabled) good_group = 1;
	if(*grp == 'P' && enc->rtpData[enc->program][1].enabled && (_strnlen(enc->data[enc->program].ert, 65) < 64)) good_group = 1;
	if(*grp == 'S' && enc->data[enc->program].ert[0] != '\0') good_group = 1;
	if(*grp == 'F' && enc->data[enc->program].lps[0] != '\0') good_group = 1;
	if(*grp == 'T') good_group = 1;
	if(*grp == 'U' && enc->data[enc->program].af_oda.num_afs) good_group = 1;
	return good_group;
}

static void get_rds_group(RDSEncoder* enc, RDSGroup *group, uint8_t stream) {
	group->a = enc->data[enc->program].pi;
	group->b = 0;
	group->c = 0;
	group->d = 0;

	struct tm *utc;
	time_t now;
	time(&now);
	utc = gmtime(&now);

	if (utc->tm_min != enc->state[enc->program].last_minute) {
		enc->state[enc->program].last_minute = utc->tm_min;
		uint8_t eon_has_ta = 0;
		for (int i = 0; i < 4; i++) {
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

		if(enc->data[enc->program].rt1_enabled && enc->data[enc->program].rt_text_timeout != 0) {
			enc->data[enc->program].rt_text_timeout--;
			if(enc->data[enc->program].rt_text_timeout == 0) {
				enc->state[enc->program].rt_update = 1;
				memcpy(enc->state[enc->program].rt_text, enc->data[enc->program].default_rt, RT_LENGTH);
			}
		}

		if(enc->data[enc->program].ct && stream == 0) {
			get_rds_ct_group(enc, group);
			goto group_coded;
		}
	}

	uint8_t good_group = 0;
	uint8_t cant_find_group = 0;
	uint8_t grp_sqc_idx = 0;
	char grp;

	if(stream != 0) {
		group->a = 0;
		if(get_rds_custom_groups2(enc, group)) {
			goto group_coded_rds2;
		}
		if(enc->encoder_data.rds2_mode == 0) { // tunneling
			group->b = enc->state[enc->program].last_stream0_group[0];
			group->c = enc->state[enc->program].last_stream0_group[1];
			group->d = enc->state[enc->program].last_stream0_group[2];
			goto group_coded_rds2;
		} else if(enc->encoder_data.rds2_mode == 1) { // independent tunneling
			while(good_group == 0) {
				grp_sqc_idx = enc->state[enc->program].grp_seq_idx[2];
				if(enc->data[enc->program].grp_sqc_rds2[grp_sqc_idx] == '\0') {
					enc->state[enc->program].grp_seq_idx[2] = 0;
					grp_sqc_idx = 0;
				}
				grp = enc->data[enc->program].grp_sqc_rds2[grp_sqc_idx];
		
				good_group = check_rds_good_group(enc, &grp, stream);
		
				if(!good_group) cant_find_group++;
				else cant_find_group = 0;
				if(!good_group && cant_find_group == 23) {
					cant_find_group = 0;
					break;
				}
		
				enc->state[enc->program].grp_seq_idx[2]++;
			}
			if(!good_group) grp = '0';
		
			get_rds_sequence_group(enc, group, &grp, stream);

			goto group_coded_rds2;
		}
	}

	if(get_rds_custom_groups(enc, group)) {
		goto group_coded;
	}

	while(good_group == 0) {
		grp_sqc_idx = enc->state[enc->program].grp_seq_idx[0];
		if(enc->data[enc->program].grp_sqc[grp_sqc_idx] == '\0') {
			enc->state[enc->program].grp_seq_idx[0] = 0;
			grp_sqc_idx = 0;
		}
		grp = enc->data[enc->program].grp_sqc[grp_sqc_idx];

		good_group = check_rds_good_group(enc, &grp, stream);

		if(!good_group) cant_find_group++;
		else cant_find_group = 0;
		if(!good_group && cant_find_group == 23) {
			cant_find_group = 0;
			break;
		}

		enc->state[enc->program].grp_seq_idx[0]++;
	}
	if(!good_group) grp = '0';

	get_rds_sequence_group(enc, group, &grp, stream);

group_coded_rds2:
	if (group->a == 0 && stream != 0) {
		if(group->is_type_b) group->c = enc->data[enc->program].pi;
		group->b |= enc->data[enc->program].tp << 10;
		group->b |= enc->data[enc->program].pty << 5;
	} else if(stream == 0) {
		goto group_coded;
	}
	return;

group_coded:
	if(stream != 0) {
		goto group_coded_rds2;
	}
	group->b |= enc->data[enc->program].tp << 10;
	group->b |= enc->data[enc->program].pty << 5;
	if(group->is_type_b) group->c = enc->data[enc->program].pi;

	enc->state[enc->program].last_stream0_group[0] = group->b;
	enc->state[enc->program].last_stream0_group[1] = group->c;
	enc->state[enc->program].last_stream0_group[2] = group->d;
	return;
}

void get_rds_bits(RDSEncoder* enc, uint8_t *bits, uint8_t stream) {
	RDSGroup group;
	get_rds_group(enc, &group, stream);
	add_checkwords(&group, bits);
}

void reset_rds_state(RDSEncoder* enc, uint8_t program) {
	RDSEncoder tempCoder;
	tempCoder.program = program;
	memset(&(tempCoder.state[program]), 0, sizeof(RDSState));
	memset(&(tempCoder.rtpState[program]), 0, sizeof(RDSRTPlusState)*2);

	tempCoder.state[program].rt_ab = 1;
	tempCoder.state[program].ptyn_ab = 1;
	set_rds_rt1(&tempCoder, enc->data[program].rt1);
	set_rds_rt2(&tempCoder, enc->data[program].rt2);
	set_rds_ert(&tempCoder, enc->data[program].ert);
	set_rds_ps(&tempCoder, enc->data[program].ps);
	set_rds_tps(&tempCoder, enc->data[program].tps);
	set_rds_ptyn(&tempCoder, enc->data[program].ptyn);
	set_rds_lps(&tempCoder, enc->data[program].lps);
	set_rds_grpseq(&tempCoder, enc->data[program].grp_sqc);
	set_rds_grpseq2(&tempCoder, enc->data[program].grp_sqc_rds2);

	struct tm *utc;
	time_t now;
	time(&now);
	utc = gmtime(&now);
	tempCoder.state[program].last_minute = utc->tm_min;

	for(int i = 0; i < 4; i++) {
		tempCoder.data[program].eon[i].ta = 0;
	}

	memcpy(&(enc->state[program]), &(tempCoder.state[program]), sizeof(RDSState));
	memcpy(&(enc->rtpState[program]), &(tempCoder.rtpState[program]), sizeof(RDSRTPlusState));
}

void set_rds_defaults(RDSEncoder* enc, uint8_t program) {
	memset(&(enc->data[program]), 0, sizeof(RDSData));
	memset(&(enc->rtpData[program]), 0, sizeof(RDSRTPlusData)*2);
	memset(&(enc->encoder_data), 0, sizeof(RDSEncoderData));

	enc->encoder_data.encoder_addr[0] = 255;
	enc->encoder_data.encoder_addr[1] = 255;

	enc->data[program].ct = 1;
	strcpy((char *)enc->data[program].grp_sqc, DEFAULT_GRPSQC);
	enc->data[program].tp = 1;
	enc->data[program].pi = 0xFFFF;
	strcpy((char *)enc->data[program].ps, "* RDS * ");
	enc->data[program].rt1_enabled = 1;

	memset(enc->data[program].rt1, ' ', 59);

	enc->data[program].rt_type = 2;

	reset_rds_state(enc, program);

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

	enc->data[enc->program].rt_text_timeout = enc->data[enc->program].original_rt_text_timeout;

	enc->state[enc->program].rt_update = 1;

	memset(enc->data[enc->program].rt1, ' ', RT_LENGTH);
	while (*rt1 != 0 && len < RT_LENGTH) enc->data[enc->program].rt1[len++] = *rt1++;

	while (len > 0 && enc->data[enc->program].rt1[len - 1] == ' ') {
        len--;
    }

	if (len < RT_LENGTH) {
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

	while (len > 0 && enc->data[enc->program].rt2[len - 1] == ' ') {
		len--;
	}

	if (len < RT_LENGTH) {
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

		while (i < len) {
			i += 4;
			enc->state[enc->program].lps_segments++;
		}
	} else {
		enc->state[enc->program].lps_segments = 8;
	}
}

void set_rds_ert(RDSEncoder* enc, char *ert) {
	uint8_t i = 0, len = 0;

	enc->state[enc->program].ert_update = 1;

	memset(enc->data[enc->program].ert, ' ', RT_LENGTH);
	while (*ert != 0 && len < RT_LENGTH) enc->data[enc->program].ert[len++] = *ert++;

	while (len > 0 && enc->data[enc->program].ert[len - 1] == ' ') {
		len--;
	}

	if (len < RT_LENGTH) {
		enc->state[enc->program].ert_segments = 0;

		enc->data[enc->program].ert[len++] = '\r';

		while (i < len) {
			i += 4;
			enc->state[enc->program].ert_segments++;
		}
	} else {
		enc->state[enc->program].ert_segments = 32;
	}
}

void set_rds_rtplus_flags(RDSEncoder* enc, uint8_t flags) {
	enc->rtpData[enc->program][0].enabled = (flags==2);
	enc->rtpData[enc->program][0].running	= flags & 1;
}

void set_rds_ertplus_flags(RDSEncoder* enc, uint8_t flags) {
	enc->rtpData[enc->program][1].enabled = (flags==2);
	enc->rtpData[enc->program][1].running	= flags & 1;
}

void set_rds_rtplus_tags(RDSEncoder* enc, uint8_t *tags) {
	enc->rtpData[enc->program][0].type[0]	= tags[0] & 0x3f;
	enc->rtpData[enc->program][0].start[0]	= tags[1] & 0x3f;
	enc->rtpData[enc->program][0].len[0]	= tags[2] & 0x3f;
	enc->rtpData[enc->program][0].type[1]	= tags[3] & 0x3f;
	enc->rtpData[enc->program][0].start[1]	= tags[4] & 0x3f;
	enc->rtpData[enc->program][0].len[1]	= tags[5] & 0x1f;

	enc->rtpState[enc->program][0].toggle ^= 1;
	enc->rtpData[enc->program][0].running = 1;
	enc->rtpData[enc->program][0].enabled = 1;
}

void set_rds_ertplus_tags(RDSEncoder* enc, uint8_t *tags) {
	enc->rtpData[enc->program][1].type[0]	= tags[0] & 0x3f;
	enc->rtpData[enc->program][1].start[0]	= tags[1] & 0x3f;
	enc->rtpData[enc->program][1].len[0]	= tags[2] & 0x3f;
	enc->rtpData[enc->program][1].type[1]	= tags[3] & 0x3f;
	enc->rtpData[enc->program][1].start[1]	= tags[4] & 0x3f;
	enc->rtpData[enc->program][1].len[1]	= tags[5] & 0x1f;

	enc->rtpState[enc->program][1].toggle ^= 1;
	enc->rtpData[enc->program][1].running = 1;
	enc->rtpData[enc->program][1].enabled = 1;
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
void set_rds_grpseq2(RDSEncoder* enc, char *grpseq2) {
	uint8_t len = 0;

	if(grpseq2[0] == '\0') {
		memset(enc->data[enc->program].grp_sqc_rds2, 0, 24);
		memcpy(enc->data[enc->program].grp_sqc_rds2, enc->data[enc->program].grp_sqc, 24);
		return;
	}

	memset(enc->data[enc->program].grp_sqc_rds2, 0, 24);
	while (*grpseq2 != 0 && len < 24) enc->data[enc->program].grp_sqc_rds2[len++] = *grpseq2++;
}