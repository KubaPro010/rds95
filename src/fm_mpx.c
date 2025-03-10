/*
 * mpxgen - FM multiplex encoder with Stereo and RDS
 * Copyright (C) 2019 Anthony96922
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

static float mpx_vol;

static uint8_t rdsgen;

void set_output_volume(float vol) {
	if (vol > 100.0f) vol = 100.0f;
	mpx_vol = vol / 100.0f;
}

void set_rdsgen(uint8_t gen) {
	if (gen > 1) gen = 1;
	rdsgen = gen;
}

void set_carrier_volume(uint8_t carrier, float new_volume) {
	/* check for valid index */
	if (carrier >= MPX_SUBCARRIER_END) return;

	volumes[carrier] = new_volume / 100.0f;
}

void fm_mpx_init(uint32_t sample_rate) {
	rdsgen = 1;
}

void fm_rds_get_frames(float *outbuf, size_t num_frames) {
	size_t j = 0;
	float out;

	for (size_t i = 0; i < num_frames; i++) {
		out = 0.0f;

		out += get_rds_sample(0)
			* volumes[MPX_SUBCARRIER_RDS_STREAM_0];

		/* clipper */
		out = fminf(+1.0f, out);
		out = fmaxf(-1.0f, out);

		/* adjust volume and put into channel */
		if(rdsgen != 0) outbuf[j] = out * mpx_vol;
		j++;

	}
}

void fm_mpx_exit() {
	return;
}
