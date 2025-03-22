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

float get_rds_sample(RDSModulator* rdsMod) {
	uint16_t idx;
	float *cur_waveform;
	float sample;
	if (rdsMod->sample_count == SAMPLES_PER_BIT) {
		if (rdsMod->bit_pos == BITS_PER_GROUP) {
			get_rds_bits(rdsMod->enc, rdsMod->bit_buffer);
			rdsMod->bit_pos = 0;
		}

		rdsMod->cur_bit = rdsMod->bit_buffer[rdsMod->bit_pos++];
		rdsMod->prev_output = rdsMod->cur_output;
		rdsMod->cur_output = rdsMod->prev_output ^ rdsMod->cur_bit;

		idx = rdsMod->in_sample_index;
		cur_waveform = waveform[rdsMod->cur_output];

		for (uint16_t i = 0; i < FILTER_SIZE; i++) {
			rdsMod->sample_buffer[idx++] += *cur_waveform++;
			if (idx == SAMPLE_BUFFER_SIZE) idx = 0;
		}

		rdsMod->in_sample_index += SAMPLES_PER_BIT;
		if (rdsMod->in_sample_index == SAMPLE_BUFFER_SIZE) rdsMod->in_sample_index = 0;

		rdsMod->sample_count = 0;
	}
	rdsMod->sample_count++;

	sample = rdsMod->sample_buffer[rdsMod->out_sample_index];

	rdsMod->sample_buffer[rdsMod->out_sample_index++] = 0;
	if (rdsMod->out_sample_index == SAMPLE_BUFFER_SIZE)
		rdsMod->out_sample_index = 0;
	return sample*rdsMod->params.level*rdsMod->params.rdsgen;
}
