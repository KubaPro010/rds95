#include "modulator.h"

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
		tempFile.crc = crc16_ccitt((char*)&tempFile, offsetof(RDSModulatorParametersFile, crc));
	}
	memcpy(&tempMod, &tempFile.params, sizeof(RDSModulatorParameters));
	
	if (strcmp(option, "LEVEL") == 0) tempMod.level = emp->level;
	else if (strcmp(option, "RDSGEN") == 0) tempMod.rdsgen = emp->rdsgen;
	else if (strcmp(option, "ALL") == 0) {
		tempMod.level = emp->level;
		tempMod.rdsgen = emp->rdsgen;
	} else return;

	memcpy(&tempFile.params, &tempMod, sizeof(RDSModulatorParameters));
	tempFile.check = 160;
	tempFile.crc = crc16_ccitt((char*)&tempFile, offsetof(RDSModulatorParametersFile, crc));
	
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
	uint16_t calculated_crc = crc16_ccitt((char*)&tempFile, offsetof(RDSModulatorParametersFile, crc));
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

	if(STREAMS > 0) rdsMod->data[1].symbol_shift = M_PI;
	if(modulatorsaved()) {
		Modulator_loadFromFile(&rdsMod->params);
	} else {
		Modulator_saveToFile(&rdsMod->params, "ALL");
	}
}

float get_rds_sample(RDSModulator* rdsMod, uint8_t stream) {
	rdsMod->data[stream].phase += 1187.5 / RDS_SAMPLE_RATE;

	if (rdsMod->data[stream].phase >= 1.0f) {
		rdsMod->data[stream].phase -= 1.0f;
		if (rdsMod->data[stream].bit_pos == BITS_PER_GROUP) {
			get_rds_bits(rdsMod->enc, rdsMod->data[stream].bit_buffer, stream);
			rdsMod->data[stream].bit_pos = 0;
		}

		rdsMod->data[stream].cur_bit = rdsMod->data[stream].bit_buffer[rdsMod->data[stream].bit_pos++];
		rdsMod->data[stream].prev_output = rdsMod->data[stream].cur_output;
		rdsMod->data[stream].cur_output = rdsMod->data[stream].prev_output ^ rdsMod->data[stream].cur_bit;
	}

	float sample = sinf(M_2PI * rdsMod->data[stream].phase + rdsMod->data[stream].symbol_shift);
	if(rdsMod->data[stream].cur_output == 0) sample = -sample; // do bpsk
	
	uint8_t tooutput = rdsMod->params.rdsgen > stream ? 1 : 0;
	return sample*rdsMod->params.level*tooutput;
}
