extern void msleep(unsigned long ms);

extern int _strnlen(const char *s, int maxlen);

extern uint16_t crc16_ccitt(char *data, uint16_t len);

extern void add_checkwords(uint16_t *blocks, uint8_t *bits, uint8_t stream);
extern uint8_t add_rds_af(RDSAFs *af_list, float freq);
extern char *convert_to_rds_charset(char *str);