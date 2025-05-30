#include "lib.h"

extern int nanosleep(const struct timespec *req, struct timespec *rem);
void msleep(unsigned long ms) {
	struct timespec ts;
	ts.tv_sec = ms / 1000ul;
	ts.tv_nsec = (ms % 1000ul) * 1000;
	nanosleep(&ts, NULL);
}

int _strnlen(const char *s, int maxlen) {
	int len = 0;
	while (s[len] != 0 && len < maxlen) len++;
	return len;
}

uint16_t crc16_ccitt(char* data, uint16_t len) {
	uint16_t i, crc=0xFFFF;
	for (i=0; i < len; i++ ) {
		crc = (unsigned char)(crc >> 8) | (crc << 8);
		crc ^= data[i];
		crc ^= (unsigned char)(crc & 0xff) >> 4;
		crc ^= (crc << 8) << 4;
		crc ^= ((crc & 0xff) << 4) << 1;
	}
	return ((crc ^= 0xFFFF) & 0xFFFF);
}

uint16_t get_block_from_group(RDSGroup *group, uint8_t block) {
	switch (block) {
	case 0: return group->a;
	case 1: return group->b;
	case 2: return group->c;
	case 3: return group->d;
	default: return 0;
	}
}

static uint16_t offset_words[] = {
	0x0FC, /*  A  */
	0x198, /*  B  */
	0x168, /*  C  */
	0x1B4, /*  D  */
	0x350  /*  C' */
};

void add_checkwords(RDSGroup *group, uint8_t *bits)
{
	uint16_t offset_word;

	for (uint8_t i = 0; i < GROUP_LENGTH; i++) {
		offset_word = offset_words[i];
		if (i == 2 && group->is_type_b) offset_word = offset_words[4];

		uint16_t block = get_block_from_group(group, i);

		uint16_t block_crc = 0;
		uint8_t j, bit, msb;
		for (j = 0; j < BLOCK_SIZE; j++) {
			bit = (block & (0x8000 >> j)) != 0;
			msb = (block_crc >> (POLY_DEG - 1)) & 1;
			block_crc <<= 1;
			if (msb ^ bit) block_crc ^= POLY;
			*bits++ = bit;
		}
		uint16_t check = block_crc ^ offset_word;
		for (j = 0; j < POLY_DEG; j++) *bits++ = (check & ((1 << (POLY_DEG - 1)) >> j)) != 0;
	}
}

uint8_t add_rds_af_oda(RDSAFsODA *af_list, float freq) {
	uint16_t af;

	uint8_t entries_reqd = 1;
	if (freq < 64.1f || freq > 107.9f) entries_reqd = 2;

	if (af_list->num_afs + entries_reqd > MAX_AFS) return 1;

	if(freq >= 64.1f && freq <= 88.0f) {
		af = (uint16_t)(freq * 10.0f) - 384;
		af_list->afs[af_list->num_entries] = af;
		af_list->num_entries += 1;
	} else if (freq >= 87.6f && freq <= 107.9f) {
		af = (uint16_t)(freq * 10.0f) - 875;
		af_list->afs[af_list->num_entries] = af;
		af_list->num_entries += 1;
	} else if (freq >= 153.0f && freq <= 279.0f) {
		af = (uint16_t)(freq - 153.0f) / 9 + 1;
		af_list->afs[af_list->num_entries + 0] = AF_CODE_LFMF_FOLLOWS;
		af_list->afs[af_list->num_entries + 1] = af;
		af_list->num_entries += 2;
	} else if (freq >= 531.0f && freq <= 1602.0f) {
		af = (uint16_t)(freq - 531.0f) / 9 + 16;
		af_list->afs[af_list->num_entries + 0] = AF_CODE_LFMF_FOLLOWS;
		af_list->afs[af_list->num_entries + 1] = af;
		af_list->num_entries += 2;
	} else return 1;

	af_list->num_afs++;

	return 0;
}

uint8_t add_rds_af(RDSAFs *af_list, float freq) {
	uint16_t af;

	uint8_t entries_reqd = 1;
	if (freq < 87.6f || freq > 107.9f) entries_reqd = 2;

	if (af_list->num_afs + entries_reqd > MAX_AFS) return 1;

	if (freq >= 87.6f && freq <= 107.9f) {
		af = (uint16_t)(freq * 10.0f) - 875;
		af_list->afs[af_list->num_entries] = af;
		af_list->num_entries += 1;
	} else if (freq >= 153.0f && freq <= 279.0f) {
		af = (uint16_t)(freq - 153.0f) / 9 + 1;
		af_list->afs[af_list->num_entries + 0] = AF_CODE_LFMF_FOLLOWS;
		af_list->afs[af_list->num_entries + 1] = af;
		af_list->num_entries += 2;
	} else if (freq >= 531.0f && freq <= 1602.0f) {
		af = (uint16_t)(freq - 531.0f) / 9 + 16;
		af_list->afs[af_list->num_entries + 0] = AF_CODE_LFMF_FOLLOWS;
		af_list->afs[af_list->num_entries + 1] = af;
		af_list->num_entries += 2;
	} else return 1;

	af_list->num_afs++;

	return 0;
}

char *convert_to_rdscharset(char *str) {
	static char new_str[255];
	uint8_t i = 0;

	while (*str != 0 && i < 255) {
		switch (*str) {
		case 0xc2:
			str++;
			switch (*str) {
			case 0xa1: new_str[i] = 0x8e; break; /* INVERTED EXCLAMATION MARK */
			case 0xa3: new_str[i] = 0xaa; break; /* POUND SIGN */
			case 0xa7: new_str[i] = 0xbf; break; /* SECTION SIGN */
			case 0xa9: new_str[i] = 0xa2; break; /* COPYRIGHT SIGN */
			case 0xaa: new_str[i] = 0xa0; break; /* FEMININE ORDINAL INDICATOR */
			case 0xb0: new_str[i] = 0xbb; break; /* DEGREE SIGN */
			case 0xb1: new_str[i] = 0xb4; break; /* PLUS-MINUS SIGN */
			case 0xb2: new_str[i] = 0xb2; break; /* SUPERSCRIPT TWO */
			case 0xb3: new_str[i] = 0xb3; break; /* SUPERSCRIPT THREE */
			case 0xb5: new_str[i] = 0xb8; break; /* MIKRO SIGN */
			case 0xb9: new_str[i] = 0xb1; break; /* SUPERSCRIPT ONE */
			case 0xba: new_str[i] = 0xb0; break; /* MASCULINE ORDINAL INDICATOR */
			case 0xbc: new_str[i] = 0xbc; break; /* VULGAR FRACTION ONE QUARTER */
			case 0xbd: new_str[i] = 0xbd; break; /* VULGAR FRACTION ONE HALF */
			case 0xbe: new_str[i] = 0xbe; break; /* VULGAR FRACTION THREE QUARTERS */
			case 0xbf: new_str[i] = 0xb9; break; /* INVERTED QUESTION MARK */
			default: new_str[i] = ' '; break;
			}
			break;

		case 0xc3:
			str++;
			switch (*str) {
			case 0x80: new_str[i] = 0xc1; break; /* LATIN CAPITAL LETTER A WITH GRAVE */
			case 0x81: new_str[i] = 0xc0; break; /* LATIN CAPITAL LETTER A WITH ACUTE */
			case 0x82: new_str[i] = 0xd0; break; /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX */
			case 0x83: new_str[i] = 0xe0; break; /* LATIN CAPITAL LETTER A WITH TILDE */
			case 0x84: new_str[i] = 0xd1; break; /* LATIN CAPITAL LETTER A WITH DIAERESIS */
			case 0x85: new_str[i] = 0xe1; break; /* LATIN CAPITAL LETTER A WITH RING ABOVE */
			case 0x86: new_str[i] = 0xe2; break; /* LATIN CAPITAL LETTER AE */
			case 0x87: new_str[i] = 0x8b; break; /* LATIN CAPITAL LETTER C WITH CEDILLA */
			case 0x88: new_str[i] = 0xc3; break; /* LATIN CAPITAL LETTER E WITH GRAVE */
			case 0x89: new_str[i] = 0xc2; break; /* LATIN CAPITAL LETTER E WITH ACUTE */
			case 0x8a: new_str[i] = 0xd2; break; /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX */
			case 0x8b: new_str[i] = 0xd3; break; /* LATIN CAPITAL LETTER E WITH DIAERESIS */
			case 0x8c: new_str[i] = 0xc5; break; /* LATIN CAPITAL LETTER I WITH GRAVE */
			case 0x8d: new_str[i] = 0xc4; break; /* LATIN CAPITAL LETTER I WITH ACUTE */
			case 0x8e: new_str[i] = 0xd4; break; /* LATIN CAPITAL LETTER I WITH CIRCUMFLEX */
			case 0x8f: new_str[i] = 0xd5; break; /* LATIN CAPITAL LETTER I WITH DIAERESIS */
			case 0x90: new_str[i] = 0xce; break; /* LATIN CAPITAL LETTER ETH */
			case 0x91: new_str[i] = 0x8a; break; /* LATIN CAPITAL LETTER N WITH TILDE */
			case 0x92: new_str[i] = 0xc7; break; /* LATIN CAPITAL LETTER O WITH GRAVE */
			case 0x93: new_str[i] = 0xc6; break; /* LATIN CAPITAL LETTER O WITH ACUTE */
			case 0x94: new_str[i] = 0xd6; break; /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX */
			case 0x95: new_str[i] = 0xe6; break; /* LATIN CAPITAL LETTER O WITH TILDE */
			case 0x96: new_str[i] = 0xd7; break; /* LATIN CAPITAL LETTER O WITH DIAERESIS */
			case 0x98: new_str[i] = 0xe7; break; /* LATIN CAPITAL LETTER O WITH STROKE */
			case 0x99: new_str[i] = 0xc9; break; /* LATIN CAPITAL LETTER U WITH GRAVE */
			case 0x9a: new_str[i] = 0xc8; break; /* LATIN CAPITAL LETTER U WITH ACUTE */
			case 0x9b: new_str[i] = 0xd8; break; /* LATIN CAPITAL LETTER U WITH CIRCUMFLEX */
			case 0x9c: new_str[i] = 0xd9; break; /* LATIN CAPITAL LETTER U WITH DIAERESIS */
			case 0x9d: new_str[i] = 0xe5; break; /* LATIN CAPITAL LETTER Y WITH ACUTE */
			case 0x9e: new_str[i] = 0xe8; break; /* LATIN CAPITAL LETTER THORN */
			case 0xa0: new_str[i] = 0x81; break; /* LATIN SMALL LETTER A WITH GRAVE */
			case 0xa1: new_str[i] = 0x80; break; /* LATIN SMALL LETTER A WITH ACUTE */
			case 0xa2: new_str[i] = 0x90; break; /* LATIN SMALL LETTER A WITH CIRCUMFLEX */
			case 0xa3: new_str[i] = 0xf0; break; /* LATIN SMALL LETTER A WITH TILDE */
			case 0xa4: new_str[i] = 0x91; break; /* LATIN SMALL LETTER A WITH DIAERESIS */
			case 0xa5: new_str[i] = 0xf1; break; /* LATIN SMALL LETTER A WITH RING ABOVE */
			case 0xa6: new_str[i] = 0xf2; break; /* LATIN SMALL LETTER AE */
			case 0xa7: new_str[i] = 0x9b; break; /* LATIN SMALL LETTER C WITH CEDILLA */
			case 0xa8: new_str[i] = 0x83; break; /* LATIN SMALL LETTER E WITH GRAVE */
			case 0xa9: new_str[i] = 0x82; break; /* LATIN SMALL LETTER E WITH ACUTE */
			case 0xaa: new_str[i] = 0x92; break; /* LATIN SMALL LETTER E WITH CIRCUMFLEX */
			case 0xab: new_str[i] = 0x93; break; /* LATIN SMALL LETTER E WITH DIAERESIS */
			case 0xac: new_str[i] = 0x85; break; /* LATIN SMALL LETTER I WITH GRAVE */
			case 0xad: new_str[i] = 0x84; break; /* LATIN SMALL LETTER I WITH ACUTE */
			case 0xae: new_str[i] = 0x94; break; /* LATIN SMALL LETTER I WITH CIRCUMFLEX */
			case 0xaf: new_str[i] = 0x95; break; /* LATIN SMALL LETTER I WITH DIAERESIS */
			case 0xb0: new_str[i] = 0xef; break; /* LATIN SMALL LETTER ETH */
			case 0xb1: new_str[i] = 0x9a; break; /* LATIN SMALL LETTER N WITH TILDE */
			case 0xb2: new_str[i] = 0x87; break; /* LATIN SMALL LETTER O WITH GRAVE */
			case 0xb3: new_str[i] = 0x86; break; /* LATIN SMALL LETTER O WITH ACUTE */
			case 0xb4: new_str[i] = 0x96; break; /* LATIN SMALL LETTER O WITH CIRCUMFLEX */
			case 0xb5: new_str[i] = 0xf6; break; /* LATIN SMALL LETTER O WITH TILDE */
			case 0xb6: new_str[i] = 0x97; break; /* LATIN SMALL LETTER O WITH DIAERESIS */
			case 0xb7: new_str[i] = 0xba; break; /* DIVISION SIGN */
			case 0xb8: new_str[i] = 0xf7; break; /* LATIN SMALL LETTER O WITH STROKE */
			case 0xb9: new_str[i] = 0x89; break; /* LATIN SMALL LETTER U WITH GRAVE */
			case 0xba: new_str[i] = 0x88; break; /* LATIN SMALL LETTER U WITH ACUTE */
			case 0xbb: new_str[i] = 0x98; break; /* LATIN SMALL LETTER U WITH CIRCUMFLEX */
			case 0xbc: new_str[i] = 0x99; break; /* LATIN SMALL LETTER U WITH DIAERESIS */
			case 0xbd: new_str[i] = 0xf5; break; /* LATIN SMALL LETTER Y WITH ACUTE */
			case 0xbe: new_str[i] = 0xf8; break; /* LATIN SMALL LETTER THORN */
			default: new_str[i] = ' '; break;
			}
			break;

		case 0xc4:
			str++;
			switch (*str) {
			case 0x87: new_str[i] = 0xfb; break; /* LATIN SMALL LETTER C WITH ACUTE */
			case 0x8c: new_str[i] = 0xcb; break; /* LATIN CAPITAL LETTER C WITH CARON */
			case 0x8d: new_str[i] = 0xdb; break; /* LATIN SMALL LETTER C WITH CARON */
			case 0x91: new_str[i] = 0xde; break; /* LATIN SMALL LETTER D WITH STROKE */
			case 0x9b: new_str[i] = 0xa5; break; /* LATIN SMALL LETTER E WITH CARON */
			case 0xb0: new_str[i] = 0xb5; break; /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
			case 0xb1: new_str[i] = 0x9f; break; /* LATIN SMALL LETTER DOTLESS I */
			case 0xb2: new_str[i] = 0x8f; break; /* LATIN CAPITAL LIGATURE IJ */
			case 0xb3: new_str[i] = 0x9f; break; /* LATIN SMALL LIGATURE IJ */
			case 0xbf: new_str[i] = 0xcf; break; /* LATIN CAPITAL LETTER L WITH MIDDLE DOT */
			default: new_str[i] = ' '; break;
			}
			break;

		case 0xc5:
			str++;
			switch (*str) {
			case 0x80: new_str[i] = 0xdf; break; /* LATIN SMALL LETTER L WITH MIDDLE DOT */
			case 0x84: new_str[i] = 0xb6; break; /* LATIN SMALL LETTER N WITH ACUTE */
			case 0x88: new_str[i] = 0xa6; break; /* LATIN SMALL LETTER N WITH CARON */
			case 0x8a: new_str[i] = 0xe9; break; /* LATIN CAPITAL LETTER ENG */
			case 0x8b: new_str[i] = 0xf9; break; /* LATIN SMALL LETTER ENG */
			case 0x91: new_str[i] = 0xa7; break; /* LATIN SMALL LETTER O WITH DOUBLE ACUTE */
			case 0x92: new_str[i] = 0xe3; break; /* LATIN CAPITAL LIGATURE OE */
			case 0x93: new_str[i] = 0xf3; break; /* LATIN SMALL LIGATURE OE */
			case 0x94: new_str[i] = 0xea; break; /* LATIN CAPITAL LETTER R WITH ACUTE */
			case 0x95: new_str[i] = 0xfa; break; /* LATIN SMALL LETTER R WITH ACUTE */
			case 0x98: new_str[i] = 0xca; break; /* LATIN CAPITAL LETTER R WITH CARON */
			case 0x99: new_str[i] = 0xda; break; /* LATIN SMALL LETTER R WITH CARON */
			case 0x9a: new_str[i] = 0xec; break; /* LATIN CAPITAL LETTER S WITH ACUTE */
			case 0x9b: new_str[i] = 0xfc; break; /* LATIN SMALL LETTER S WITH ACUTE */
			case 0x9e: new_str[i] = 0x8c; break; /* LATIN CAPITAL LETTER S WITH CEDILLA */
			case 0x9f: new_str[i] = 0x9c; break; /* LATIN SMALL LETTER S WITH CEDILLA */
			case 0xa0: new_str[i] = 0xcc; break; /* LATIN CAPITAL LETTER S WITH CARON */
			case 0xa1: new_str[i] = 0xdc; break; /* LATIN SMALL LETTER S WITH CARON */
			case 0xa6: new_str[i] = 0xee; break; /* LATIN CAPITAL LETTER T WITH STROKE */
			case 0xa7: new_str[i] = 0xfe; break; /* LATIN SMALL LETTER T WITH STROKE */
			case 0xb1: new_str[i] = 0xb7; break; /* LATIN SMALL LETTER U WITH DOUBLE ACUTE */
			case 0xb5: new_str[i] = 0xf4; break; /* LATIN SMALL LETTER W WITH CIRCUMFLEX */
			case 0xb7: new_str[i] = 0xe4; break; /* LATIN SMALL LETTER Y WITH CIRCUMFLEX */
			case 0xb9: new_str[i] = 0xed; break; /* LATIN CAPITAL LETTER Z WITH ACUTE */
			case 0xba: new_str[i] = 0xfd; break; /* LATIN SMALL LETTER Z WITH ACUTE */
			case 0xbd: new_str[i] = 0xcd; break; /* LATIN CAPITAL LETTER Z WITH CARON */
			case 0xbe: new_str[i] = 0xdd; break; /* LATIN SMALL LETTER Z WITH CARON */
			default: new_str[i] = ' '; break;
			}
			break;

		case 0xc7:
			str++;
			switch (*str) {
			case 0xa6: new_str[i] = 0xa4; break; /* LATIN CAPITAL LETTER G WITH CARON */
			case 0xa7: new_str[i] = 0x9d; break; /* LATIN SMALL LETTER G WITH CARON */
			default: new_str[i] = ' '; break;
			}
			break;

		case 0xce:
			str++;
			switch (*str) {
			case 0xb1: new_str[i] = 0xa1; break; /* GREEK SMALL LETTER ALPHA */
			case 0xb2: new_str[i] = 0x8d; break; /* GREEK SMALL LETTER BETA */
			default: new_str[i] = ' '; break;
			}
			break;

		case 0xcf:
			str++;
			switch (*str) {
			case 0x80: new_str[i] = 0xa8; break; /* GREEK SMALL LETTER PI */
			default: new_str[i] = ' '; break;
			}
			break;

		default: /* 0-127 or unknown */
			switch (*str) {
			case '$': new_str[i] = 0xab; break; /* DOLLAR SIGN */
			default: new_str[i] = *str; break;
			}
			break;
		}

		i++;
		str++;
	}

	new_str[i] = 0;
	return new_str;
}