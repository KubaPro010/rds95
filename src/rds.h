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
#define FILTER_SIZE	 32
#define SAMPLE_BUFFER_SIZE	(SAMPLES_PER_BIT + FILTER_SIZE)

#define RT_LENGTH	64
#define PS_LENGTH	8
#define PTYN_LENGTH	8
#define LPS_LENGTH	32
#define DEFAULT_GRPSQC "002222XY"
#define MAX_AFS 25

#define AF_CODE_FILLER		205
#define AF_CODE_NO_AF		224
#define AF_CODE_NUM_AFS_BASE	AF_CODE_NO_AF
#define AF_CODE_LFMF_FOLLOWS	250

#define PROGRAMS 2
#define EONS 4

#define MAX_ODAS	8
// List of ODAs: https://www.nrscstandards.org/committees/dsm/archive/rds-oda-aids.pdf
#define	ODA_AID_RTPLUS	0x4bd7

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
} RDSEON;
typedef struct
{
	uint8_t days : 7; // let's say that here it will be stored by bits, so 0b1000000 is monday and so on
	uint8_t pty: 5;
	uint16_t execution_hours[12];
	uint16_t execution_minutes[12];
	char command[35];
} RDSSchedulerItem;
typedef struct
{
	uint8_t enabled : 1;
	RDSSchedulerItem items[49];
} RDSScheduler;
typedef struct {
	uint16_t pi;

	char ps[PS_LENGTH];
	char rt1[RT_LENGTH];

	uint8_t dsn;
	uint8_t psn;

	uint8_t ecc;

	uint8_t ta : 1;
	uint8_t pty : 5;
	uint8_t tp : 1;
	uint8_t dpty : 1;

	char tps[PS_LENGTH];

	uint8_t rt1_enabled : 1;
	uint8_t rt2_enabled : 1;
	uint8_t rt_type : 2;
	uint8_t rt_text_timeout;
	uint8_t rt_switching_period;
	uint8_t orignal_rt_switching_period;
	char default_rt[RT_LENGTH];
	char rt2[RT_LENGTH];

	uint8_t ptyn_enabled : 1;
	char ptyn[PTYN_LENGTH];

	uint8_t af_method : 1;
	RDSAFs af;

	uint8_t ct : 1;

	char lps[LPS_LENGTH];

	char grp_sqc[24];
	char grp_sqc_rds2[24];

	uint8_t udg1_len : 4;
	uint8_t udg2_len : 4;

	uint16_t udg1[8][3];
	uint16_t udg2[8][3];

	RDSEON eon[EONS];
} RDSData;
typedef struct {
	uint8_t ps_update : 1;
	uint8_t tps_update : 1;
	char ps_text[PS_LENGTH];
	char tps_text[PS_LENGTH];
	uint8_t ps_csegment : 4;

	char rt_text[RT_LENGTH];
	uint8_t rt_state : 5;
	uint8_t rt_update : 1;
	uint8_t rt2_update : 1;
	uint8_t rt_ab : 1;
	uint8_t rt_segments : 5;
	uint8_t rt2_segments : 5;
	uint8_t current_rt : 1;

	char ptyn_text[PTYN_LENGTH];
	uint8_t ptyn_state : 1;
	uint8_t ptyn_update : 1;
	uint8_t ptyn_ab : 1;

	char lps_text[LPS_LENGTH];
	uint8_t lps_state : 5;
	uint8_t lps_update : 1;
	uint8_t lps_segments : 5;

	uint16_t custom_group[GROUP_LENGTH];
	uint16_t custom_group2[GROUP_LENGTH + 1];

	uint8_t rtp_oda : 1;
	uint8_t grp_seq_idx[4];
	uint8_t udg_idxs[2];

	uint8_t last_minute : 6;
	uint8_t ta_timeout : 7;
	uint8_t original_ta_timeout : 7;

	uint8_t eon_index : 3;
	uint8_t eon_state : 4;

	uint16_t last_stream0_group[3];
} RDSState;

typedef struct {
	uint8_t group;
	uint16_t aid;
	uint16_t scb;
} RDSODA;
typedef struct {
	uint8_t current : 4;
	uint8_t count : 4;
} RDSODAState;

typedef struct {
	uint8_t group;
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
	RDSScheduler schedule;
	uint8_t rds2_mode : 1;
	// uint8_t rds2_buffer[16384];
} RDSEncoderData;
typedef struct {
	RDSEncoderData encoder_data;
	RDSData data[PROGRAMS];
	RDSState state[PROGRAMS];
	RDSODA odas[PROGRAMS][MAX_ODAS];
	RDSODAState oda_state[PROGRAMS];
	RDSRTPlusData rtpData[PROGRAMS];
	RDSRTPlusState rtpState[PROGRAMS];
	uint8_t program : 3;
} RDSEncoder;
typedef struct {
	uint8_t file_starter; // Always is 225 first polish radio programme am frequency
	RDSData data[PROGRAMS];
	RDSRTPlusData rtpData[PROGRAMS];
	RDSODA odas[PROGRAMS][MAX_ODAS];
	uint8_t file_middle; // Always is 160, average of both
	RDSODAState oda_state[PROGRAMS];
	RDSEncoderData encoder_data;
	uint8_t program : 3;
	uint8_t file_ender; // Always is 95 my freq
} RDSEncoderFile;

#define GROUP_TYPE_0	( 0 << 4)
#define GROUP_TYPE_1	( 1 << 4)
#define GROUP_TYPE_2	( 2 << 4)
#define GROUP_TYPE_3	( 3 << 4)
#define GROUP_TYPE_4	( 4 << 4)
#define GROUP_TYPE_5	( 5 << 4)
#define GROUP_TYPE_6	( 6 << 4)
#define GROUP_TYPE_7	( 7 << 4)
#define GROUP_TYPE_8	( 8 << 4)
#define GROUP_TYPE_9	( 9 << 4)
#define GROUP_TYPE_10	(10 << 4)
#define GROUP_TYPE_11	(11 << 4)
#define GROUP_TYPE_12	(12 << 4)
#define GROUP_TYPE_13	(13 << 4)
#define GROUP_TYPE_14	(14 << 4)
#define GROUP_TYPE_15	(15 << 4)

#define GROUP_VER_A	0
#define GROUP_VER_B	1

#define GROUP_0A	(GROUP_TYPE_0  | GROUP_VER_A)
#define GROUP_1A	(GROUP_TYPE_1  | GROUP_VER_A)
#define GROUP_2A	(GROUP_TYPE_2  | GROUP_VER_A)
#define GROUP_3A	(GROUP_TYPE_3  | GROUP_VER_A)
#define GROUP_4A	(GROUP_TYPE_4  | GROUP_VER_A)
#define GROUP_5A	(GROUP_TYPE_5  | GROUP_VER_A)
#define GROUP_6A	(GROUP_TYPE_6  | GROUP_VER_A)
#define GROUP_7A	(GROUP_TYPE_7  | GROUP_VER_A)
#define GROUP_8A	(GROUP_TYPE_8  | GROUP_VER_A)
#define GROUP_9A	(GROUP_TYPE_9  | GROUP_VER_A)
#define GROUP_10A	(GROUP_TYPE_10 | GROUP_VER_A)
#define GROUP_11A	(GROUP_TYPE_11 | GROUP_VER_A)
#define GROUP_12A	(GROUP_TYPE_12 | GROUP_VER_A)
#define GROUP_13A	(GROUP_TYPE_13 | GROUP_VER_A)
#define GROUP_14A	(GROUP_TYPE_14 | GROUP_VER_A)
#define GROUP_15A	(GROUP_TYPE_15 | GROUP_VER_A)

#define GROUP_0B	(GROUP_TYPE_0  | GROUP_VER_B)
#define GROUP_1B	(GROUP_TYPE_1  | GROUP_VER_B)
#define GROUP_2B	(GROUP_TYPE_2  | GROUP_VER_B)
#define GROUP_3B	(GROUP_TYPE_3  | GROUP_VER_B)
#define GROUP_4B	(GROUP_TYPE_4  | GROUP_VER_B)
#define GROUP_5B	(GROUP_TYPE_5  | GROUP_VER_B)
#define GROUP_6B	(GROUP_TYPE_6  | GROUP_VER_B)
#define GROUP_7B	(GROUP_TYPE_7  | GROUP_VER_B)
#define GROUP_8B	(GROUP_TYPE_8  | GROUP_VER_B)
#define GROUP_9B	(GROUP_TYPE_9  | GROUP_VER_B)
#define GROUP_10B	(GROUP_TYPE_10 | GROUP_VER_B)
#define GROUP_11B	(GROUP_TYPE_11 | GROUP_VER_B)
#define GROUP_12B	(GROUP_TYPE_12 | GROUP_VER_B)
#define GROUP_13B	(GROUP_TYPE_13 | GROUP_VER_B)
#define GROUP_14B	(GROUP_TYPE_14 | GROUP_VER_B)
#define GROUP_15B	(GROUP_TYPE_15 | GROUP_VER_B)

#define GET_GROUP_TYPE(x)	((x >> 4) & 15)
#define GET_GROUP_VER(x)	(x & 1)

#define IS_TYPE_B(a)	(a[1] & 0x0800)

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
void set_rds_rtplus_flags(RDSEncoder* enc, uint8_t flags);
void set_rds_rtplus_tags(RDSEncoder* enc, uint8_t *tags);
void set_rds_ptyn(RDSEncoder* enc, char *ptyn);
void set_rds_grpseq(RDSEncoder* enc, char *grpseq);
void set_rds_grpseq2(RDSEncoder* enc, char *grpseq2);

#endif
