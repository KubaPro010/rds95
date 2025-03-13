#include "common.h"
#include "rds.h"
#include "modulator.h"
#include "lib.h"
#include <time.h>

static struct rds_params_t rds_data;

// #region Struct Defs
static struct {
	uint8_t ecclic_enabled;
	uint8_t ecc_or_lic;
	uint8_t pin_enabled;

	uint8_t ps_update;
	uint8_t tps_update;

	uint8_t rt1_enabled;
	uint8_t rt_update;
	uint8_t rt_ab;
	uint8_t rt_segments;

	uint8_t ptyn_enabled;
	uint8_t ptyn_update;
	uint8_t ptyn_ab;

	uint8_t lps_update;
	uint8_t lps_segments;

	uint16_t custom_group[GROUP_LENGTH];
} rds_state;

// #region ODA
static struct rds_oda_t odas[MAX_ODAS];
static struct {
	uint8_t current;
	uint8_t count;
} oda_state;

static struct {
	uint8_t group;
	uint8_t enabled;
	uint8_t running;
	uint8_t toggle;
	uint8_t type[2];
	uint8_t start[2];
	uint8_t len[2];
} rtplus_cfg;
// #endregion
// #endregion

// #region Helper funcs
static void register_oda(uint8_t group, uint16_t aid, uint16_t scb) {
	if (oda_state.count >= MAX_ODAS) return;

	odas[oda_state.count].group = group;
	odas[oda_state.count].aid = aid;
	odas[oda_state.count].scb = scb;
	oda_state.count++;
}

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
// #endregion

// #region Group encoding
static void get_rds_ps_group(uint16_t *blocks) {
	static unsigned char ps_text[PS_LENGTH];
	static unsigned char tps_text[PS_LENGTH];
	static uint8_t ps_csegment;

	if (ps_csegment == 0 && rds_state.ps_update) {
		memcpy(ps_text, rds_data.ps, PS_LENGTH);
		rds_state.ps_update = 0;
	}
	if(ps_csegment == 0 && rds_state.tps_update) {
		memcpy(tps_text, rds_data.tps, PS_LENGTH);
		rds_state.tps_update = 0;
	}

	blocks[1] |= rds_data.ta << 4;
	blocks[1] |= rds_data.ms << 3;
	blocks[1] |= ((rds_data.di >> (3 - ps_csegment)) & INT8_0) << 2;
	blocks[1] |= ps_csegment;
	blocks[2] = get_next_af();
	if(rds_data.ta && tps_text[0] != '\0') {
		blocks[3] = tps_text[ps_csegment * 2] << 8 | tps_text[ps_csegment * 2 + 1];
	} else {
		/* TODO: Add DPS */
		blocks[3] =  ps_text[ps_csegment * 2] << 8 |  ps_text[ps_csegment * 2 + 1];
	}
	ps_csegment++;
	if (ps_csegment >= 4) ps_csegment = 0;
}

static uint8_t get_rds_rt_group(uint16_t *blocks) {
	static unsigned char rt_text[RT_LENGTH];
	static uint8_t rt_state;

	if (rds_state.rt_update) {
		memcpy(rt_text, rds_data.rt1, RT_LENGTH);
		rds_state.rt_ab ^= 1;
		rds_state.rt_update = 0;
		rt_state = 0;
	}

	if(!rds_state.rt1_enabled) {
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

static void get_rds_oda_group(uint16_t *blocks) {
	struct rds_oda_t this_oda = odas[oda_state.current];

	if(this_oda.aid == ODA_AID_RTPLUS && rtplus_cfg.enabled == 0) {
		return;
	}

	blocks[1] |= 3 << 12;

	blocks[1] |= GET_GROUP_TYPE(this_oda.group) << 1;
	blocks[1] |= GET_GROUP_VER(this_oda.group);
	blocks[2] = this_oda.scb;
	blocks[3] = this_oda.aid;

	oda_state.current++;
	if (oda_state.current >= oda_state.count) oda_state.current = 0;
}

static uint8_t get_rds_ct_group(uint16_t *blocks) {
	static uint8_t latest_minutes;
	struct tm *utc, *local_time;
	time_t now;
	uint8_t l;
	uint32_t mjd;
	int16_t offset;

	now = time(NULL);
	utc = gmtime(&now);

	if (utc->tm_min != latest_minutes) {
		latest_minutes = utc->tm_min;

		l = utc->tm_mon <= 1 ? 1 : 0;
		mjd = 14956 + utc->tm_mday +
			(uint32_t)((utc->tm_year - l) * 365.25f) +
			(uint32_t)((utc->tm_mon + (1+1) + l * 12) * 30.6001f);

		blocks[1] |= 4 << 12 | (mjd >> 15);
		blocks[2] = (mjd << 1) | (utc->tm_hour >> 4);
		blocks[3] = (utc->tm_hour & INT16_L4) << 12 | utc->tm_min << 6;

		local_time = localtime(&now);

		offset = local_time->__tm_gmtoff / (30 * 60);
		if (offset < 0) blocks[3] |= 1 << 5;
		blocks[3] |= abs(offset);

		return 1;
	}

	return 0;
}

static void get_rds_ptyn_group(uint16_t *blocks) {
	static unsigned char ptyn_text[PTYN_LENGTH];
	static uint8_t ptyn_state;

	if (ptyn_state == 0 && rds_state.ptyn_update) {
		memcpy(ptyn_text, rds_data.ptyn, PTYN_LENGTH);
		rds_state.ptyn_ab ^= 1;
		rds_state.ptyn_update = 0;
	}

	blocks[1] |= 10 << 12 | ptyn_state;
	blocks[1] |= rds_state.ptyn_ab << 4;
	blocks[2] =  ptyn_text[ptyn_state * 4 + 0] << 8;
	blocks[2] |= ptyn_text[ptyn_state * 4 + 1];
	blocks[3] =  ptyn_text[ptyn_state * 4 + 2] << 8;
	blocks[3] |= ptyn_text[ptyn_state * 4 + 3];

	ptyn_state++;
	if (ptyn_state == 2) ptyn_state = 0;
}

static void get_rds_lps_group(uint16_t *blocks) {
	static unsigned char lps_text[LPS_LENGTH];
	static uint8_t lps_state;

	if (lps_state == 0 && rds_state.lps_update) {
		memcpy(lps_text, rds_data.lps, LPS_LENGTH);
		rds_state.lps_update = 0;
	}

	blocks[1] |= 15 << 12 | lps_state;
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
		blocks[3] = rds_data.pin_day << 11;
		blocks[3] |= rds_data.pin_hour << 6;
		blocks[3] |= rds_data.pin_minute;
	}
}

static void get_rds_lic_group(uint16_t *blocks) {
	blocks[1] |= 1 << 12;
	if(rds_state.ecclic_enabled) {
		blocks[2] = 0x3000; // 0b0011000000000000
		blocks[2] |= rds_data.lic;
	}

	if(rds_state.pin_enabled) {
		blocks[3] = rds_data.pin_day << 11;
		blocks[3] |= rds_data.pin_hour << 6;
		blocks[3] |= rds_data.pin_minute;
	}
}
static void get_rds_rtplus_group(uint16_t *blocks) {
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
// #endregion

static uint8_t get_rds_other_groups(uint16_t *blocks) {
	static uint8_t group_counter[GROUP_15B];

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

	if (oda_state.count) {
		if (++group_counter[GROUP_3A] >= 25) {
			group_counter[GROUP_3A] = 0;
			get_rds_oda_group(blocks);
			return 1;
		}
	}

	if (rds_state.ptyn_enabled && rds_data.ptyn[0]) {
		if (++group_counter[GROUP_10A] >= 13) {
			group_counter[GROUP_10A] = 0;
			get_rds_ptyn_group(blocks);
			return 1;
		}
	}

	if (++group_counter[rtplus_cfg.group] >= 30) {
		group_counter[rtplus_cfg.group] = 0;
		if(rtplus_cfg.enabled == 0) {
			return 0;
		}
		get_rds_rtplus_group(blocks);
		return 1;
	}

	return 0;
}

static uint8_t get_rds_long_text_groups(uint16_t *blocks) {
	static uint8_t group_selector = 0;

	if (group_selector == 4 && rds_data.lps[0]) {
		get_rds_lps_group(blocks);
		goto group_coded;
	}

	if (++group_selector >= 8) group_selector = 0;
	return 0;

group_coded:
	if (++group_selector >= 8) group_selector = 0;
	return 1;
}

static uint8_t get_rds_custom_groups(uint16_t *blocks) {
	if(rds_state.custom_group[0] == 1) {
		rds_state.custom_group[0] = 0;
		blocks[0] = rds_data.pi;
		blocks[1] = rds_state.custom_group[1];
		blocks[2] = rds_state.custom_group[2];
		blocks[3] = rds_state.custom_group[3];
		return 1;
	}
	return 0;
}

static void get_rds_group(uint16_t *blocks) {
	static uint8_t state;

	blocks[0] = rds_data.pi;
	blocks[1] = rds_data.tp << 10;
	blocks[1] |= rds_data.pty << 5;
	blocks[2] = 0;
	blocks[3] = 0;

	if (rds_data.ct && get_rds_ct_group(blocks)) {
		goto group_coded;
	}

	if(get_rds_custom_groups(blocks)) {
		goto group_coded;
	}

	if (get_rds_long_text_groups(blocks)) {
		goto group_coded;
	}
	if (get_rds_other_groups(blocks)) {
		goto group_coded;
	}

	if(state < 6) {
		get_rds_ps_group(blocks);
		state++;
	} else if(state >= 6) {
		if(!get_rds_rt_group(blocks)) {
			get_rds_ps_group(blocks);
		}
		state++;
	}
	if (state >= 12) state = 0;

group_coded:
	if (IS_TYPE_B(blocks)) {
		blocks[2] = rds_data.pi;
	}
}

void get_rds_bits(uint8_t *bits) {
	static uint16_t out_blocks[GROUP_LENGTH];
	get_rds_group(out_blocks);
	add_checkwords(out_blocks, bits);
}

static void init_rtplus(uint8_t group) {
	register_oda(group, ODA_AID_RTPLUS, 0);
	rtplus_cfg.group = group;
	rtplus_cfg.enabled = 0;
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
	set_rds_tp(rds_params.tp);
	set_rds_ecc(rds_params.ecc);
	set_rds_lic(rds_params.lic);
	set_rds_ecclic_toggle(1);
	set_rds_pin_enabled(1);
	set_rds_ct(1);
	set_rds_ms(1);
	set_rds_di(DI_STEREO | DI_DPTY);

	init_rtplus(GROUP_11A);

	init_rds_objects();
}

void set_rds_pi(uint16_t pi_code) {
	rds_data.pi = pi_code;
}

void set_rds_ecc(uint8_t ecc) {
	rds_data.ecc = ecc;
}

void set_rds_lic(uint8_t lic) {
	rds_data.lic = lic & INT16_L12;
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

		rds_data.rt1[len++] = '\r';

		while (i < len) {
			i += 4;
			rds_state.rt_segments++;
		}
	} else {
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
void set_rds_tps(unsigned char *tps) {
	uint8_t len = 0;

	if(tps[0] == '\0') {
		rds_state.tps_update = 1;
		memset(rds_data.tps, 0, PS_LENGTH);
		return;
	}

	rds_state.tps_update = 1;
	memset(rds_data.tps, ' ', PS_LENGTH);
	while (*tps != 0 && len < PS_LENGTH)
		rds_data.tps[len++] = *tps++;
}

void set_rds_lps(unsigned char *lps) {
	uint8_t i = 0, len = 0;

	if(lps[0] == '\0') {
		rds_state.lps_update = 1;
		memset(rds_data.lps, 0, LPS_LENGTH);
		return;
	}
	rds_state.lps_update = 1;
	memset(rds_data.lps, '\r', LPS_LENGTH);
	while (*lps != 0 && len < LPS_LENGTH)
		rds_data.lps[len++] = *lps++;

	if (len < LPS_LENGTH) {
		rds_state.lps_segments = 0;

		len++;

		while (i < len) {
			i += 4;
			rds_state.lps_segments++;
		}
	} else {
		rds_state.lps_segments = 8;
	}
}

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
	rtplus_cfg.enabled = 1;
}

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

void set_rds_cg(uint16_t* blocks) {
	rds_state.custom_group[0] = 1;
	rds_state.custom_group[1] = blocks[1];
	rds_state.custom_group[2] = blocks[2];
	rds_state.custom_group[3] = blocks[3];
}
