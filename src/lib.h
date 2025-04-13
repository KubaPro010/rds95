extern void msleep(unsigned long ms);

extern int _strnlen(const char *s, int maxlen);

extern uint16_t crc16_ccitt(char *data, uint16_t len);

uint16_t get_block_grom_group(RDSGroup *group, uint8_t block);

extern void add_checkwords(RDSGroup *group, uint8_t *bits);
extern uint8_t add_rds_af_oda(RDSAFsODA *af_list, float freq);
extern uint8_t add_rds_af(RDSAFs *af_list, float freq);
extern char *convert_to_rds_charset(char *str);