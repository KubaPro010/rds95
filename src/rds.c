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

	uint8_t rtp_oda;
	uint8_t grp_seq_idx[2];
	uint8_t udg_idxs[2];
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
	blocks[2] = rds_data.ecc;

	if(rds_data.pin[0]) {
		blocks[3] = rds_data.pin[1] << 11; // day
		blocks[3] |= rds_data.pin[2] << 6; // hour
		blocks[3] |= rds_data.pin[3]; // minute
	}
}

static void get_rds_lic_group(uint16_t *blocks) {
	blocks[1] |= 1 << 12;
	blocks[2] = 0x3000; // 0b0011000000000000
	blocks[2] |= rds_data.lic;

	if(rds_data.pin[0]) {
		blocks[3] = rds_data.pin[1] << 11; // day
		blocks[3] |= rds_data.pin[2] << 6; // hour
		blocks[3] |= rds_data.pin[3]; // minute
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

	uint8_t good_group = 0;
	uint8_t cant_find_group = 0;
	char grp;

	while(good_group == 0) {
		uint8_t grp_sqc_idx = rds_state.grp_seq_idx[0]++;
		if(rds_data.grp_sqc[grp_sqc_idx] == '\0') {
			rds_state.grp_seq_idx[0] = 0;
			grp_sqc_idx = 0;
		}
		grp = rds_data.grp_sqc[grp_sqc_idx];

		if(grp == '0') good_group = 1;
		if(grp == '1' && rds_state.ecclic_enabled) good_group = 1;
		if(grp == '2' && rds_state.rt1_enabled) good_group = 1;
		if(grp == 'A' && rds_state.ptyn_enabled) good_group = 1;
		if(grp == 'R' && rtplus_cfg.enabled) good_group = 1;
		if(grp == '3' && oda_state.count != 0) good_group = 1;
		if(grp == 'F' && rds_data.lps[0] != '\0') good_group = 1;

		if(!good_group) cant_find_group++;
		else cant_find_group = 0;
		if(!good_group && cant_find_group == 23) {
			cant_find_group = 0;
			return;
			break;
		}
	}

	switch (grp)
	{
		default:
		case '0':
			if(rds_state.grp_seq_idx[1] != 3) rds_state.grp_seq_idx[0]--;
			else rds_state.grp_seq_idx[1] = 0;
			rds_state.grp_seq_idx[1]++;
			get_rds_ps_group(blocks);
			goto group_coded;
		case '1':
			if(rds_data.ecc && rds_data.lic) {
				if(rds_state.ecc_or_lic == 0) {
					get_rds_ecc_group(blocks);
				} else {
					get_rds_lic_group(blocks);
				}
				rds_state.ecc_or_lic ^= 1;
			} else if(rds_data.lic) {
				get_rds_lic_group(blocks);
			} else {
				get_rds_ecc_group(blocks);
			}
			goto group_coded;
		case '2':
			get_rds_rt_group(blocks);
			goto group_coded;
		case 'A':
			get_rds_ptyn_group(blocks);
			goto group_coded;
		// TODO: Add EON
		case 'X':
			uint16_t blocks[3] = rds_data.udg1[rds_state.udg_idxs[0]++];
			blocks[1] |= blocks[0];
			blocks[2] = blocks[2];
			blocks[3] = blocks[3];
			goto group_coded;
		case 'Y':
			uint16_t blocks[3] = rds_data.udg2[rds_state.udg_idxs[1]++];
			blocks[1] |= blocks[0];
			blocks[2] = blocks[2];
			blocks[3] = blocks[3];
			goto group_coded;
		case 'R':
			if(rds_state.rtp_oda == 0) {
				get_rds_rtplus_group(blocks);
			} else {
				get_rds_oda_group(blocks);
			}
			rds_state.rtp_oda ^= 1;
			goto group_coded;
		// TODO: add uecp
		case '3':
			get_rds_oda_group(blocks);
			goto group_coded;
		case 'F':
			get_rds_lps_group(blocks);
			goto group_coded;
	}


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
	set_rds_grpseq(rds_params.grp_sqc);
	set_rds_udg1(rds_params.udg1);
	set_rds_udg2(rds_params.udg2);

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
	rds_data.pin[0] = enabled & 1;
}

void set_rds_pin(uint8_t day, uint8_t hour, uint8_t minute) {
	rds_data.pin[1] = (day & INT8_L5);
	rds_data.pin[2] = (hour & INT8_L5);
	rds_data.pin[3] = (minute & INT8_L6);
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

	rds_state.tps_update = 1;
	if(tps[0] == '\0') {
		memset(rds_data.tps, 0, PS_LENGTH);
		return;
	}

	memset(rds_data.tps, ' ', PS_LENGTH);
	while (*tps != 0 && len < PS_LENGTH)
		rds_data.tps[len++] = *tps++;
}

void set_rds_lps(unsigned char *lps) {
	uint8_t i = 0, len = 0;

	rds_state.lps_update = 1;
	if(lps[0] == '\0') {
		memset(rds_data.lps, 0, LPS_LENGTH);
		return;
	}
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
	rtplus_cfg.enabled = (flags==2);
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
	rds_data.pty = pty & 31;
}

void set_rds_ptyn_enabled(uint8_t enabled) {
	rds_state.ptyn_enabled = enabled & 1;
}

void set_rds_ptyn(unsigned char *ptyn) {
	uint8_t len = 0;

	rds_state.ptyn_update = 1;

	if(ptyn[0] == '\0') {
		memset(rds_data.ptyn, 0, PTYN_LENGTH);
		return;
	}

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
	rds_state.custom_group[1] = blocks[0];
	rds_state.custom_group[2] = blocks[1];
	rds_state.custom_group[3] = blocks[2];
}

void set_rds_grpseq(unsigned char* grpseq) {
	uint8_t len = 0;
	memset(rds_data.grp_sqc, ' ', 24);
	while (*grpseq != 0 && len < 24)
		rds_data.grp_sqc[len++] = *grpseq++;
}

void set_rds_udg1(uint16_t** groups) {
	memcpy(&rds_data.udg1, &groups, sizeof(groups));
}

void set_rds_udg2(uint16_t** groups) {
	memcpy(&rds_data.udg2, &groups, sizeof(groups));
}