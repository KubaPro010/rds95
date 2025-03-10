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
#include "modulator.h"
#ifdef RDS2
#include "rds2.h"
#endif
#include "lib.h"
#include <time.h>

static struct rds_params_t rds_data;

/* RDS data controls */
static struct {
	/* Slow Labeling Codes */
	uint8_t ecclic_enabled;
	uint8_t ecc_or_lic;
	/* Program Item Number */
	uint8_t pin_enabled;

	/* Program Service */
	uint8_t ps_update;
	uint8_t tps_update;

	/* Radio Text*/
	uint8_t rt1_enabled;
	uint8_t rt_update;
	uint8_t rt_ab;
	uint8_t rt_segments;

	/* PTYN */
	uint8_t ptyn_enabled;
	uint8_t ptyn_update;
	uint8_t ptyn_ab;

	/* Long PS */
	uint8_t lps_on;
	uint8_t lps_update;
	uint8_t lps_segments;

	uint8_t custom_group_in;
	uint16_t custom_group[GROUP_LENGTH];

	/* TODO: add UDG */
} rds_state;

#ifdef ODA
/* ODA */
static struct rds_oda_t odas[MAX_ODAS];
static struct {
	uint8_t current;
	uint8_t count;
} oda_state;
#endif

#ifdef ODA_RTP
/* RT+ */
static struct {
	uint8_t group;
	uint8_t enabled;
	uint8_t running;
	uint8_t toggle;
	uint8_t type[2];
	uint8_t start[2];
	uint8_t len[2];
} rtplus_cfg;
#endif

#ifdef ODA
static void register_oda(uint8_t group, uint16_t aid, uint16_t scb) {

	if (oda_state.count >= MAX_ODAS) return; /* can't accept more ODAs */

	odas[oda_state.count].group = group;
	odas[oda_state.count].aid = aid;
	odas[oda_state.count].scb = scb;
	oda_state.count++;
}
#endif

/* Get the next AF entry
 */
static uint16_t get_next_af() {
	static uint8_t af_state;
	uint16_t out;

	if (rds_data.af.num_afs) {
		if (af_state == 0) {
			out = (AF_CODE_NUM_AFS_BASE + rds_data.af.num_afs) << 8;
			out |= rds_data.af.afs[0];
			af_state += 1;
		} else {
			out = rds_data.af.afs[af_state] << 8;
			if (rds_data.af.afs[af_state + 1])
				out |= rds_data.af.afs[af_state + 1];
			else
				out |= AF_CODE_FILLER;
			af_state += 2;
		}
		if (af_state >= rds_data.af.num_entries) af_state = 0;
	} else {
		out = AF_CODE_NO_AF << 8 | AF_CODE_FILLER;
	}

	return out;
}

/* PS group (0A) */
static void get_rds_ps_group(uint16_t *blocks) {
	static unsigned char ps_text[PS_LENGTH];
	static unsigned char tps_text[PS_LENGTH];
	static uint8_t ps_csegment;

	if (ps_csegment == 0 && rds_state.ps_update) {
		memcpy(ps_text, rds_data.ps, PS_LENGTH);
		rds_state.ps_update = 0; /* rewind */
	}
	if(ps_csegment == 0 && rds_state.tps_update) {
		memcpy(tps_text, rds_data.tps, PS_LENGTH);
		rds_state.tps_update = 0; /* rewind */
	}

	/* TA */
	blocks[1] |= rds_data.ta << 4;

	/* MS */
	blocks[1] |= rds_data.ms << 3;

	/* DI */
	blocks[1] |= ((rds_data.di >> (3 - ps_csegment)) & INT8_0) << 2; /* DI  is a 4 bit number, we also have 4 segments, so we send a bit of the di number depending on the segment, so 0b0101 would be like: 0 1 0 1*/

	/* PS segment address */
	blocks[1] |= ps_csegment;

	/* AF */
	blocks[2] = get_next_af();

	/* PS */
	if(rds_data.ta && rds_data.traffic_ps_on) {
		blocks[3] = tps_text[ps_csegment * 2] << 8 | tps_text[ps_csegment * 2 + 1];
	} else {
		/* TODO: Add DPS */
		blocks[3] =  ps_text[ps_csegment * 2] << 8 |  ps_text[ps_csegment * 2 + 1];
	}

	ps_csegment++;
	if (ps_csegment >= 4) ps_csegment = 0;
}

/* RT group (2A) */
static uint8_t get_rds_rt_group(uint16_t *blocks) {
	static unsigned char rt_text[RT_LENGTH];
	static uint8_t rt_state;

	if (rds_state.rt_update) {
		memcpy(rt_text, rds_data.rt1, RT_LENGTH);
		rds_state.rt_ab ^= 1;
		rds_state.rt_update = 0;
		rt_state = 0; /* rewind when new RT arrives */
	}

	if(rt_text[0] == '\r' || !rds_state.rt1_enabled) {
		/* RT strings lesser than 64 in size should have been ended with a \r if a return is on [0] then that means that our string is empty and thus we can not generate this group*/
		return 0;
	}

	blocks[1] |= 2 << 12;
	blocks[1] |= rds_state.rt_ab << 4;
	blocks[1] |= rt_state;
	blocks[2] =  rt_text[rt_state * 4    ] << 8;
	blocks[2] |= rt_text[rt_state * 4 + 1];
	blocks[3] =  rt_text[rt_state * 4 + 2] << 8;
	blocks[3] |= rt_text[rt_state * 4 + 3];

	rt_state++;
	if (rt_state >= rds_state.rt_segments) rt_state = 0;
	return 1;
}

/* ODA group (3A)
 */
#ifdef ODA
static void get_rds_oda_group(uint16_t *blocks) {
	/* select ODA */
	struct rds_oda_t this_oda = odas[oda_state.current];

	#ifdef ODA_RTP
	if(this_oda.aid == ODA_AID_RTPLUS && rtplus_cfg.enabled == 0) {
		return;
	}
	#endif

	blocks[1] |= 3 << 12;

	blocks[1] |= GET_GROUP_TYPE(this_oda.group) << 1;
	blocks[1] |= GET_GROUP_VER(this_oda.group);
	blocks[2] = this_oda.scb;
	blocks[3] = this_oda.aid;

	oda_state.current++;
	if (oda_state.current >= oda_state.count) oda_state.current = 0;
}
#endif

static uint8_t get_rds_ct_group(uint16_t *blocks) {
	static uint8_t latest_minutes;
	struct tm *utc, *local_time;
	time_t now;
	uint8_t l;
	uint32_t mjd;
	int16_t offset;

	/* Check time */
	now = time(NULL);
	utc = gmtime(&now);

	if (utc->tm_min != latest_minutes) {
		/* Generate CT group */
		latest_minutes = utc->tm_min;

		l = utc->tm_mon <= 1 ? 1 : 0;
		mjd = 14956 + utc->tm_mday +
			(uint32_t)((utc->tm_year - l) * 365.25f) +
			(uint32_t)((utc->tm_mon + (1+1) + l * 12) * 30.6001f);

		blocks[1] |= 4 << 12 | (mjd >> 15);
		blocks[2] = (mjd << 1) | (utc->tm_hour >> 4);
		blocks[3] = (utc->tm_hour & INT16_L4) << 12 | utc->tm_min << 6;

		/* get local time (for the offset) */
		local_time = localtime(&now);

		/* tm_gmtoff doesn't exist in POSIX but __tm_gmtoff does */
		offset = local_time->__tm_gmtoff / (30 * 60); /*__tm_gmtoff is time in seconds from utc, so utc + __tm_gmtoff = local time*/
		if (offset < 0) blocks[3] |= 1 << 5; /* if less than 0 then set the flag*/
		blocks[3] |= abs(offset); /* offset should be halfs of a hour, so offset 1 = 30 minutes, 2 = 1 hour*/

		return 1;
	}

	return 0;
}

/* PTYN group (10A) */
static void get_rds_ptyn_group(uint16_t *blocks) {
	static unsigned char ptyn_text[PTYN_LENGTH];
	static uint8_t ptyn_state;

	if (ptyn_state == 0 && rds_state.ptyn_update) {
		memcpy(ptyn_text, rds_data.ptyn, PTYN_LENGTH);
		rds_state.ptyn_ab ^= 1;
		rds_state.ptyn_update = 0;
	}

	blocks[1] |= 10 << 12 | ptyn_state;
	blocks[1] |= rds_state.ptyn_ab << 4; /* no one knew about ptyn's ab until i saw sdr++'s decoder, it had decoded ptyn but not show it? it also did decode the ab flag */
	blocks[2] =  ptyn_text[ptyn_state * 4 + 0] << 8;
	blocks[2] |= ptyn_text[ptyn_state * 4 + 1];
	blocks[3] =  ptyn_text[ptyn_state * 4 + 2] << 8;
	blocks[3] |= ptyn_text[ptyn_state * 4 + 3];

	ptyn_state++;
	if (ptyn_state == 2) ptyn_state = 0;
}

/* Long PS group (15A) */
static void get_rds_lps_group(uint16_t *blocks) {
	static unsigned char lps_text[LPS_LENGTH];
	static uint8_t lps_state;

	if (lps_state == 0 && rds_state.lps_update) {
		memcpy(lps_text, rds_data.lps, LPS_LENGTH);
		rds_state.lps_update = 0;
	}

	blocks[1] |= 15 << 12 | lps_state;
	/* does lps have an ab? */
	blocks[2] =  lps_text[lps_state * 4    ] << 8;
	blocks[2] |= lps_text[lps_state * 4 + 1];
	blocks[3] =  lps_text[lps_state * 4 + 2] << 8;
	blocks[3] |= lps_text[lps_state * 4 + 3];

	lps_state++;
	if (lps_state == rds_state.lps_segments) lps_state = 0;
}

static void get_rds_ecc_group(uint16_t *blocks) {
	blocks[1] |= 1 << 12;
	if(rds_state.ecclic_enabled) {
		blocks[2] = rds_data.ecc;
	}

	if(rds_state.pin_enabled) {
		blocks[3] = rds_data.pin_day << 11; /* first 5 bits */
		blocks[3] |= rds_data.pin_hour << 6; /* 5-10 bits from start */
		blocks[3] |= rds_data.pin_minute; /* 10-16 bits from start */
	}
}

static void get_rds_lic_group(uint16_t *blocks) {
	blocks[1] |= 1 << 12;
	if(rds_state.ecclic_enabled) {
		blocks[2] = 0x3000; /* 0b0011000000000000 first 1 bit is the linkage actuator, the 3 after are variant codes which is 3 in this case after theres space for data */
		blocks[2] |= rds_data.lic;
	}

	if(rds_state.pin_enabled) {
		/* pin is also in lic as it also is a 1a group*/
		blocks[3] = rds_data.pin_day << 11;
		blocks[3] |= rds_data.pin_hour << 6;
		blocks[3] |= rds_data.pin_minute;
	}
}

/* RT+ */
#ifdef ODA_RTP
static void init_rtplus(uint8_t group) {
	register_oda(group, ODA_AID_RTPLUS, 0);
	rtplus_cfg.group = group;
	rtplus_cfg.enabled = 1;
}
#endif

#ifdef ODA_RTP
/* RT+ group */
static void get_rds_rtplus_group(uint16_t *blocks) {
	/* RT+ block format */
	blocks[1] |= GET_GROUP_TYPE(rtplus_cfg.group) << 12;
	blocks[1] |= GET_GROUP_VER(rtplus_cfg.group) << 11;
	blocks[1] |= rtplus_cfg.toggle << 4 | rtplus_cfg.running << 3;
	blocks[1] |= (rtplus_cfg.type[0] & INT8_U5) >> 3;

	blocks[2] =  (rtplus_cfg.type[0] & INT8_L3) << 13;
	blocks[2] |= (rtplus_cfg.start[0] & INT8_L6) << 7;
	blocks[2] |= (rtplus_cfg.len[0] & INT8_L6) << 1;
	blocks[2] |= (rtplus_cfg.type[1] & INT8_U3) >> 5;

	blocks[3] =  (rtplus_cfg.type[1] & INT8_L5) << 11;
	blocks[3] |= (rtplus_cfg.start[1] & INT8_L6) << 5;
	blocks[3] |= rtplus_cfg.len[1] & INT8_L5;
}
#endif

/* Lower priority groups are placed in a subsequence
 */
static uint8_t get_rds_other_groups(uint16_t *blocks) {
	static uint8_t group_counter[GROUP_15B];

	/* Type 1A groups */
	if (rds_data.ecc || rds_data.lic) {
		if (++group_counter[GROUP_1A] >= 22) {
			group_counter[GROUP_1A] = 0;
			if(rds_data.ecc && rds_data.lic) {
				if(rds_state.ecc_or_lic == 0) {
					get_rds_ecc_group(blocks);
				} else {
					get_rds_lic_group(blocks);
				}
				rds_state.ecc_or_lic ^= 1;
			} else if(rds_data.ecc) {
				get_rds_ecc_group(blocks);
			} else if(rds_data.ecc) {
				get_rds_lic_group(blocks);
			}
			return 1;
		}
	}

	/* Type 3A groups */
	#ifdef ODA
	if (oda_state.count) {
		if (++group_counter[GROUP_3A] >= 25) {
			group_counter[GROUP_3A] = 0;
			get_rds_oda_group(blocks);
			return 1;
		}
	}
	#endif

	/* Type 10A groups */
	if (rds_state.ptyn_enabled && rds_data.ptyn[0]) {
		if (++group_counter[GROUP_10A] >= 13) {
			group_counter[GROUP_10A] = 0;
			/* Do not generate a 10A group if PTYN is off */
			get_rds_ptyn_group(blocks);
			return 1;
		}
	}

	/* RT+ groups */
	#ifdef ODA_RTP
	if (++group_counter[rtplus_cfg.group] >= 30) {
		group_counter[rtplus_cfg.group] = 0;
		if(rtplus_cfg.enabled == 0) {
			return 0;
		}
		get_rds_rtplus_group(blocks);
		return 1;
	}
	#endif

	return 0;
}

/*
 * Codes a group once every 3 groups
 * Ex: 0A, 2A, 0A, 12A, 2A, 0A, 2A, 12A, etc
 */
static uint8_t get_rds_long_text_groups(uint16_t *blocks) {
	static uint8_t group_selector = 0;
	static uint8_t group_slot_counter = 0;

	/* exit early until the 4th call */
	if (++group_slot_counter < 4)
		return 0;

	/* reset the slot counter */
	group_slot_counter = 0;

	/* 3:1 ratio */
	switch (group_selector) {
	case 0:
	case 1:
	case 2:
	case 3: /* Long PS */
		if (rds_data.lps[0] && rds_state.lps_on) {
			get_rds_lps_group(blocks);
			goto group_coded;
		}
		break;
	}

	/* if no group was coded */
	if (++group_selector >= 8) group_selector = 0;
	return 0;

group_coded:
	if (++group_selector >= 8) group_selector = 0;
	return 1;
}

/* Get a custom group if there is one*/
static uint8_t get_rds_custom_groups(uint16_t *blocks) {
	if(rds_state.custom_group_in) {
		rds_state.custom_group_in = 0;
		blocks[0] = rds_state.custom_group[0];
		blocks[1] = rds_state.custom_group[1];
		blocks[2] = rds_state.custom_group[2];
		blocks[3] = rds_state.custom_group[3];
		return 1;
	}
	return 0;
}

/* Creates an RDS group.
 * This generates sequences of the form 0A, 2A, 0A, 2A, 0A, 2A, etc.
 */
static void get_rds_group(uint16_t *blocks) {
	static uint8_t state;

	/* Basic block data */
	blocks[0] = rds_data.pi;
	blocks[1] = rds_data.tp << 10;
	blocks[1] |= rds_data.pty << 5;
	blocks[2] = 0;
	blocks[3] = 0;

	/* Generate block content */

	/* CT (clock time) has priority over other group types */
	if (rds_data.ct && get_rds_ct_group(blocks)) {
		goto group_coded;
	}

	/* Next to top priority, if want top then just disable CT i guess */
	if(get_rds_custom_groups(blocks)) {
		goto group_coded;
	}

	/* Longer text groups get medium priority */
	if (get_rds_long_text_groups(blocks)) {
		goto group_coded;
	}

	/* Other groups */
	if (get_rds_other_groups(blocks)) {
		goto group_coded;
	}

	/* Standard group sequence
	This group sequence is way better than the offcial one, official goes like: 0A 2A which is shitty here it is 0A 0A 0A 0A 2A 2A 2A 2A (2A) so we're in sequence and for example immiedietly get PS and quite fast get RT */
	if(state < 6) {
		/* 0A */
		get_rds_ps_group(blocks);
		state++;
	} else if(state >= 6) {
		/* 2A */
		if(!get_rds_rt_group(blocks)) { /* try to generate 2A if can't generate 2A than that means our text is empty so we generate PS instead of wasting groups on nothing*/
			get_rds_ps_group(blocks);
		}
		state++;
	}
	if (state >= 12) state = 0;

group_coded:
	/* for version B groups. good for custom groups */
	if (IS_TYPE_B(blocks)) {
		blocks[2] = rds_data.pi;
	}
}

void get_rds_bits(uint8_t *bits) {
	static uint16_t out_blocks[GROUP_LENGTH];
	get_rds_group(out_blocks);
#ifdef RDS2
	add_checkwords(out_blocks, bits, false);
#else
	add_checkwords(out_blocks, bits);
#endif
}

void init_rds_encoder(struct rds_params_t rds_params) {

	/* AF */
	if (rds_params.af.num_afs) {
		set_rds_af(rds_params.af);
		fprintf(stderr, show_af_list(rds_params.af));
	}

	set_rds_pi(rds_params.pi);
	set_rds_ps(rds_params.ps);
	rds_state.rt_ab = 1;
	set_rds_rt1_enabled(1);
	set_rds_rt1(rds_params.rt1);
	set_rds_pty(rds_params.pty);
	rds_state.ptyn_ab = 1;
	set_rds_ptyn(rds_params.ptyn);
	set_rds_lps(rds_params.lps);
	set_rds_lpson(1);
	set_rds_tp(rds_params.tp);
	set_rds_ecc(rds_params.ecc);
	set_rds_lic(rds_params.lic);
	set_rds_ecclic_toggle(1);
	set_rds_pin_enabled(1);
	set_rds_ct(1);
	set_rds_ms(1);
	set_rds_di(DI_STEREO | DI_DPTY);

	/* Assign the RT+ AID to group 11A */
	#ifdef ODA_RTP
	init_rtplus(GROUP_11A);
	#endif

	/* initialize modulator objects */
	init_rds_objects();
#ifdef RDS2
	init_rds2_encoder(rds_params.rds2_image_path);
#endif
}

void exit_rds_encoder() {
	exit_rds_objects();
#ifdef RDS2
	exit_rds2_encoder();
#endif
}

void set_rds_pi(uint16_t pi_code) {
	rds_data.pi = pi_code;
}

void set_rds_ecc(uint8_t ecc) {
	rds_data.ecc = ecc; /* ecc is 8 bits, so a byte */
}

void set_rds_lic(uint8_t lic) {
	rds_data.lic = lic & INT16_L12; /* lic is 12 bits according to the docs, but the values can be less, for example english is 0x09 */
}

void set_rds_ecclic_toggle(uint8_t toggle) {
	rds_state.ecclic_enabled = toggle & INT8_0;
}

void set_rds_pin_enabled(uint8_t enabled) {
	rds_state.pin_enabled = enabled & INT8_0;
}

void set_rds_pin(uint8_t day, uint8_t hour, uint8_t minute) {
	rds_data.pin_day = (day & INT8_L5);
	rds_data.pin_hour = (hour & INT8_L5);
	rds_data.pin_minute = (minute & INT8_L6);
}

void set_rds_rt1_enabled(uint8_t rt1en) {
	rds_state.rt1_enabled = rt1en & INT8_0;
}
void set_rds_rt1(unsigned char *rt1) {
	uint8_t i = 0, len = 0;

	rds_state.rt_update = 1;
	memset(rds_data.rt1, ' ', RT_LENGTH);
	while (*rt1 != 0 && len < RT_LENGTH)
		rds_data.rt1[len++] = *rt1++;

	if (len < RT_LENGTH) {
		rds_state.rt_segments = 0;

		/* Terminate RT with '\r' (carriage return) if RT
		 * is < 64 characters long
		 */
		rds_data.rt1[len++] = '\r';

		/* find out how many segments are needed */
		while (i < len) {
			i += 4;
			rds_state.rt_segments++;
		}
	} else {
		/* Default to 16 if RT is 64 characters long */
		rds_state.rt_segments = 16;
	}
}

void set_rds_ps(unsigned char *ps) {
	uint8_t len = 0;

	rds_state.ps_update = 1;
	memset(rds_data.ps, ' ', PS_LENGTH);
	while (*ps != 0 && len < PS_LENGTH)
		rds_data.ps[len++] = *ps++;
}
void set_rds_tpson(uint8_t tpson) {
	rds_data.traffic_ps_on = tpson & INT8_0;
}
void set_rds_tps(unsigned char *tps) {
	uint8_t len = 0;

	rds_state.tps_update = 1;
	memset(rds_data.tps, ' ', PS_LENGTH);
	while (*tps != 0 && len < PS_LENGTH)
		rds_data.tps[len++] = *tps++;
}

void set_rds_lpson(uint8_t lpson) {
	rds_state.lps_on = lpson & INT8_0;
}

void set_rds_lps(unsigned char *lps) {
	uint8_t i = 0, len = 0;

	if (!lps[0]) {
		memset(rds_data.lps, 0, LPS_LENGTH);
		return;
	}
	rds_state.lps_update = 1;
	memset(rds_data.lps, '\r', LPS_LENGTH);
	while (*lps != 0 && len < LPS_LENGTH)
		rds_data.lps[len++] = *lps++;

	if (len < LPS_LENGTH) {
		rds_state.lps_segments = 0;

		/* increment to allow adding an '\r' in all cases */
		len++;

		/* find out how many segments are needed */
		while (i < len) {
			i += 4;
			rds_state.lps_segments++;
		}
	} else {
		/* default to 8 if LPS is 32 characters long */
		rds_state.lps_segments = 8;
	}
}

#ifdef ODA_RTP
void set_rds_rtplus_flags(uint8_t flags) {
	if(flags == 2) {
		rtplus_cfg.enabled = 0;
	} else {
		rtplus_cfg.enabled = 1;
	}
	rtplus_cfg.running	= flags & INT8_0;
}

void set_rds_rtplus_tags(uint8_t *tags) {
	rtplus_cfg.type[0]	= tags[0] & INT8_L6;
	rtplus_cfg.start[0]	= tags[1] & INT8_L6;
	rtplus_cfg.len[0]	= tags[2] & INT8_L6;
	rtplus_cfg.type[1]	= tags[3] & INT8_L6;
	rtplus_cfg.start[1]	= tags[4] & INT8_L6;
	rtplus_cfg.len[1]	= tags[5] & INT8_L5;

	rtplus_cfg.toggle ^= 1;
	rtplus_cfg.running = 1;
}
#endif

void set_rds_af(struct rds_af_t new_af_list) {
	memcpy(&rds_data.af, &new_af_list, sizeof(struct rds_af_t));
}

void clear_rds_af() {
	memset(&rds_data.af, 0, sizeof(struct rds_af_t));
}

void set_rds_pty(uint8_t pty) {
	rds_data.pty = pty & INT8_L5;
}

void set_rds_ptyn_enabled(uint8_t enabled) {
	rds_state.ptyn_enabled = enabled & INT8_0;
}

void set_rds_ptyn(unsigned char *ptyn) {
	uint8_t len = 0;

	if (!ptyn[0]) {
		memset(rds_data.ptyn, 0, PTYN_LENGTH);
		return;
	}

	rds_state.ptyn_update = 1;
	memset(rds_data.ptyn, ' ', PTYN_LENGTH);
	while (*ptyn != 0 && len < PTYN_LENGTH)
		rds_data.ptyn[len++] = *ptyn++;
}

void set_rds_ta(uint8_t ta) {
	rds_data.ta = ta & INT8_0;
}

void set_rds_tp(uint8_t tp) {
	rds_data.tp = tp & INT8_0;
}

void set_rds_ms(uint8_t ms) {
	rds_data.ms = ms & INT8_0;
}

void set_rds_di(uint8_t di) {
	rds_data.di = di & INT8_L4;
}

void set_rds_ct(uint8_t ct) {
	rds_data.ct = ct & INT8_0;
}

uint16_t get_rds_pi() {
	/* this is for custom groups, specifically 'G=' */
	return rds_data.pi;
}

void set_rds_cg(uint16_t* blocks) {
	rds_state.custom_group[0] = blocks[0];
	rds_state.custom_group[1] = blocks[1];
	rds_state.custom_group[2] = blocks[2];
	rds_state.custom_group[3] = blocks[3];
	rds_state.custom_group_in = 1;
}
