#include "modulator.h"

static float waveform[2][FILTER_SIZE];

void Modulator_saveToFile(RDSModulatorParameters *emp, const char *option) {
	char modulatorPath[128];
	snprintf(modulatorPath, sizeof(modulatorPath), "%s/.rdsModulator", getenv("HOME"));
	FILE *file;
	
	RDSModulatorParameters tempMod;
	RDSModulatorParametersFile tempFile;
	memset(&tempFile, 0, sizeof(tempFile));
	file = fopen(modulatorPath, "rb");
	if (file != NULL) {
		fread(&tempFile, sizeof(RDSModulatorParametersFile), 1, file);
		fclose(file);
	} else {
		memset(&tempFile, 0, sizeof(RDSModulatorParametersFile));
		tempFile.check = 160;
		memcpy(&tempFile.params, emp, sizeof(RDSModulatorParameters));
		tempFile.crc = crc16_ccitt((char*)&tempFile, sizeof(RDSModulatorParametersFile) - sizeof(uint16_t));
	}
	memcpy(&tempMod, &tempFile.params, sizeof(RDSModulatorParameters));
	
	if (strcmp(option, "LEVEL") == 0) {
		tempMod.level = emp->level;
	} else if (strcmp(option, "RDSGEN") == 0) {
		tempMod.rdsgen = emp->rdsgen;
	} else if (strcmp(option, "ALL") == 0) {
		tempMod.level = emp->level;
		tempMod.rdsgen = emp->rdsgen;
	} else {
		return;
	}

	memcpy(&tempFile.params, &tempMod, sizeof(RDSModulatorParameters));
	tempFile.check = 160;
	tempFile.crc = crc16_ccitt((char*)&tempFile, sizeof(RDSModulatorParametersFile) - sizeof(uint16_t));
	
	file = fopen(modulatorPath, "wb");
	if (file == NULL) {
		perror("Error opening file");
		return;
	}
	fwrite(&tempFile, sizeof(RDSModulatorParametersFile), 1, file);
	fclose(file);
}

void Modulator_loadFromFile(RDSModulatorParameters *emp) {
	char modulatorPath[128];
	snprintf(modulatorPath, sizeof(modulatorPath), "%s/.rdsModulator", getenv("HOME"));
	FILE *file = fopen(modulatorPath, "rb");
	if (file == NULL) {
		perror("Error opening file");
		return;
	}
	RDSModulatorParametersFile tempFile;
	memset(&tempFile, 0, sizeof(tempFile));
	fread(&tempFile, sizeof(RDSModulatorParametersFile), 1, file);
	if (tempFile.check != 160) {
		fprintf(stderr, "[RDSMODULATOR-FILE] Invalid file format\n");
		fclose(file);
		return;
	}
	uint16_t calculated_crc = crc16_ccitt((char*)&tempFile, sizeof(RDSModulatorParametersFile) - sizeof(uint16_t));
	if (calculated_crc != tempFile.crc) {
		fprintf(stderr, "[RDSMODULATOR-FILE] CRC mismatch! Data may be corrupted\n");
		fclose(file);
		return;
	}
	memcpy(emp, &tempFile.params, sizeof(RDSModulatorParameters));
	fclose(file);
}

int modulatorsaved() {
	char encoderPath[128];
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
	float *cur_waveform;
	if (rdsMod->data[stream].sample_count == SAMPLES_PER_BIT) {
		if (rdsMod->data[stream].bit_pos == BITS_PER_GROUP) {
			get_rds_bits(rdsMod->enc, rdsMod->data[stream].bit_buffer, stream);
			rdsMod->data[stream].bit_pos = 0;
		}

		rdsMod->data[stream].cur_bit = rdsMod->data[stream].bit_buffer[rdsMod->data[stream].bit_pos++];
		rdsMod->data[stream].prev_output = rdsMod->data[stream].cur_output;
		rdsMod->data[stream].cur_output = rdsMod->data[stream].prev_output ^ rdsMod->data[stream].cur_bit;

		uint16_t idx = rdsMod->data[stream].in_sample_index;
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

	float sample = rdsMod->data[stream].sample_buffer[rdsMod->data[stream].out_sample_index];
	if(stream != 0 && rdsMod->data[stream].symbol_shifting.symbol_shift != 0) {
		rdsMod->data[stream].symbol_shifting.sample_buffer[rdsMod->data[stream].symbol_shifting.sample_buffer_idx++] = rdsMod->data[stream].sample_buffer[rdsMod->data[stream].out_sample_index];

		if (rdsMod->data[stream].symbol_shifting.sample_buffer_idx == rdsMod->data[stream].symbol_shifting.symbol_shift) rdsMod->data[stream].symbol_shifting.sample_buffer_idx = 0;

		sample = rdsMod->data[stream].symbol_shifting.sample_buffer[rdsMod->data[stream].symbol_shifting.sample_buffer_idx];
	}

	rdsMod->data[stream].sample_buffer[rdsMod->data[stream].out_sample_index++] = 0;
	if (rdsMod->data[stream].out_sample_index == SAMPLE_BUFFER_SIZE) rdsMod->data[stream].out_sample_index = 0;
	
	uint8_t tooutput = rdsMod->params.rdsgen > stream ? 1 : 0;
	return sample*rdsMod->params.level*tooutput;
}
