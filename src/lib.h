#include "common.h"
#include "rds.h"
#include <time.h>

void msleep(unsigned long ms);

int _strnlen(const char *s, int maxlen);

uint16_t crc16_ccitt(char *data, uint16_t len);

uint16_t get_block_grom_group(RDSGroup *group, uint8_t block);

void add_checkwords(RDSGroup *group, uint8_t *bits);
uint8_t add_rds_af_oda(RDSAFsODA *af_list, float freq);
uint8_t add_rds_af(RDSAFs *af_list, float freq);
char *convert_to_rdscharset(char *str);