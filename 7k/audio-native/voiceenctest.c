/* voiceenctest.c - native voice encoder test application
 *
 * Based on native pcm test application platform/system/extras/sound/playwav.c
 *
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/msm_audio_qcp.h>
#include <linux/msm_audio_amrnb.h>
#include <pthread.h>
#include <errno.h>
#include "audiotest_def.h"

#define QCELP_DEVICE_NODE "/dev/msm_qcelp_in"
#define EVRC_DEVICE_NODE "/dev/msm_evrc_in"
#define AMRNB_DEVICE_NODE "/dev/msm_amrnb_in"

struct qcp_header {
	/* RIFF Section */
	char riff[4];
	unsigned int s_riff;
	char qlcm[4];

	/* Format chunk */
	char fmt[4];
        unsigned int s_fmt;
        char mjr;
        char mnr;
        unsigned int data1;         /* UNIQUE ID of the codec */
        unsigned short data2;
        unsigned short data3;
        char data4[8];
        unsigned short ver;         /* Codec Info */
        char name[80];
        unsigned short abps;    /* average bits per sec of the codec */
        unsigned short bytes_per_pkt;
        unsigned short samp_per_block;
        unsigned short samp_per_sec;
        unsigned short bits_per_samp;
        unsigned char vr_num_of_rates;         /* Rate Header fmt info */
        unsigned char rvd1[3];
        unsigned short vr_bytes_per_pkt[8];
        unsigned int rvd2[5];

	/* Vrat chunk */
        unsigned char vrat[4];
        unsigned int s_vrat;
        unsigned int v_rate;
        unsigned int size_in_pkts;

	/* Data chunk */
        unsigned char data[4];
        unsigned int s_data;
} __attribute__ ((packed));

/* Common part */
static struct qcp_header append_header = {
        {'R', 'I', 'F', 'F'}, 0, {'Q', 'L', 'C', 'M'}, /* Riff */
        {'f', 'm', 't', ' '}, 150, 1, 0, 0, 0, 0,{0}, 0, {0},0,0,160,8000,16,0,{0},{0},{0}, /* Fmt */
        {'v','r','a','t'}, 0, 0, 0, /* Vrat */
        {'d','a','t','a'},0 /* Data */
        };

#define QCP_HEADER_SIZE sizeof(struct qcp_header)

static int rec_type; // record type
static int rec_stop;
static int frame_format;
static int dtx_mode;
static int min_rate;
static int max_rate;
static int rec_source;

static struct msm_audio_evrc_enc_config evrccfg;
static struct msm_audio_qcelp_enc_config qcelpcfg;
static struct msm_audio_amrnb_enc_config_v2 amrnbcfg_v2;

#ifdef _ANDROID_
static const char *cmdfile = "/data/audio_test";
#else
static const char *cmdfile = "/tmp/audio_test";
#endif

static uint8_t qcelp_pkt_size[5] = {0x00, 0x03, 0x07, 0x10, 0x22};
static uint8_t evrc_pkt_size[5] = {0x00, 0x02, 0x00, 0xa, 0x16};
static uint8_t amrnb_pkt_size[8] = {0x0c, 0x0d, 0x0f, 0x11, 0x13, 0x20, 0x1a, 0x1f};


static void create_qcp_header(int Datasize, int Frames)
{
	append_header.s_riff = Datasize + QCP_HEADER_SIZE - 8; /* exclude riff id and size field */
	if( rec_type  == 1 ) { /* QCELP 13K */
		printf("QCELP 13K header\n");
		append_header.data1 = 0x5E7F6D41;
		append_header.data2 = 0xB115;
		append_header.data3 = 0x11D0;
		append_header.data4[0] = 0xBA;
		append_header.data4[1] = 0x91;
		append_header.data4[2] = 0x00;
		append_header.data4[3] = 0x80;
		append_header.data4[4] = 0x5F;
		append_header.data4[5] = 0xB4;
		append_header.data4[6] = 0xB9;
		append_header.data4[7] = 0x7E;
		append_header.ver = 0x0002;
		memcpy(append_header.name, "Qcelp 13K", 9);
		append_header.abps = 13000;
		append_header.bytes_per_pkt = 35;
		append_header.vr_num_of_rates = 5;
		append_header.vr_bytes_per_pkt[0] = 0x0422;
		append_header.vr_bytes_per_pkt[1] = 0x0310;
		append_header.vr_bytes_per_pkt[2] = 0x0207;
		append_header.vr_bytes_per_pkt[3] = 0x0103;
		append_header.s_vrat = 0x00000008;
		append_header.v_rate = 0x00000001;
		append_header.size_in_pkts = Frames;
	} else if ( rec_type   == 2) { /* EVRC */
		printf("EVRC header\n");
		append_header.data1 = 0xe689d48d;
		append_header.data2 = 0x9076;
		append_header.data3 = 0x46b5;
		append_header.data4[0] = 0x91;
		append_header.data4[1] = 0xef;
		append_header.data4[2] = 0x73;
		append_header.data4[3] = 0x6a;
		append_header.data4[4] = 0x51;
		append_header.data4[5] = 0x00;
		append_header.data4[6] = 0xce;
		append_header.data4[7] = 0xb4;
		append_header.ver = 0x0001;
		memcpy(append_header.name, "TIA IS-127 Enhanced Variable Rate Codec, Speech Service Option 3", 64);
		append_header.abps = 9600;
		append_header.bytes_per_pkt = 23;
		append_header.vr_num_of_rates = 4;
		append_header.vr_bytes_per_pkt[0] = 0x0416;
		append_header.vr_bytes_per_pkt[1] = 0x030a;
		append_header.vr_bytes_per_pkt[2] = 0x0200;
		append_header.vr_bytes_per_pkt[3] = 0x0102;
		append_header.s_vrat = 0x00000008;
		append_header.v_rate = 0x00000001;
		append_header.size_in_pkts = Frames;
	} else if( rec_type == 3 ) { /* AMRNB */
		printf("AMRNB header\n");
		append_header.data1 = 0x6aa8e053;
		append_header.data2 = 0x474f;
		append_header.data3 = 0xbd46;
		append_header.data4[0] = 0x8a;
		append_header.data4[1] = 0xfa;
		append_header.data4[2] = 0xac;
		append_header.data4[3] = 0xf2;
		append_header.data4[4] = 0x32;
		append_header.data4[5] = 0x82;
		append_header.data4[6] = 0x73;
		append_header.data4[7] = 0xbd;
		append_header.ver = 0x0001;
		memcpy(append_header.name, "AMR-NB   ", 9);
		append_header.abps = 0x9c31;
		append_header.bytes_per_pkt = 32;
		append_header.vr_num_of_rates = 8;
		append_header.vr_bytes_per_pkt[0] = 0x081f;
		append_header.vr_bytes_per_pkt[1] = 0x071b;
		append_header.vr_bytes_per_pkt[2] = 0x0614;
		append_header.vr_bytes_per_pkt[3] = 0x0513;
		append_header.vr_bytes_per_pkt[4] = 0x0411;
		append_header.vr_bytes_per_pkt[4] = 0x040f;
		append_header.vr_bytes_per_pkt[5] = 0x030d;
		append_header.vr_bytes_per_pkt[6] = 0x020c;
		append_header.s_vrat = 0x00000008;
		append_header.v_rate = 0x00000001;
		append_header.size_in_pkts = Frames;
	}
	append_header.s_data = Datasize;
        return;
}

static int voiceenc_start(struct audtest_config *clnt_config)
{
	int afd, fd, dfd=0;
	unsigned char tmp;
        unsigned char buf[1024];
        unsigned sz;
	int readcnt,writecnt;
	struct msm_audio_stream_config cfg;
	struct msm_audio_stats stats;
	int datasize=0, framecnt=0;
	unsigned short enc_id;
	int device_id = 0;
	int control = 0;
	const char *device = "handset_tx";
	memset(&stats,0,sizeof(stats));
	memset(&cfg,0,sizeof(cfg));

	fd = open(clnt_config->file_name, O_CREAT | O_RDWR, 0666);

	if (fd < 0) {
		printf("Unable to create output file = %s\n",
			clnt_config->file_name);
		goto file_err;
	}
	else
		printf("file created =%s\n",clnt_config->file_name);

	/* Open Device 	Node */
	if (rec_type == 1) {
		afd = open(QCELP_DEVICE_NODE, O_RDWR);
	} else if (rec_type == 2) {
		afd = open(EVRC_DEVICE_NODE, O_RDWR);
	} else if (rec_type == 3) {
		afd = open(AMRNB_DEVICE_NODE, O_RDWR);
	} else
		goto device_err;

	if (afd < 0) {
		printf("Unable to open audio device = %s\n",
			(rec_type == 1? QCELP_DEVICE_NODE:(rec_type == 2? \
				EVRC_DEVICE_NODE: AMRNB_DEVICE_NODE)));
		goto device_err;
	}

	if (rec_source > VOC_REC_BOTH ) {
		if (ioctl(afd, AUDIO_GET_SESSION_ID, &enc_id)) {
			perror("could not get encoder id\n");
			close(fd);
			close(afd);
			return -1;
		}
		device_id = enable_device_tx(device);
		if (device_id < 0) {
			close(fd);
			close(afd);
			return -1;
		}
		printf("device_id = %d\n", device_id);
		printf("enc_id = %d\n", enc_id);
	}
	/* Config param */
	if(ioctl(afd, AUDIO_GET_STREAM_CONFIG, &cfg)) {
		printf(" Error getting buf config param AUDIO_GET_CONFIG \n");
		goto fail;
	}

	if(ioctl(afd, AUDIO_SET_STREAM_CONFIG, &cfg)) {
		printf(" Error getting buf config param AUDIO_GET_CONFIG \n");
		goto fail;
	}
	printf("Default buffer size = 0x%8x\n", cfg.buffer_size);
	printf("Default buffer count = 0x%8x\n",cfg.buffer_count);

	if (rec_type == 1) {
		if (ioctl(afd, AUDIO_GET_QCELP_ENC_CONFIG, &qcelpcfg)) {
			printf("Error: AUDIO_GET_QCELP_ENC_CONFIG failed\n");
			goto fail;
		}
		printf("cdma rate = 0x%8x\n", qcelpcfg.cdma_rate);
		printf("min_bit_rate = 0x%8x\n", qcelpcfg.min_bit_rate);
		printf("max_bit_rate = 0x%8x\n", qcelpcfg.max_bit_rate);
		qcelpcfg.cdma_rate = max_rate;
		qcelpcfg.min_bit_rate = min_rate;
		qcelpcfg.max_bit_rate = max_rate;
		if (ioctl(afd, AUDIO_SET_QCELP_ENC_CONFIG, &qcelpcfg)) {
			printf("Error: AUDIO_SET_QCELP_ENC_CONFIG failed\n");
			goto fail;
		}
		printf("cdma rate = 0x%8x\n", qcelpcfg.cdma_rate);
		printf("min_bit_rate = 0x%8x\n", qcelpcfg.min_bit_rate);
		printf("max_bit_rate = 0x%8x\n", qcelpcfg.max_bit_rate);
	} else if(rec_type == 2) {

		if (ioctl(afd, AUDIO_GET_EVRC_ENC_CONFIG, &evrccfg)) {
			printf("Error: AUDIO_GET_EVRC_ENC_CONFIG failed\n");
			goto fail;
		}
		printf("cdma rate = 0x%8x\n", evrccfg.cdma_rate);
		printf("min_bit_rate = 0x%8x\n", evrccfg.min_bit_rate);
		printf("max_bit_rate = 0x%8x\n", evrccfg.max_bit_rate);
		evrccfg.cdma_rate = max_rate;
		evrccfg.min_bit_rate = min_rate;
		evrccfg.max_bit_rate = max_rate;
		if (ioctl(afd, AUDIO_SET_EVRC_ENC_CONFIG, &evrccfg)) {
			printf("Error: AUDIO_GET_EVRC_ENC_CONFIG failed\n");
			goto fail;
		}
		printf("cdma rate = 0x%8x\n", evrccfg.cdma_rate);
		printf("min_bit_rate = 0x%8x\n", evrccfg.min_bit_rate);
		printf("max_bit_rate = 0x%8x\n", evrccfg.max_bit_rate);
	} else if (rec_type == 3) {

		/* AMRNB specific settings */
		if (ioctl(afd, AUDIO_GET_AMRNB_ENC_CONFIG_V2, &amrnbcfg_v2)) {
			perror("Error: AUDIO_GET_AMRNB_ENC_CONFIG_V2 failed");
			goto fail;
		}
		printf("dtx mode = 0x%8x\n", amrnbcfg_v2.dtx_enable);
		printf("rate = 0x%8x\n", amrnbcfg_v2.band_mode);
		amrnbcfg_v2.dtx_enable = dtx_mode; /* 0 - DTX off, 1 - DTX on */
		amrnbcfg_v2.band_mode = max_rate;
		if (ioctl(afd, AUDIO_SET_AMRNB_ENC_CONFIG_V2, &amrnbcfg_v2)) {
			perror("Error: AUDIO_GET_AMRNB_ENC_CONFIG_V2 failed");
			goto fail;
		}
		printf("dtx mode = 0x%8x\n", amrnbcfg_v2.dtx_enable);
		printf("rate = 0x%8x\n", amrnbcfg_v2.band_mode);
	}

	/* Record form voice link */
	if (rec_source <= VOC_REC_BOTH ) {

		if (ioctl(afd, AUDIO_SET_INCALL, &rec_source)) {
			perror("Error: AUDIO_SET_INCALL failed");
			goto fail;
		}
		printf("rec source = 0x%8x\n", rec_source);
	}

	/* Store handle for commands pass*/
	clnt_config->private_data = (void *) afd;
	sz = cfg.buffer_size;

	ioctl(afd, AUDIO_START, 0);

	printf("Voice encoder started \n");

	if(frame_format == 1) { /* QCP file */
	        lseek(fd, QCP_HEADER_SIZE, SEEK_SET);
		printf("qcp_headsize %d\n",QCP_HEADER_SIZE);
		printf("QCP format\n");
	} else
		printf("DSP format\n");

	rec_stop = 0;
	while(!rec_stop) {
		memset(buf,0,sz);
		readcnt = read(afd, buf, sz);
                if (readcnt <= 0) {
                        printf("cannot read buffer error code =0x%8x", readcnt);
			goto fail;
                }
		else
		{
			unsigned char *memptr = buf;
			printf("read cnt %d\n",readcnt);
			/* QCP Format */
			if( frame_format == 1) {
				// logic for qcp generation
				if (rec_type ==  1) {
					if (buf[1] <= 4 || buf[1] >=1) {
						readcnt = qcelp_pkt_size[buf[1]] + 1;
						memptr = &buf[1];
						printf("0x%2x, %d\n", buf[1], readcnt);
					} else
						printf("Unexpected frame\n");
				} else if(rec_type == 2) {
					if ((buf[1] <= 4 || buf[1] >=1) && (buf[1] != 2)) {
						readcnt = evrc_pkt_size[buf[1]] + 1;
						memptr = &buf[1];
						printf("0x%2x, %d\n", buf[1], readcnt);
					} else
						printf("Unexpected frame\n");
				} else if(rec_type == 3) {
					if (buf[1] <= 7 || buf[1] >=0) {
						readcnt = amrnb_pkt_size[buf[1]] + 1;
						memptr = &buf[1];
						printf("0x%2x, %d\n", buf[1], readcnt);
					} else
						printf("Unexpected frame\n");
				 }
			}
			writecnt = write(fd, memptr, readcnt);
			if (writecnt <= 0) {
				printf("cannot write buffer error code =0x%8x", writecnt);
				goto fail;
			}
                }
		framecnt++;
		datasize += writecnt;
        }
done:
	ioctl(afd, AUDIO_GET_STATS, &stats);
	printf("\n read_bytes = %d, read_frame_counts = %d\n",datasize, framecnt);
	ioctl(afd, AUDIO_STOP, 0);
	if(frame_format == 1) { /* QCP file */
		create_qcp_header(datasize, framecnt);
	        lseek(fd, 0, SEEK_SET);
		write(fd, (char *)&append_header, QCP_HEADER_SIZE);
	}

	printf("Secondary encoder stopped \n");
	close(afd);
	close(fd);
	if (rec_source > VOC_REC_BOTH ) {
		disable_device_tx(device_id);
	}
	return 0;
fail:
	close(afd);
	if (rec_source > VOC_REC_BOTH ) {
		disable_device_tx(device_id);
	}
device_err:
	close(fd);
	unlink(clnt_config->file_name);
file_err:
	return -1;
}

void *voiceenc_thread(void *arg)
{
	struct audiotest_thread_context *context =
	    (struct audiotest_thread_context *)arg;
	int ret_val;

	ret_val = voiceenc_start(&context->config);
	free_context(context);
	pthread_exit((void *)ret_val);
	return NULL;
}

int voiceenc_read_params(void)
{
	struct audiotest_thread_context *context;
	char *token;
	int ret_val = 0;

	if ((context = get_free_context()) == NULL) {
		ret_val = -1;
	} else {
		#ifdef _ANDROID_
			context->config.file_name = "/data/sample.qcp";
		#else
			context->config.file_name = "/tmp/sample.qcp";
		#endif
		context->type = AUDIOTEST_TEST_MOD_VOICE_ENC;
		token = strtok(NULL, " ");
		rec_type = 0; /* qcelp */
		frame_format = 0;
		dtx_mode = 0;
		min_rate = 4;
		max_rate = 4;
		rec_source = 0;
		while (token != NULL) {
			if (!memcmp(token, "-id=", (sizeof("-id=") - 1))) {
				context->cxt_id =
				    atoi(&token[sizeof("-id=") - 1]);
			}else if (!memcmp(token, "-type=", (sizeof("-type=") - 1))) {
				rec_type =
				    atoi(&token[sizeof("-type=") - 1]);
			}else if (!memcmp(token, "-fmt=", (sizeof("-fmt=") - 1))) {
				frame_format =
				    atoi(&token[sizeof("-fmt=") - 1]);
			}else if (!memcmp(token, "-dtx=", (sizeof("-dtx=") - 1))) {
				dtx_mode =
				    atoi(&token[sizeof("-dtx=") - 1]);
			}else if (!memcmp(token, "min=", (sizeof("-min=") - 1))) {
				min_rate =
				    atoi(&token[sizeof("-min=") - 1]);
			}else if (!memcmp(token, "-max=", (sizeof("-max=") - 1))) {
				max_rate =
				    atoi(&token[sizeof("-max=") - 1]);
			}else if (!memcmp(token, "-src=", (sizeof("-src=") - 1))) {
				rec_source =
				    atoi(&token[sizeof("-src=") - 1]);
			} else if (!memcmp(token, "-out=",
                                        (sizeof("-out=") - 1))) {
                                context->config.file_name = token + (sizeof("-out=")-1);
			}
			token = strtok(NULL, " ");
		}
		pthread_create(&context->thread, NULL,
			       voiceenc_thread, (void *)context);
	}

	return ret_val;

}

int voiceenc_control_handler(void *private_data)
{
	int drvfd , ret_val = 0;
	char *token;

	token = strtok(NULL, " ");
	if ((private_data != NULL) && (token != NULL)) {
		drvfd = (int) private_data;
		if (!memcmp(token, "-cmd=", (sizeof("-cmd=") - 1))) {
			token = &token[sizeof("-cmd=") - 1];
			printf("%s: cmd %s\n", __FUNCTION__, token);
			if (!strcmp(token, "stop")) {
				rec_stop = 1;
			}
		}
	} else {
		ret_val = -1;
	}
	return ret_val;
}

const char *voiceenc_help_txt =
	"Voice encoder \n \
echo \"voiceenc -id=xxx -out=path_of_file -type=yy -fmt=zz -dtx=yy -min=zz -max=yy -src=zz\" > %s \n\
type: 1 - qcelp, 2 - evrc, 3 - amrnb \n \
fmt: 0 - dsp 1 - qcp \n \
src: 0 - Uplink 1 - Downlink, 2 - UL/DL, 3 - Mic \n \
bps: bit per second 64k to 384k \n \
examples: \n\
echo \"voiceenc -id=123 -out=path_of_file -type=3 -fmt=1 -dtx=0 -min=7 -max=7 -src=3\" > %s \n\
Supported control command: stop \n ";

void voiceenc_help_menu(void) {
	printf(voiceenc_help_txt, *cmdfile, *cmdfile);
}
