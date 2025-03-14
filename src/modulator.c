#include "common.h"
#include "rds.h"
#include "waveforms.h"
#include "modulator.h"

static struct rds_t rds;
static float waveform[2][FILTER_SIZE];

static float level;
static uint8_t rdsgen;

void init_rds_objects() {
	level = 1.0f;

	memset(&rds, 0, sizeof(rds));

	for (uint8_t i = 0; i < 2; i++) {
		// This is the bpsk, so waveform[0] is 0 degrees out of phase but waveform[1] is 180 degrees, bpsk
		for (uint16_t j = 0; j < FILTER_SIZE; j++) {
			waveform[i][j] = i ?
				+waveform_biphase[j] : -waveform_biphase[j];
		}
	}
}

void set_rds_level(float _level) {
	level = fminf(1.0f, fmaxf(0.0f, _level));
}
void set_rds_gen(uint8_t rdsgen) {
	rdsgen = min(1, max(0, rdsgen)); // No RDS2
}

/* Get an RDS sample. This generates the envelope of the waveform using
 * pre-generated elementary waveform samples.
 */
float get_rds_sample() {
	uint16_t idx;
	float *cur_waveform;
	float sample;
	if (rds.sample_count == SAMPLES_PER_BIT) {
		// New Sample
		if (rds.bit_pos == BITS_PER_GROUP) {
			// New bit stream
			get_rds_bits(rds.bit_buffer);
			rds.bit_pos = 0;
		}

		// Differentially encode, so 1111 becomes 1000 and 0001 becomes 0001
		rds.cur_bit = rds.bit_buffer[rds.bit_pos++];
		rds.prev_output = rds.cur_output;
		rds.cur_output = rds.prev_output ^ rds.cur_bit;

		idx = rds.in_sample_index;
		cur_waveform = waveform[rds.cur_output]; // get the waveform, this is the biphase in a 0/180 degree phase shift

		for (uint16_t i = 0; i < FILTER_SIZE; i++) {
			rds.sample_buffer[idx++] += *cur_waveform++;
			if (idx == SAMPLE_BUFFER_SIZE) idx = 0;
		}

		rds.in_sample_index += SAMPLES_PER_BIT;
		if (rds.in_sample_index == SAMPLE_BUFFER_SIZE)
			rds.in_sample_index = 0;

		rds.sample_count = 0;
	}
	rds.sample_count++;

	sample = rds.sample_buffer[rds.out_sample_index];

	rds.sample_buffer[rds.out_sample_index++] = 0;
	if (rds.out_sample_index == SAMPLE_BUFFER_SIZE)
			rds.out_sample_index = 0;
	if(rdsgen == 0) sample = 0.0f;
	return sample*level;
}
