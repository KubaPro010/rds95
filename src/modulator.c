#include "modulator.h"

static float waveform[2][FILTER_SIZE];

void Modulator_saveToFile(RDSModulatorParameters *emp, const char *option) {
	char encoderPath[256];
	snprintf(encoderPath, sizeof(encoderPath), "%s/.rdsModulator", getenv("HOME"));
	FILE *file;
	
	RDSModulatorParameters tempEncoder;
	file = fopen(encoderPath, "rb");
	if (file != NULL) {
		fread(&tempEncoder, sizeof(RDSModulatorParameters), 1, file);
		fclose(file);
	} else {
		memcpy(&tempEncoder, emp, sizeof(RDSModulatorParameters));
	}
	
	if (strcmp(option, "LEVEL") == 0) {
		tempEncoder.level = emp->level;
	} else if (strcmp(option, "RDSGEN") == 0) {
		tempEncoder.rdsgen = emp->rdsgen;
	} else if (strcmp(option, "ALL") == 0) {
		tempEncoder.level = emp->level;
		tempEncoder.rdsgen = emp->rdsgen;
	} else {
		return;
	}
	
	file = fopen(encoderPath, "wb");
	if (file == NULL) {
		perror("Error opening file");
		return;
	}
	fwrite(&tempEncoder, sizeof(RDSModulatorParameters), 1, file);
	fclose(file);
}

void Modulator_loadFromFile(RDSModulatorParameters *emp) {
	char encoderPath[256];
	snprintf(encoderPath, sizeof(encoderPath), "%s/.rdsModulator", getenv("HOME"));
	FILE *file = fopen(encoderPath, "rb");
	if (file == NULL) {
		perror("Error opening file");
		return;
	}
	fread(emp, sizeof(RDSModulatorParameters), 1, file);
	fclose(file);
}

int modulatorsaved() {
	char encoderPath[256];
	snprintf(encoderPath, sizeof(encoderPath), "%s/.rdsModulator", getenv("HOME"));
	FILE *file = fopen(encoderPath, "rb");
	if (file) {
		fclose(file);
		return 1;
	}
	return 0;
}

void init_rds_modulator(RDSModulator* rdsMod, RDSEncoder* enc) {
	memset(rdsMod, 0, sizeof(*rdsMod));
	rdsMod->params.level = 1.0f;
	rdsMod->params.rdsgen = 1;

	rdsMod->enc = enc;

	if(STREAMS > 0) rdsMod->data[1].symbol_shifting.symbol_shift = SAMPLES_PER_BIT / 2;

	for (uint8_t i = 0; i < 2; i++) {
		for (uint16_t j = 0; j < FILTER_SIZE; j++) {
			waveform[i][j] = i ?
				+waveform_biphase[j] : -waveform_biphase[j];
		}
	}

	if(modulatorsaved()) {
		Modulator_loadFromFile(&rdsMod->params);
	} else {
		Modulator_saveToFile(&rdsMod->params, "ALL");
	}
}

float get_rds_sample(RDSModulator* rdsMod, uint8_t stream) {
	uint16_t idx;
	float *cur_waveform;
	float sample;
	if (rdsMod->data[stream].sample_count == SAMPLES_PER_BIT) {
		if (rdsMod->data[stream].bit_pos == BITS_PER_GROUP) {
			get_rds_bits(rdsMod->enc, rdsMod->data[stream].bit_buffer, stream);
			rdsMod->data[stream].bit_pos = 0;
		}

		rdsMod->data[stream].cur_bit = rdsMod->data[stream].bit_buffer[rdsMod->data[stream].bit_pos++];
		rdsMod->data[stream].prev_output = rdsMod->data[stream].cur_output;
		rdsMod->data[stream].cur_output = rdsMod->data[stream].prev_output ^ rdsMod->data[stream].cur_bit;

		idx = rdsMod->data[stream].in_sample_index;
		cur_waveform = waveform[rdsMod->data[stream].cur_output];

		for (uint16_t i = 0; i < FILTER_SIZE; i++) {
			rdsMod->data[stream].sample_buffer[idx++] += *cur_waveform++;
			if (idx == SAMPLE_BUFFER_SIZE) idx = 0;
		}

		rdsMod->data[stream].in_sample_index += SAMPLES_PER_BIT;
		if (rdsMod->data[stream].in_sample_index == SAMPLE_BUFFER_SIZE) rdsMod->data[stream].in_sample_index = 0;

		rdsMod->data[stream].sample_count = 0;
	}
	rdsMod->data[stream].sample_count++;

	if(rdsMod->data[stream].symbol_shifting.symbol_shift != 0) {
		rdsMod->data[stream].symbol_shifting.sample_buffer[rdsMod->data[stream].symbol_shifting.sample_buffer_idx++] = rdsMod->data[stream].sample_buffer[rdsMod->data[stream].out_sample_index];

		if (rdsMod->data[stream].symbol_shifting.sample_buffer_idx == rdsMod->data[stream].symbol_shifting.symbol_shift) rdsMod->data[stream].symbol_shifting.sample_buffer_idx = 0;

		sample = rdsMod->data[stream].symbol_shifting.sample_buffer[rdsMod->data[stream].symbol_shifting.sample_buffer_idx];
	} else {
		sample = rdsMod->data[stream].sample_buffer[rdsMod->data[stream].out_sample_index];
	}

	rdsMod->data[stream].sample_buffer[rdsMod->data[stream].out_sample_index++] = 0;
	if (rdsMod->data[stream].out_sample_index == SAMPLE_BUFFER_SIZE)
		rdsMod->data[stream].out_sample_index = 0;
	uint8_t tooutput = 1;
	if (rdsMod->params.rdsgen == 0) {
		tooutput = 0;
	} else {
		if(rdsMod->params.rdsgen == 1) {
			if (stream == 1) {
				tooutput = 0;
			}
		}
	}
	return sample*rdsMod->params.level*tooutput;
}
