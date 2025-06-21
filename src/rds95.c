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
#define DEFAULT_STREAMS 2
#define MAX_STREAMS 8

#define NUM_MPX_FRAMES	128

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

static inline void show_version() {
	printf("rds95 (a RDS encoder by radio95) version %s\n", VERSION);
}

static inline void show_help(char *name) {
	printf(
		"\n"
		"Usage: %s [options]\n"
		"\n"
		"\t-C,--ctl\tFIFO control pipe\n"
		"\t-d,--device\tPulseAudio device to use (default: %s)\n"
		"\t-s,--streams\tNumber of RDS streams (1-%d, default: %d)\n"
		"\n",
		name,
		RDS_DEVICE,
		MAX_STREAMS,
		DEFAULT_STREAMS
	);
}

int main(int argc, char **argv) {
	show_version();

	char control_pipe[51] = "\0";
	char rds_device_name[32] = RDS_DEVICE;
	uint8_t num_streams = DEFAULT_STREAMS;

	pa_simple *rds_device = NULL;
	pa_sample_spec format;
	pa_buffer_attr buffer;

	pthread_attr_t attr;
	pthread_t control_pipe_thread;

	const char	*short_opt = "C:d:s:h";

	struct option	long_opt[] =
	{
		{"ctl",		required_argument, NULL, 'C'},
		{"device",	required_argument, NULL, 'd'},
		{"streams",	required_argument, NULL, 's'},
		{"help",	no_argument,       NULL, 'h'},
		{ 0,		0,		0,	0 }
	};

	int opt;
	while((opt = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
		switch (opt) {
			case 'C':
				memcpy(control_pipe, optarg, 50);
				break;
			case 'd':
				memcpy(rds_device_name, optarg, 31);
				rds_device_name[31] = '\0';
				break;
			case 's':
				num_streams = (uint8_t)atoi(optarg);
				if (num_streams < 1 || num_streams > MAX_STREAMS) {
					fprintf(stderr, "Error: Number of streams must be between 1 and %d\n", MAX_STREAMS);
					return 1;
				}
				break;
			case 'h':
				show_help(argv[0]);
				return 0;
			default:
				show_help(argv[0]);
				return 1;
		}
	}

	printf("Using %d RDS stream(s)\n", num_streams);

	pthread_attr_init(&attr);

	signal(SIGINT, stop);
	signal(SIGTERM, stop);

	format.format = PA_SAMPLE_FLOAT32NE;
	format.channels = num_streams;  // Use dynamic stream count
	format.rate = RDS_SAMPLE_RATE;

	buffer.prebuf = 0;
	buffer.tlength = NUM_MPX_FRAMES * num_streams;
	buffer.maxlength = NUM_MPX_FRAMES * num_streams;

	rds_device = pa_simple_new(
		NULL,
		"rds95",
		PA_STREAM_PLAYBACK,
		rds_device_name,
		"RDS Generator",
		&format,
		NULL,
		&buffer,
		NULL
	);
	if (rds_device == NULL) {
		fprintf(stderr, "Error: cannot open sound device.\n");
		goto exit;
	}

	RDSEncoder rdsEncoder;
	RDSModulator rdsModulator;
	init_rds_encoder(&rdsEncoder);
	init_rds_modulator(&rdsModulator, &rdsEncoder, num_streams);

	if (control_pipe[0]) {
		if (open_control_pipe(control_pipe) == 0) {
			fprintf(stderr, "Reading control commands on %s.\n", control_pipe);
			int r = pthread_create(&control_pipe_thread, &attr, control_pipe_worker, (void*)&rdsModulator);
			if (r < 0) {
				fprintf(stderr, "Could not create control pipe thread.\n");
				control_pipe[0] = 0;
				goto exit;
			} else fprintf(stderr, "Created control pipe thread.\n");
		} else {
			fprintf(stderr, "Failed to open control pipe: %s.\n", control_pipe);
			control_pipe[0] = 0;
		}
	}

	int pulse_error;

	// Dynamically allocate buffer based on stream count
	float *rds_buffer = (float*)malloc(NUM_MPX_FRAMES * num_streams * sizeof(float));
	if (rds_buffer == NULL) {
		fprintf(stderr, "Error: Could not allocate memory for RDS buffer\n");
		goto exit;
	}

	while(!stop_rds) {
		for (uint16_t i = 0; i < NUM_MPX_FRAMES * num_streams; i++) {
			rds_buffer[i] = get_rds_sample(&rdsModulator, i % num_streams);
		}

		if (pa_simple_write(rds_device, rds_buffer, NUM_MPX_FRAMES * num_streams * sizeof(float), &pulse_error) != 0) {
			fprintf(stderr, "Error: could not play audio. (%s : %d)\n", pa_strerror(pulse_error), pulse_error);
			break;
		}
	}

	free(rds_buffer);

exit:
	if (control_pipe[0]) {
		fprintf(stderr, "Waiting for pipe thread to shut down.\n");
		pthread_join(control_pipe_thread, NULL);
	}

	cleanup_rds_modulator(&rdsModulator);  // Clean up dynamically allocated memory
	pthread_attr_destroy(&attr);
	if (rds_device != NULL) pa_simple_free(rds_device);

	return 0;
}