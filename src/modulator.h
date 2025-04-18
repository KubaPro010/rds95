#pragma once
#include "common.h"
#include "lib.h"
#include "rds.h"
#include "waveforms.h"

typedef struct
{
	float level;
	uint8_t rdsgen : 2;
} RDSModulatorParameters;

typedef struct
{
	uint8_t check;
	RDSModulatorParameters params;
	uint16_t crc;
} RDSModulatorParametersFile;

typedef struct
{
	uint8_t symbol_shift: 7;
	float sample_buffer[SAMPLE_BUFFER_SIZE];
	uint8_t sample_buffer_idx : 7;
} RDSModulatorSymbolShifting;

typedef struct
{
	uint8_t bit_buffer[BITS_PER_GROUP];
	uint8_t bit_pos : 7;
	float sample_buffer[SAMPLE_BUFFER_SIZE];
	uint8_t prev_output : 1;
	uint8_t cur_output : 1;
	uint8_t cur_bit : 1;
	uint8_t sample_count;
	uint16_t in_sample_index;
	uint16_t out_sample_index;
	RDSModulatorSymbolShifting symbol_shifting;
} RDSModulatorModulationData;

typedef struct {
	RDSModulatorModulationData data[STREAMS];
	RDSModulatorParameters params;
	RDSEncoder* enc;
} RDSModulator;

void Modulator_saveToFile(RDSModulatorParameters *emp, const char *option);
void Modulator_loadFromFile(RDSModulatorParameters *emp);
int modulatorsaved();
void init_rds_modulator(RDSModulator* rdsMod, RDSEncoder* enc);
float get_rds_sample(RDSModulator* rdsMod, uint8_t stream);
