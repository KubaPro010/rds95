#include "common.h"
#include <signal.h>
#include <getopt.h>
#include <pthread.h>
#include <pulse/simple.h>
#include <pulse/error.h>

#include "rds.h"
#include "control_pipe.h"
#include "lib.h"
#include "ascii_cmd.h"

#define NUM_MPX_FRAMES	512

static uint8_t stop_rds;

static void stop() {
	printf("Received an stopping signal\n");
	stop_rds = 1;
}

/* threads */
static void *control_pipe_worker() {
	while (!stop_rds) {
		poll_control_pipe();
		msleep(READ_TIMEOUT_MS);
	}

	close_control_pipe();
	pthread_exit(NULL);
}

static void show_help(char *name) {
	printf(
		"rds95 (a RDS encoder by radio95) version %.1f\n"
		"\n"
		"Usage: %s [options]\n"
		"\n"
		"    -i,--pi           Program Identification code\n"
		"                        [default: 305F]\n"
		"    -s,--ps           Program Service name\n"
		"                        [default: \"radio95\"]\n"
		"    -r,--rt1           Radio Text 1\n"
		"                        [default: (nothing)]\n"
		"    -p,--pty          Program Type\n"
		"                        [default: 0]\n"
		"    -T,--tp           Traffic Program\n"
		"                        [default: 0]\n"
		"    -A,--af           Alternative Frequency (FM/LF/MF)\n"
		"                        (more than one AF may be passed)\n"
		"    -P,--ptyn         Program Type Name\n"
		"    -l,--lps          Long PS\n"
		"    -e,--ecc          ECC code\n"
		"    -d,--di           DI code\n"
		"    -C,--ctl          FIFO control pipe\n"
		"    -h,--help         Show this help text and exit\n"
		"\n",
		VERSION,
		name
	);
}

int main(int argc, char **argv) {
	char control_pipe[51] = "\0";
	struct rds_params_t rds_params = {
		.ps = "radio95",
		.rt1 = "",
		.pi = 0x305F,
		.ecc = 0xE2,
		.lps = "radio95 - Radio Nowotomyskie",
		.grp_sqc = "00012222FFR"
	};
	/* PASIMPLE */
	pa_simple *device;
	pa_sample_spec format;

	/* pthread */
	pthread_attr_t attr;
	pthread_t control_pipe_thread;

	const char	*short_opt = "R:i:s:r:p:T:A:P:l:e:L:d:C:h";

	struct option	long_opt[] =
	{
		{"rds",		required_argument, NULL, 'R'},
		{"pi",		required_argument, NULL, 'i'},
		{"ps",		required_argument, NULL, 's'},
		{"rt1",		required_argument, NULL, 'r'},
		{"pty",		required_argument, NULL, 'p'},
		{"tp",		required_argument, NULL, 'T'},
		{"af",		required_argument, NULL, 'A'},
		{"ptyn",	required_argument, NULL, 'P'},
		{"lps",    	required_argument, NULL, 'l'},
		{"ecc",    	required_argument, NULL, 'e'},
		{"lic",    	required_argument, NULL, 'L'},
		{"di",    	required_argument, NULL, 'd'},
		{"ctl",		required_argument, NULL, 'C'},

		{"help",	no_argument, NULL, 'h'},
		{ 0,		0,		0,	0 }
	};

	int opt;
	while((opt = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
		switch (opt) {
			case 'i': /* pi */
				rds_params.pi = strtoul(optarg, NULL, 16);
				break;

			case 's': /* ps */
				memcpy(rds_params.ps, xlat((unsigned char *)optarg), PS_LENGTH);
				break;

			case 'r': /* rt1 */
				memcpy(rds_params.rt1, xlat((unsigned char *)optarg), RT_LENGTH);
				break;

			case 'p': /* pty */
				rds_params.pty = strtoul(optarg, NULL, 10);
				break;

			case 'T': /* tp */
				rds_params.tp = strtoul(optarg, NULL, 10);
				break;

			case 'A': /* af */
				if (add_rds_af(&rds_params.af, strtof(optarg, NULL)) == 1) return 1;
				break;

			case 'P': /* ptyn */
				memcpy(rds_params.ptyn, xlat((unsigned char *)optarg), PTYN_LENGTH);
				break;

			case 'l': /* lps */
				memcpy(rds_params.lps, (unsigned char *)optarg, LPS_LENGTH);
				break;

			case 'e': /* ecc */
				rds_params.ecc = strtoul(optarg, NULL, 16);
				break;

			case 'L': /* lic */
				rds_params.lic = strtoul(optarg, NULL, 16);
				break;

			case 'C': /* ctl */
				memcpy(control_pipe, optarg, 50);
				break;

			case 'h': /* help */
			default:
				show_help(argv[0]);
				return 1;
		}
	}

	/* Initialize pthread stuff */
	pthread_attr_init(&attr);

	/* Gracefully stop the encoder on SIGINT or SIGTERM */
	signal(SIGINT, stop);
	signal(SIGTERM, stop);

	/* Initialize the RDS modulator */
	init_rds_encoder(rds_params);

	/* PASIMPLE format */
	format.format = PA_SAMPLE_FLOAT32NE;
	format.channels = 1;
	format.rate = RDS_SAMPLE_RATE;

	device = pa_simple_new(
		NULL,                       // Default PulseAudio server
		"rds95",                 // Application name
		PA_STREAM_PLAYBACK,        // Direction (playback)
		"RDS",                       // Default device
		"RDS Generator",           // Stream description
		&format,                   // Sample format
		NULL,                       // Default channel map
		NULL,                       // Default buffering attributes
		NULL                     // Error variable
	);
	if (device == NULL) {
		fprintf(stderr, "Error: cannot open sound device.\n");
		goto exit;
	}

	if (control_pipe[0]) {
		if (open_control_pipe(control_pipe) == 0) {
			fprintf(stderr, "Reading control commands on %s.\n", control_pipe);
			int r;
			r = pthread_create(&control_pipe_thread, &attr, control_pipe_worker, NULL);
			if (r < 0) {
				fprintf(stderr, "Could not create control pipe thread.\n");
				control_pipe[0] = 0;
				goto exit;
			} else {
				fprintf(stderr, "Created control pipe thread.\n");
			}
		} else {
			fprintf(stderr, "Failed to open control pipe: %s.\n", control_pipe);
			control_pipe[0] = 0;
		}
	}

	int pulse_error;

	static float mpx_buffer[NUM_MPX_FRAMES];

	while(!stop_rds) {
		for (uint16_t i = 0; i < NUM_MPX_FRAMES; i++) {
			mpx_buffer[i] = get_rds_sample();
		}

		if (pa_simple_write(device, mpx_buffer, sizeof(mpx_buffer), &pulse_error) != 0) {
			fprintf(stderr, "Error: could not play audio. (%s : %d)\n", pa_strerror(pulse_error), pulse_error);
			break;
		}
	}

exit:
	if (control_pipe[0]) {
		fprintf(stderr, "Waiting for pipe thread to shut down.\n");
		pthread_join(control_pipe_thread, NULL);
	}

	pthread_attr_destroy(&attr);
	pa_simple_free(device);

	return 0;
}