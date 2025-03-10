/*
 * mpxgen - FM multiplex encoder with Stereo and RDS
 * Copyright (C) 2021 Anthony96922
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"
#include "rds.h"
#include "fm_mpx.h"
#include "waveforms.h"
#include "modulator.h"

static struct rds_t **rds_ctx;
static float **waveform;

/*
 * Create the RDS objects
 *
 */
void init_rds_objects() {
	rds_ctx = malloc(sizeof(struct rds_t));

	rds_ctx[0] = malloc(sizeof(struct rds_t));
	rds_ctx[0]->bit_buffer = malloc(BITS_PER_GROUP);
	rds_ctx[0]->sample_buffer =
		malloc(SAMPLE_BUFFER_SIZE * sizeof(float));
	rds_ctx[0]->symbol_shift_buf_idx = 0;

	waveform = malloc(2 * sizeof(float));

	for (uint8_t i = 0; i < 2; i++) {
		waveform[i] = malloc(FILTER_SIZE * sizeof(float));
		for (uint16_t j = 0; j < FILTER_SIZE; j++) {
			waveform[i][j] = i ?
				+waveform_biphase[j] : -waveform_biphase[j];
		}
	}
}

void exit_rds_objects() {
	int has_symbol_shift = rds_ctx[0]->symbol_shift;
	if (has_symbol_shift) {
		free(rds_ctx[0]->symbol_shift_buf);
	}
	free(rds_ctx[0]->sample_buffer);
	free(rds_ctx[0]->bit_buffer);
	free(rds_ctx[0]);
	free(rds_ctx);

	for (uint8_t i = 0; i < 2; i++) {
		free(waveform[i]);
	}

	free(waveform);
}

/* Get an RDS sample. This generates the envelope of the waveform using
 * pre-generated elementary waveform samples.
 */
float get_rds_sample(uint8_t stream_num) {
	struct rds_t *rds;
	uint16_t idx;
	float *cur_waveform;
	float sample;

	/* select context */
	rds = rds_ctx[stream_num];

	if (rds->sample_count == SAMPLES_PER_BIT) {
		if (rds->bit_pos == BITS_PER_GROUP) {
			get_rds_bits(rds->bit_buffer);
			rds->bit_pos = 0;
		}

                /* do differential encoding */
                rds->cur_bit = rds->bit_buffer[rds->bit_pos++];
                rds->prev_output = rds->cur_output;
                rds->cur_output = rds->prev_output ^ rds->cur_bit;

                idx = rds->in_sample_index;
                cur_waveform = waveform[rds->cur_output];

    uint16_t buffer_remaining = SAMPLE_BUFFER_SIZE - idx;
    if (buffer_remaining >= FILTER_SIZE) {
        // Process continuous chunk
        for (uint16_t i = 0; i < FILTER_SIZE; i++) {
            rds->sample_buffer[idx + i] += cur_waveform[i];
        }
        idx += FILTER_SIZE;
    } else {
        // Process in two parts to handle wrapping
        for (uint16_t i = 0; i < buffer_remaining; i++) {
            rds->sample_buffer[idx + i] += cur_waveform[i];
        }
        for (uint16_t i = 0; i < FILTER_SIZE - buffer_remaining; i++) {
            rds->sample_buffer[i] += cur_waveform[buffer_remaining + i];
        }
        idx = FILTER_SIZE - buffer_remaining;
    }
                rds->in_sample_index += SAMPLES_PER_BIT;
                if (rds->in_sample_index == SAMPLE_BUFFER_SIZE)
                        rds->in_sample_index = 0;

                rds->sample_count = 0;
        }
        rds->sample_count++;

        if (rds->symbol_shift) {
                rds->symbol_shift_buf[rds->symbol_shift_buf_idx++] =
                        rds->sample_buffer[rds->out_sample_index];

                if (rds->symbol_shift_buf_idx == rds->symbol_shift)
                        rds->symbol_shift_buf_idx = 0;

                sample = rds->symbol_shift_buf[rds->symbol_shift_buf_idx];

                goto done;
        }

        sample = rds->sample_buffer[rds->out_sample_index];
done:
        rds->sample_buffer[rds->out_sample_index++] = 0;
        if (rds->out_sample_index == SAMPLE_BUFFER_SIZE)
                rds->out_sample_index = 0;
        return sample;
}
