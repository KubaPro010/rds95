extern void msleep(unsigned long ms);

extern int _strnlen(const char *s, int maxlen);

extern void add_checkwords(uint16_t *blocks, uint8_t *bits);
extern uint8_t add_rds_af(RDSAFs *af_list, float freq);
extern char *xlat(char *str);