#ifndef RDS_H
#define RDS_H

/* The RDS error-detection code generator polynomial is
 * x^10 + x^8 + x^7 + x^5 + x^4 + x^3 + x^0
 */
#define POLY			0x1B9
#define POLY_DEG		10
#define BLOCK_SIZE		16

#define GROUP_LENGTH		4
#define BITS_PER_GROUP		(GROUP_LENGTH * (BLOCK_SIZE + POLY_DEG))
#define RDS_SAMPLE_RATE		19000
#define SAMPLES_PER_BIT     16
#define FILTER_SIZE	 64
#define SAMPLE_BUFFER_SIZE	(SAMPLES_PER_BIT + FILTER_SIZE)

#define STREAMS 2

#define RT_LENGTH	64
#define ERT_LENGTH	128
#define PS_LENGTH	8
#define PTYN_LENGTH	8
#define LPS_LENGTH	32
#define DEFAULT_GRPSQC "002222XY"
#define MAX_AFS 25

#define AF_CODE_FILLER		205
#define AF_CODE_NUM_AFS_BASE	224
#define AF_CODE_LFMF_FOLLOWS	250

#define PROGRAMS 2

// List of ODAs: https://www.nrscstandards.org/committees/dsm/archive/rds-oda-aids.pdf
#define	ODA_AID_RTPLUS	0x4bd7
#define	ODA_AID_ERT		0x6552
#define	ODA_AID_ERTPLUS	0x4BD8
#define ODA_AID_ODAAF	0x6365

typedef struct {
	uint8_t num_entries : 6;
	uint8_t num_afs : 5;
	uint16_t afs[MAX_AFS]; // 9 bit, there was no uint9_t
} RDSAFsODA;
typedef struct {
	uint8_t num_entries : 6;
	uint8_t num_afs : 5;
	uint8_t afs[MAX_AFS];
} RDSAFs;
typedef struct {
	uint16_t pi;
	uint8_t enabled : 1;
	uint8_t ta : 1;
	uint8_t tp : 1;
	uint8_t pty : 5;
	char ps[8];
	RDSAFs af;
	uint16_t data : 16;
} RDSEON;
typedef struct {
	uint16_t pi;

	char ps[PS_LENGTH];
	char rt1[RT_LENGTH];

	uint8_t ecc;
	uint16_t slc_data : 12;

	uint8_t ta : 1;
	uint8_t pty : 5;
	uint8_t tp : 1;
	uint8_t dpty : 1;

	char tps[PS_LENGTH];

	uint8_t rt1_enabled : 1;
	uint8_t rt2_enabled : 1;
	uint8_t rt_type : 2;
	uint8_t rt_text_timeout;
	uint8_t original_rt_text_timeout;
	uint8_t rt_switching_period;
	uint8_t orignal_rt_switching_period;
	char default_rt[RT_LENGTH];
	char rt2[RT_LENGTH];

	uint8_t ert_switching_period;
	uint8_t orignal_ert_switching_period;
	char ert[ERT_LENGTH];

	uint8_t ptyn_enabled : 1;
	char ptyn[PTYN_LENGTH];

	RDSAFs af;
	RDSAFsODA af_oda;

	uint8_t ct : 1;

	char lps[LPS_LENGTH];

	char grp_sqc[24];
	char grp_sqc_rds2[24];

	uint8_t udg1_len : 4;
	uint8_t udg2_len : 4;

	uint16_t udg1[8][3];
	uint16_t udg2[8][3];

	RDSEON eon[4];
} RDSData;
typedef struct {
	uint8_t af_state : 6;
} RDSEONState;
typedef struct {
	uint8_t ps_update : 1;
	uint8_t tps_update : 1;
	char ps_text[PS_LENGTH];
	char tps_text[PS_LENGTH];
	uint8_t ps_csegment : 3;

	char rt_text[RT_LENGTH];
	uint8_t rt_state : 5;
	uint8_t rt_update : 1;
	uint8_t rt2_update : 1;
	uint8_t rt_ab : 1;
	uint8_t rt_segments : 5;
	uint8_t rt2_segments : 5;
	uint8_t current_rt : 1;

	char ert_text[ERT_LENGTH];
	uint8_t ert_state : 6;
	uint8_t ert_update : 1;
	uint8_t ert_segments : 6;

	char ptyn_text[PTYN_LENGTH];
	uint8_t ptyn_state : 1;
	uint8_t ptyn_update : 1;
	uint8_t ptyn_ab : 1;

	char lps_text[LPS_LENGTH];
	uint8_t lps_state : 4;
	uint8_t lps_update : 1;
	uint8_t lps_segments : 4;

	uint16_t custom_group[GROUP_LENGTH];
	uint16_t custom_group2[GROUP_LENGTH + 1];

	uint8_t rtp_oda : 1;
	uint8_t ertp_oda : 1;
	uint8_t ert_oda : 1;
	uint8_t af_oda : 1;
	uint8_t data_ecc : 1;
	uint8_t grp_seq_idx[4];
	uint8_t udg_idxs[2];

	uint8_t fasttuning_state : 3;

	uint8_t last_minute : 6;
	uint8_t ta_timeout : 7;
	uint8_t original_ta_timeout : 7;

	uint8_t eon_index : 3;
	uint8_t eon_state : 4;
	RDSEONState eon_states[4];

	uint8_t af_state : 6;
	uint8_t af_oda_state : 6;

	uint8_t eon_linkage : 1;

	uint16_t last_stream0_group[3];
} RDSState;

typedef struct {
	uint8_t enabled : 1;
	uint8_t running : 1;
	uint8_t type[2];
	uint8_t start[2];
	uint8_t len[2];
} RDSRTPlusData;
typedef struct {
	uint8_t toggle : 1;
} RDSRTPlusState;

typedef struct
{
	uint8_t expected_encoder_addr;
	uint16_t expected_site_addr : 10;
} RDSEncoderASCIIData;
typedef struct
{
	uint8_t uecp_enabled : 1;
} RDSEncoderUECPData;
typedef struct {
	uint8_t encoder_addr[2];
	uint16_t site_addr[2];
	RDSEncoderASCIIData ascii_data;
	RDSEncoderUECPData uecp_data;
	uint16_t special_features;
	uint8_t rds2_mode : 1;
	// uint8_t rds2_buffer[16384];
} RDSEncoderData;
typedef struct {
	RDSEncoderData encoder_data;
	RDSData data[PROGRAMS];
	RDSState state[PROGRAMS];
	RDSRTPlusData rtpData[PROGRAMS][2];
	RDSRTPlusState rtpState[PROGRAMS][2];
	uint8_t program : 3;
} RDSEncoder;
typedef struct {
	uint8_t file_starter; // Always is 225 first polish radio programme am frequency
	RDSData data[PROGRAMS];
	RDSRTPlusData rtpData[PROGRAMS][2];
	uint8_t file_middle; // Always is 160, average of both
	RDSEncoderData encoder_data;
	uint8_t program : 3;
	uint8_t file_ender; // Always is 95 my freq
	uint16_t crc;
} RDSEncoderFile;

typedef struct
{
	uint16_t a;
	uint16_t b;
	uint16_t c;
	uint16_t d;
	uint8_t is_type_b : 1;
} RDSGroup;

#define IS_TYPE_B(b)	(b & 0x0800)

void saveToFile(RDSEncoder *emp, const char *option);
void loadFromFile(RDSEncoder *emp);
int rdssaved();

void reset_rds_state(RDSEncoder* enc, uint8_t program);
void set_rds_defaults(RDSEncoder* enc, uint8_t program);
void init_rds_encoder(RDSEncoder* enc);
void get_rds_bits(RDSEncoder* enc, uint8_t *bits, uint8_t stream);
void set_rds_rt1(RDSEncoder* enc, char *rt1);
void set_rds_rt2(RDSEncoder* enc, char *rt2);
void set_rds_ps(RDSEncoder* enc, char *ps);
void set_rds_tps(RDSEncoder* enc, char *tps);
void set_rds_lps(RDSEncoder* enc, char *lps);
void set_rds_ert(RDSEncoder *enc, char *ert);
void set_rds_rtplus_flags(RDSEncoder *enc, uint8_t flags);
void set_rds_ertplus_flags(RDSEncoder *enc, uint8_t flags);
void set_rds_rtplus_tags(RDSEncoder *enc, uint8_t *tags);
void set_rds_ertplus_tags(RDSEncoder *enc, uint8_t *tags);
void set_rds_ptyn(RDSEncoder *enc, char *ptyn);
void set_rds_grpseq(RDSEncoder* enc, char *grpseq);
void set_rds_grpseq2(RDSEncoder* enc, char *grpseq2);

#endif
