#pragma once
#include "common.h"
#include "rds.h"
#include "waveforms.h"

typedef struct {
	uint8_t bit_buffer[BITS_PER_GROUP];
	uint8_t bit_pos;
	float sample_buffer[SAMPLE_BUFFER_SIZE];
	uint8_t prev_output;
	uint8_t cur_output;
	uint8_t cur_bit;
	uint8_t sample_count;
	uint16_t in_sample_index;
	uint16_t out_sample_index;
	float level;
	uint8_t rdsgen;
	RDSEncoder* enc;
} RDSModulator;

void init_rds_modulator(RDSModulator* rdsMod, RDSEncoder* enc);
float get_rds_sample(RDSModulator* rdsMod);
