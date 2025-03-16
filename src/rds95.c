#include "common.h"
#include <signal.h>
#include <getopt.h>
#include <pthread.h>
#include <pulse/simple.h>
#include <pulse/error.h>

#include "rds.h"
#include "modulator.h"
#include "control_pipe.h"
#include "lib.h"
#include "ascii_cmd.h"

#define NUM_MPX_FRAMES	512

static uint8_t stop_rds;

static void stop() {
	printf("Received an stopping signal\n");
	stop_rds = 1;
}

static void *control_pipe_worker(void* modulator) {
	RDSModulator *mod = (RDSModulator*)modulator;
	while (!stop_rds) {
		poll_control_pipe(mod);
		msleep(READ_TIMEOUT_MS);
	}

	close_control_pipe();
	pthread_exit(NULL);
}

static void show_version() {
	printf("rds95 (a RDS encoder by radio95) version 1.0");
}

static void show_help(char *name) {
	printf(
		"\n"
		"Usage: %s [options]\n"
		"\n"
		"    -C,--ctl          FIFO control pipe\n"
		"    -h,--help         Show this help text and exit\n"
		"\n",
		VERSION,
		name
	);
}

int main(int argc, char **argv) {
	show_version();

	char control_pipe[51] = "\0";

	pa_simple *device;
	pa_sample_spec format;

	pthread_attr_t attr;
	pthread_t control_pipe_thread;

	const char	*short_opt = "C:h";

	struct option	long_opt[] =
	{
		{"ctl",		required_argument, NULL, 'C'},

		{"help",	no_argument, NULL, 'h'},
		{ 0,		0,		0,	0 }
	};

	int opt;
	while((opt = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
		switch (opt) {
			case 'C':
				memcpy(control_pipe, optarg, 50);
				break;

			case 'h':
			default:
				show_help(argv[0]);
				return 1;
		}
	}

	pthread_attr_init(&attr);

	signal(SIGINT, stop);
	signal(SIGTERM, stop);

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

	RDSEncoder rdsEncoder;
	RDSModulator rdsModulator;
	init_rds_encoder(&rdsEncoder);
	init_rds_modulator(&rdsModulator, &rdsEncoder);

	if (control_pipe[0]) {
		if (open_control_pipe(control_pipe) == 0) {
			fprintf(stderr, "Reading control commands on %s.\n", control_pipe);
			int r;
			r = pthread_create(&control_pipe_thread, &attr, control_pipe_worker, (void*)&rdsModulator);
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
			mpx_buffer[i] = get_rds_sample(&rdsModulator);
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