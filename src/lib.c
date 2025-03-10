/*
 * mpxgen - FM multiplex encoder with Stereo and RDS
 * Copyright (C) 2021 Anthony96922
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"
#include "rds.h"
#include <time.h>

/*
 * Stuff common for both RDS and RDS2
 *
 */

/* needed workaround implicit declaration (edit: do we need this? ) */
extern int nanosleep(const struct timespec *req, struct timespec *rem);

/* millisecond sleep */
void msleep(unsigned long ms) {
	struct timespec ts;
	ts.tv_sec = ms / 1000ul;		/* whole seconds */
	ts.tv_nsec = (ms % 1000ul) * 1000;	/* remainder, in nanoseconds */
	nanosleep(&ts, NULL);
}

/* just like strlen */
size_t _strnlen(const char *s, size_t maxlen) {
	size_t len = 0;
	while (s[len] != 0 && len < maxlen)
		len++;
	return len;
}

/* unsigned equivalent of strcmp */
int ustrcmp(const unsigned char *s1, const unsigned char *s2) {
	unsigned char c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;
		if (c1 == '\0')
			return c1 - c2;
	} while (c1 == c2);

	return c1 - c2;
}

#ifdef ODA_RTP
static char *rtp_content_types[64] = {
	/* dummy */
	"DUMMY_CLASS",
	/* item */
	"ITEM.TITLE",
	"ITEM.ALBUM",
	"ITEM.TRACKNUMBER",
	"ITEM.ARTIST",
	"ITEM.COMPOSITION",
	"ITEM.MOVEMENT",
	"ITEM.CONDUCTOR",
	"ITEM.COMPOSER",
	"ITEM.BAND",
	"ITEM.COMMENT",
	"ITEM.GENRE",
	/* info */
	"INFO.NEWS",
	"INFO.NEWS.LOCAL",
	"INFO.STOCKMARKET",
	"INFO.SPORT",
	"INFO.LOTTERY",
	"INFO.HOROSCOPE",
	"INFO.DAILY_DIVERSION",
	"INFO.HEALTH",
	"INFO.EVENT",
	"INFO.SCENE",
	"INFO.CINEMA",
	"INFO.TV",
	"INFO.DATE_TIME",
	"INFO.WEATHER",
	"INFO.TRAFFIC",
	"INFO.ALARM",
	"INFO.ADVERTISEMENT",
	"INFO.URL",
	"INFO.OTHER",
	/* program */
	"STATIONNAME.SHORT",
	"STATIONNAME.LONG",
	"PROGRAMME.NOW",
	"PROGRAMME.NEXT",
	"PROGRAMME.PART",
	"PROGRAMME.HOST",
	"PROGRAMME.EDITORIAL_STAFF",
	"PROGRAMME.FREQUENCY",
	"PROGRAMME.HOMEPAGE",
	"PROGRAMME.SUBCHANNEL",
	/* interactivity */
	"PHONE.HOTLINE",
	"PHONE.STUDIO",
	"PHONE.OTHER",
	"SMS.STUDIO",
	"SMS.OTHER",
	"EMAIL.HOTLINE",
	"EMAIL.STUDIO",
	"EMAIL.OTHER",
	"MMS.OTHER",
	"CHAT",
	"CHAT.CENTRE",
	"VOTE.QUESTION",
	"VOTE.CENTRE",
	/* rfu */
	"RFU_1",
	"RFU_2",
	/* private classes */
	"PRIVATE_1",
	"PRIVATE_2",
	"PRIVATE_3",
	/* descriptor */
	"PLACE",
	"APPOINTMENT",
	"IDENTIFIER",
	"PURCHASE",
	"GET_DATA"
};

uint8_t get_rtp_tag_id(char *rtp_tag_name) {
	uint8_t tag_id = 0;
	for (uint8_t i = 0; i < 64; i++) {
		if (strcmp(rtp_tag_name, rtp_content_types[i]) == 0) {
			tag_id = i;
			break;
		}
	}
	return tag_id;
}

char *get_rtp_tag_name(uint8_t rtp_tag) {
	if (rtp_tag > 63) rtp_tag = 0;
	return rtp_content_types[rtp_tag];
}
#endif
static uint16_t offset_words[] = {
	0x0FC, /*  A  */
	0x198, /*  B  */
	0x168, /*  C  */
	0x1B4, /*  D  */
	0x350  /*  C' */
};

/* CRC-16 ITU-T/CCITT checkword calculation */
/* wanna ask where is this used */
uint16_t crc16(uint8_t *data, size_t len) {
	uint16_t crc = 0xffff;

	for (size_t i = 0; i < len; i++) {
		crc = (crc >> 8) | (crc << 8);
		crc ^= data[i];
		crc ^= (crc & 0xff) >> 4;
		crc ^= (crc << 8) << 4;
		crc ^= ((crc & 0xff) << 4) << 1;
	}

	return crc ^ 0xffff;
}

/* Calculate the checkword for each block and emit the bits */
#ifdef RDS2
void add_checkwords(uint16_t *blocks, uint8_t *bits, bool rds2)
#else
void add_checkwords(uint16_t *blocks, uint8_t *bits)
#endif
{
	size_t i, j;
	uint8_t bit, msb;
	uint16_t block, block_crc, check, offset_word;
	bool group_type_b = false;
#ifdef RDS2
	bool tunneling_type_b = false;
#endif

	/* if b11 is 1, then type B */
#ifdef RDS2
	if (!rds2 && IS_TYPE_B(blocks))
#else
	if (IS_TYPE_B(blocks))
#endif
		group_type_b = true;

#ifdef RDS2
	/* for when version B groups are coded in RDS2 */
	if (rds2 && IS_TUNNELING(blocks) && IS_TYPE_B(blocks))
		tunneling_type_b = true;
#endif

	for (i = 0; i < GROUP_LENGTH; i++) {
#ifdef RDS2
		/*
		 * If tunneling type B groups use offset
		 * word C' for block 3
		 */
		if (rds2 && i == 2 && tunneling_type_b) {
			offset_word = offset_words[4];
		} else
#endif
		/* Group version B needs C' for block 3 */
		if (i == 2 && group_type_b) {
			offset_word = offset_words[4];
		} else {
			offset_word = offset_words[i];
		}

		block = blocks[i];

		/* Classical CRC computation */
		block_crc = 0;
		for (j = 0; j < BLOCK_SIZE; j++) {
			bit = (block & (INT16_15 >> j)) != 0;
			msb = (block_crc >> (POLY_DEG - 1)) & 1;
			block_crc <<= 1;
			if (msb ^ bit) block_crc ^= POLY;
			*bits++ = bit;
		}
		check = block_crc ^ offset_word;
		for (j = 0; j < POLY_DEG; j++) {
			*bits++ = (check & ((1 << (POLY_DEG - 1)) >> j)) != 0;
		}
	}
}

/*
 * AF stuff
 */
uint8_t add_rds_af(struct rds_af_t *af_list, float freq) {
	uint16_t af;
	uint8_t entries_reqd = 1; /* default for FM */

	/*
	 * check how many slots are required for the frequency
	 *
	 * LF/MF (AM) needs 2 entries: one for the
	 * AM follows code and the freq code itself
	 */
	if (freq < 87.6f || freq > 107.9f) { /* is LF/MF */
		entries_reqd = 2;
	}

	/* check if the AF list is full */
	if (af_list->num_afs + entries_reqd > MAX_AFS) {
		/* Too many AF entries */
		return 1;
	}

	/* check if new frequency is valid */
	if (freq >= 87.6f && freq <= 107.9f) { /* FM */
		af = (uint16_t)(freq * 10.0f) - 875;
		af_list->afs[af_list->num_entries] = af;
		af_list->num_entries += 1;
	} else if (freq >= 153.0f && freq <= 279.0f) { /* LF */
		af = (uint16_t)(freq - 153.0f) / 9 + 1;
		af_list->afs[af_list->num_entries + 0] = AF_CODE_LFMF_FOLLOWS;
		af_list->afs[af_list->num_entries + 1] = af;
		af_list->num_entries += 2;
	} else if (freq >= 531.0f && freq <= 1602.0f) { /* AM 9 kHz spacing */
		af = (uint16_t)(freq - 531.0f) / 9 + 16;
		af_list->afs[af_list->num_entries + 0] = AF_CODE_LFMF_FOLLOWS;
		af_list->afs[af_list->num_entries + 1] = af;
		af_list->num_entries += 2;
	} else {
		return 1;
	}

	af_list->num_afs++;

	return 0;
}

char *show_af_list(struct rds_af_t af_list) {
	float freq;
	bool is_lfmf = false;
	static char outstr[255];
	uint8_t outstrlen = 0;

	outstrlen += sprintf(outstr, "AF: %u,", af_list.num_afs);

	for (uint8_t i = 0; i < af_list.num_entries; i++) {
		if (af_list.afs[i] == AF_CODE_LFMF_FOLLOWS) {
			/* The next AF will be for LF/MF */
			is_lfmf = true;
			continue;
		}

		if (is_lfmf) {
			if (af_list.afs[i] >= 1 && af_list.afs[i] <= 15) { /* LF */
				freq = 153.0f + ((float)(af_list.afs[i] -  1) * 9.0f);
				outstrlen += sprintf(outstr + outstrlen, " (LF)%.0f", freq);
			} else { /*if (af_list.afs[i] >= 16 && af_list.afs[i] <= 135) {*/ /* MF */
				freq = 531.0f + ((float)(af_list.afs[i] - 16) * 9.0f);
				outstrlen += sprintf(outstr + outstrlen, " (MF)%.0f", freq);
			}
			is_lfmf = false;
			continue;
		}

		/* FM */
		freq = (float)((uint16_t)af_list.afs[i] + 875) / 10.0f;
		outstrlen += sprintf(outstr + outstrlen, " %.1f", freq);
	}

	return outstr;
}

/*
 * UTF-8 to RDS char set converter
 *
 * Translates certain chars into their RDS equivalents
 * NOTE!! Only applies to PS, RT and PTYN. LPS uses UTF-8 (SCB = 1)
 *
 */
#define XLATSTRLEN	255
unsigned char *xlat(unsigned char *str) {
	static unsigned char new_str[XLATSTRLEN];
	uint8_t i = 0;

	while (*str != 0 && i < XLATSTRLEN) {
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