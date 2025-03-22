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

#define RDS_DEVICE "RDS"
#define RDS2_DEVICE "RDS2"

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
	printf("rds95 (a RDS encoder by radio95) version 1.1\n");
}

static void show_help(char *name) {
	printf(
		"\n"
		"Usage: %s [options]\n"
		"\n"
		"    -C,--ctl          FIFO control pipe\n"
		"    -h,--help         Show this help text and exit\n"
		"\n",
		name
	);
}

int main(int argc, char **argv) {
	show_version();

	char control_pipe[51] = "\0";

	pa_simple *rds1_device;
	#ifdef RDS2_DEVICE
	pa_simple *rds2_device = NULL;
	#endif
	pa_sample_spec format;
	pa_buffer_attr buffer;

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

	buffer.prebuf = 0;
	buffer.tlength = 8192;
	buffer.maxlength = 8192;

	rds1_device = pa_simple_new(
		NULL,
		"rds95",
		PA_STREAM_PLAYBACK,
		RDS_DEVICE,
		"RDS1 Generator",
		&format,
		NULL,
		&buffer,
		NULL
	);
	if (rds1_device == NULL) {
		fprintf(stderr, "Error: cannot open sound device.\n");
		goto exit;
	}
	#ifdef RDS2_DEVICE
	rds2_device = pa_simple_new(
		NULL,
		"rds95",
		PA_STREAM_PLAYBACK,
		RDS2_DEVICE,
		"RDS2 Generator",
		&format,
		NULL,
		&buffer,
		NULL
	);
	if(rds2_device == NULL) {
		fprintf(stderr, "Error: cannot open sound device.\n");
		goto exit;
	}
	#endif

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

	float rds1_buffer[NUM_MPX_FRAMES];
	#ifdef RDS2_DEVICE
	float rds2_buffer[NUM_MPX_FRAMES];
	#endif

	while(!stop_rds) {
		for (uint16_t i = 0; i < NUM_MPX_FRAMES; i++) {
			rds1_buffer[i] = get_rds_sample(&rdsModulator, false);
			#ifdef RDS2_DEVICE
			rds2_buffer[i] = get_rds_sample(&rdsModulator, true);
			#endif
		}

		if (pa_simple_write(rds1_device, rds1_buffer, sizeof(rds1_buffer), &pulse_error) != 0) {
			fprintf(stderr, "Error: could not play audio. (%s : %d)\n", pa_strerror(pulse_error), pulse_error);
			break;
		}
		#ifdef RDS2_DEVICE
		if (pa_simple_write(rds2_device, rds2_buffer, sizeof(rds2_buffer), &pulse_error) != 0) {
			fprintf(stderr, "Error: could not play audio. RDS2 (%s : %d)\n", pa_strerror(pulse_error), pulse_error);
			break;
		}
		#endif
	}

exit:
	if (control_pipe[0]) {
		fprintf(stderr, "Waiting for pipe thread to shut down.\n");
		pthread_join(control_pipe_thread, NULL);
	}

	pthread_attr_destroy(&attr);
	pa_simple_free(rds1_device);
	#ifdef RDS2_DEVICE
	pa_simple_free(rds2_device);
	#endif

	return 0;
}