#include "common.h"
#include "rds.h"
#include "lib.h"
#include "ascii_cmd.h"

#define CMD_MATCHES(a) (ustrcmp(cmd, (unsigned char *)a) == 0)

void process_ascii_cmd(unsigned char *str) {
	unsigned char *cmd, *arg;
	uint16_t cmd_len = 0;

	cmd_len = _strnlen((const char*)str, CTL_BUFFER_SIZE);

	if (cmd_len > 3 && str[2] == ' ') {
		cmd = str;
		cmd[2] = 0;
		arg = str + 3;

		if (CMD_MATCHES("CG")) {
			/* this stays */
			uint16_t blocks[4];
			int count = sscanf((const char*)arg, "%hX %hX %hX %hX",
							&blocks[0], &blocks[1],
							&blocks[2], &blocks[3]);
			if (count == 4) {
				set_rds_cg(blocks);
			}
			return;
		}
	}

	if(cmd_len == 5) {
		cmd = str;
		if(CMD_MATCHES("AFCH=")) {
			clear_rds_af();
			return;
		}
	}
	if (cmd_len > 5 && str[4] == '=') {
		/* compatibilty with existing (standarts)*/
		cmd = str;
		cmd[4] = 0;
		arg = str + 5;
		if (CMD_MATCHES("TEXT")) {
			arg[RT_LENGTH * 2] = 0;
			set_rds_rt1(xlat(arg));
			return;
		}
		if (CMD_MATCHES("PTYN")) {
			arg[PTYN_LENGTH] = 0;
			set_rds_ptyn(xlat(arg));
			return;
		}
		if (CMD_MATCHES("AFCH")) {
			if(arg[0] == 'A' || arg[0] == 'B') {
				return;
			}
			clear_rds_af();
			uint8_t arg_count;
			rds_af_t new_af;
			uint8_t af[MAX_AFS], *af_iter;
			arg_count = sscanf((char *)arg,
				"%hhx,%hhx,%hhx,%hhx,%hhx," /* AF list */
				"%hhx,%hhx,%hhx,%hhx,%hhx,"
				"%hhx,%hhx,%hhx,%hhx,%hhx,"
				"%hhx,%hhx,%hhx,%hhx,%hhx,"
				"%hhx,%hhx,%hhx,%hhx,%hhx",
			&af[0],  &af[1],  &af[2],  &af[3],  &af[4],
			&af[5],  &af[6],  &af[7],  &af[8],  &af[9],
			&af[10], &af[11], &af[12], &af[13], &af[14],
			&af[15], &af[16], &af[17], &af[18], &af[19],
			&af[20], &af[21], &af[22], &af[23], &af[24]);

			af_iter = af;
			memset(&new_af, 0, sizeof(struct rds_af_t));
			while (arg_count-- != 0) {
				uint8_t current_value = *af_iter;
				float frequency = (875.0 + current_value) / 10.0;
				add_rds_af(&new_af, frequency);
				af_iter++;
			}
			set_rds_af(new_af);
			return;
		}
	}

	if(cmd_len == 4) {
		cmd = str;
		if(CMD_MATCHES("TPS=")) {
			set_rds_tpson(0);
			return;
		}
		if(CMD_MATCHES("LPS=")) {
			set_rds_lpson(0);
			return;
		}
	}
	if (cmd_len > 4 && str[3] == '=') {
		cmd = str;
		cmd[3] = 0;
		arg = str + 4;
		if (CMD_MATCHES("TPS")) {
			arg[PS_LENGTH * 2] = 0;
			set_rds_tps(xlat(arg));
			set_rds_tpson(1);
			return;
		}
		if (CMD_MATCHES("RT1")) {
			arg[RT_LENGTH * 2] = 0;
			set_rds_rt1(xlat(arg));
			return;
		}
		if (CMD_MATCHES("PTY")) {
			arg[2] = 0;
			set_rds_pty(strtoul((char *)arg, NULL, 10));
			return;
		}
		if (CMD_MATCHES("ECC")) {
			arg[2] = 0;
			set_rds_ecc(strtoul((char *)arg, NULL, 16));
			return;
		}
		if (CMD_MATCHES("LIC")) {
			arg[2] = 0;
			set_rds_lic(strtoul((char *)arg, NULL, 16));
			return;
		}

		#ifdef ODA_RTP
		if (CMD_MATCHES("RTP")) {
			char tag_names[2][32];
			uint8_t tags[6];
			if (sscanf((char *)arg, "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu",
				&tags[0], &tags[1], &tags[2], &tags[3],
				&tags[4], &tags[5]) == 6) {
				set_rds_rtplus_tags(tags);
			} else if (sscanf((char *)arg, "%31[^,],%hhu,%hhu,%31[^,],%hhu,%hhu",
				tag_names[0], &tags[1], &tags[2],
				tag_names[1], &tags[4], &tags[5]) == 6) {
				tags[0] = get_rtp_tag_id(tag_names[0]);
				tags[3] = get_rtp_tag_id(tag_names[1]);
				set_rds_rtplus_tags(tags);
			}
			return;
		}
		#endif

		if (CMD_MATCHES("LPS")) {
			arg[LPS_LENGTH] = 0;
			set_rds_lpson(1);
			set_rds_lps(arg);
			return;
		}

		if (CMD_MATCHES("PIN")) {
			uint8_t pin[3];
			if (sscanf((char *)arg, "%hhu,%hhu,%hhu",
				&pin[0], &pin[1], &pin[2]) == 3) {
					set_rds_pin(pin[0], pin[1], pin[2]);
			}
			return;
		}
	}
	if(cmd_len == 3) {
		cmd = str;
		if(CMD_MATCHES("AF=")) {
			clear_rds_af();
			return;
		}
	}
	if (cmd_len > 3 && str[2] == '=') {
		cmd = str;
		cmd[2] = 0;
		arg = str + 3;

		if (CMD_MATCHES("PS")) {
			if(arg[0] == '\0') arg[0] = ' '; /* fix for strings that start with a space idk why but tps works fine with space started strings */
			arg[PS_LENGTH * 2] = 0;
			set_rds_ps(xlat(arg));
			return;
		}
		if (CMD_MATCHES("CT")) {
			set_rds_ct(arg[0]);
			return;
		}
		if (CMD_MATCHES("DI")) {
			arg[2] = 0;
			set_rds_di(strtoul((char *)arg, NULL, 10));
			return;
		}
		if (CMD_MATCHES("TP")) {
			set_rds_tp(arg[0]);
			return;
		}
		if (CMD_MATCHES("TA")) {
			set_rds_ta(arg[0]);
			return;
		}
		if (CMD_MATCHES("MS")) {
			set_rds_ms(arg[0]);
			return;
		}
		if (CMD_MATCHES("PI")) {
			arg[4] = 0;
			set_rds_pi(strtoul((char *)arg, NULL, 16));
			return;
		}

		if (CMD_MATCHES("AF")) {
			if(arg[0] == 'A' || arg[0] == 'B') {
				return;
			}
			clear_rds_af();
			uint8_t arg_count;
			rds_af_t new_af;
			float af[MAX_AFS], *af_iter;
			arg_count = sscanf((char *)arg,
				"%f,%f,%f,%f,%f," /* AF list */
				"%f,%f,%f,%f,%f,"
				"%f,%f,%f,%f,%f,"
				"%f,%f,%f,%f,%f,"
				"%f,%f,%f,%f,%f",
			&af[0],  &af[1],  &af[2],  &af[3],  &af[4],
			&af[5],  &af[6],  &af[7],  &af[8],  &af[9],
			&af[10], &af[11], &af[12], &af[13], &af[14],
			&af[15], &af[16], &af[17], &af[18], &af[19],
			&af[20], &af[21], &af[22], &af[23], &af[24]);
			af_iter = af;
			memset(&new_af, 0, sizeof(struct rds_af_t));
			while (arg_count-- != 0) {
				add_rds_af(&new_af, *af_iter++);
			}
			set_rds_af(new_af);
			return;
		}
	}

	if (cmd_len > 2 && str[1] == '=') {
		cmd = str;
		cmd[1] = 0;
		arg = str + 2;
		if (CMD_MATCHES("G")) {
			uint16_t blocks[4];
			if(cmd_len == 14) {
				/* RDS1 Group*/
				blocks[0] = get_rds_pi();
				int count = sscanf((char *)arg, "%4hx%4hx%4hx", &blocks[1], &blocks[2], &blocks[3]);
				if(count == 3) {
					set_rds_cg(blocks);
				}
			}
			return;
		}
	}
	if (cmd_len > 6 && str[5] == '=') {
		cmd = str;
		cmd[5] = 0;
		arg = str + 6;

		if (CMD_MATCHES("PINEN")) {
			arg[1] = 0;
			set_rds_pin_enabled(strtoul((char *)arg, NULL, 10));
			return;
		}

		if (CMD_MATCHES("RT1EN")) {
			set_rds_rt1_enabled(arg[0]);
			return;
		}
	}

	if (cmd_len > 6 && str[5] == '=') {
		cmd = str;
		cmd[5] = 0;
		arg = str + 6;
	}
	if (cmd_len > 7 && str[6] == '=') {
		cmd = str;
		cmd[6] = 0;
		arg = str + 7;

		if (CMD_MATCHES("PTYNEN")) {
			arg[1] = 0;
			set_rds_ptyn_enabled(strtoul((char *)arg, NULL, 10));
			return;
		}

		#ifdef ODA_RTP
		if (CMD_MATCHES("RTPRUN")) {
			arg[1] = 0;
			set_rds_rtplus_flags(strtoul((char *)arg, NULL, 10));
			return;
		}
		#endif
	}
	if (cmd_len > 6 && str[5] == '=') {
		cmd = str;
		cmd[5] = 0;
		arg = str + 6;
		if (CMD_MATCHES("ECCEN")) {
			set_rds_ecclic_toggle(arg[0]);
			return;
		}
	}
}
