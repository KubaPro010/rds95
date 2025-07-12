#include "common.h"
#include <signal.h>
#include <getopt.h>
#include <pthread.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include "../inih/ini.h"

#include "rds.h"
#include "modulator.h"
#include "control_pipe.h"
#include "udp_server.h"
#include "lib.h"
#include "ascii_cmd.h"

#define RDS_DEVICE "RDS"
#define DEFAULT_CONFIG_PATH "/etc/rds95.conf"
#define DEFAULT_STREAMS 2
#define MAX_STREAMS 4

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

static void *udp_server_worker() {
	while (!stop_rds) {
		poll_udp_server();
		msleep(READ_TIMEOUT_MS);
	}

	close_udp_server();
	pthread_exit(NULL);
}

static inline void show_help(char *name) {
	printf(
		"\n"
		"Usage: %s [options]\n"
		"\n"
		"\t-c,--config\tSet the config path [default: %s]\n"
		"\n",
		name,
		DEFAULT_CONFIG_PATH
	);
}

typedef struct
{
	char control_pipe[51];
	uint16_t udp_port;
	char rds_device_name[32];
	uint8_t num_streams;
} RDS95_Config;

static int config_handler(void* user, const char* section, const char* name, const char* value) {
    RDS95_Config* config = (RDS95_Config*)user;

    #define MATCH(s, n) (strcmp(section, s) == 0 && strcmp(name, n) == 0)

    if (MATCH("rds95", "control_pipe")) {
        strncpy(config->control_pipe, value, sizeof(config->control_pipe) - 1);
        config->control_pipe[sizeof(config->control_pipe) - 1] = '\0';
    } else if (MATCH("rds95", "udp_port")) {
        config->udp_port = (uint16_t)atoi(value);
    } else if (MATCH("devices", "rds95")) {
        strncpy(config->rds_device_name, value, sizeof(config->rds_device_name) - 1);
        config->rds_device_name[sizeof(config->rds_device_name) - 1] = '\0';
    } else if (MATCH("rds95", "streams")) {
        int streams = atoi(value);
        if (streams > MAX_STREAMS || streams == 0) return 0;
        config->num_streams = (uint8_t)streams;
    } else {
        return 0;  // Unknown config key
    }
    return 1;
}

int main(int argc, char **argv) {
	printf("rds95 (a RDS encoder by radio95) version %s\n", VERSION);

	char config_path[64] = DEFAULT_CONFIG_PATH;
	RDS95_Config config = {
		.control_pipe = "\0",
		.udp_port = 0,
		.rds_device_name = RDS_DEVICE,
		.num_streams = DEFAULT_STREAMS
	};

	pa_simple *rds_device = NULL;
	pa_sample_spec format;
	pa_buffer_attr buffer;

	pthread_attr_t attr;
	pthread_t control_pipe_thread;
	pthread_t udp_server_thread;

	const char	*short_opt = "c:h";

	struct option	long_opt[] =
	{
		{"config",		required_argument, NULL, 'c'},
		{"help",	no_argument,       NULL, 'h'},
		{ 0,		0,		0,	0 }
	};

	int opt;
	while((opt = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
		switch (opt) {
			case 'c':
				memcpy(config_path, optarg, 62);
				config_path[63] = '\0';
				break;
			case 'h':
				show_help(argv[0]);
				return 0;
			default:
				show_help(argv[0]);
				return 1;
		}
	}

	int res = ini_parse(config_path, config_handler, &config);
	if(res != 0) {
		fprintf(stderr, "Error: Could not read ini config, error code as return code.\n");
		return res;
	}

	printf("Using %d RDS stream(s)\n", config.num_streams);

	pthread_attr_init(&attr);

	signal(SIGINT, stop);
	signal(SIGTERM, stop);

	format.format = PA_SAMPLE_FLOAT32NE;
	format.channels = config.num_streams;  // Use dynamic stream count
	format.rate = RDS_SAMPLE_RATE;

	buffer.prebuf = 0;
	buffer.tlength = NUM_MPX_FRAMES * config.num_streams;
	buffer.maxlength = NUM_MPX_FRAMES * config.num_streams;

	rds_device = pa_simple_new(
		NULL,
		"rds95",
		PA_STREAM_PLAYBACK,
		config.rds_device_name,
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
	init_rds_modulator(&rdsModulator, &rdsEncoder, config.num_streams);

	if (config.control_pipe[0]) {
		if (open_control_pipe(config.control_pipe) == 0) {
			fprintf(stderr, "Reading control commands on %s.\n", config.control_pipe);
			int r = pthread_create(&control_pipe_thread, &attr, control_pipe_worker, (void*)&rdsModulator);
			if (r < 0) {
				fprintf(stderr, "Could not create control pipe thread.\n");
				config.control_pipe[0] = 0;
				goto exit;
			} else fprintf(stderr, "Created control pipe thread.\n");
		} else {
			fprintf(stderr, "Failed to open control pipe: %s.\n", config.control_pipe);
			config.control_pipe[0] = 0;
		}
	}

	if(config.udp_port) {
		if(open_udp_server(config.udp_port, &rdsModulator) == 0) {
			fprintf(stderr, "Reading control commands on UDP:%d.\n", config.udp_port);
			int r = pthread_create(&udp_server_thread, &attr, udp_server_worker, NULL);
			if (r < 0) {
				fprintf(stderr, "Could not create UDP server thread.\n");
				config.udp_port = 0;
				goto exit;
			} else fprintf(stderr, "Created UDP server thread.\n");
		} else {
			fprintf(stderr, "Failed to open UDP server\n");
			config.udp_port = 0;
		}
	}

	int pulse_error;

	// Dynamically allocate buffer based on stream count
	float *rds_buffer = (float*)malloc(NUM_MPX_FRAMES * config.num_streams * sizeof(float));
	if (rds_buffer == NULL) {
		fprintf(stderr, "Error: Could not allocate memory for RDS buffer\n");
		goto exit;
	}

	while(!stop_rds) {
		for (uint16_t i = 0; i < NUM_MPX_FRAMES * config.num_streams; i++) {
			rds_buffer[i] = get_rds_sample(&rdsModulator, i % config.num_streams);
		}

		if (pa_simple_write(rds_device, rds_buffer, NUM_MPX_FRAMES * config.num_streams * sizeof(float), &pulse_error) != 0) {
			fprintf(stderr, "Error: could not play audio. (%s : %d)\n", pa_strerror(pulse_error), pulse_error);
			break;
		}
	}

	free(rds_buffer);

exit:
	if (config.control_pipe[0]) {
		fprintf(stderr, "Waiting for pipe thread to shut down.\n");
		pthread_join(control_pipe_thread, NULL);
	}

	if(config.udp_port) {
		fprintf(stderr, "Waiting for UDP thread to shut down.\n");
		pthread_join(udp_server_thread, NULL);
	}

	cleanup_rds_modulator(&rdsModulator);  // Clean up dynamically allocated memory
	pthread_attr_destroy(&attr);
	if (rds_device != NULL) pa_simple_free(rds_device);

	return 0;
}