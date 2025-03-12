extern void msleep(unsigned long ms);

extern int _strnlen(const char *s, int maxlen);
extern int ustrcmp(const unsigned char *s1, const unsigned char *s2);

extern uint8_t get_rtp_tag_id(char *rtp_tag_name);
extern char *get_rtp_tag_name(uint8_t rtp_tag);
extern void add_checkwords(uint16_t *blocks, uint8_t *bits);
extern uint8_t add_rds_af(struct rds_af_t *af_list, float freq);
extern char *show_af_list(struct rds_af_t af_list);
extern unsigned char *xlat(unsigned char *str);