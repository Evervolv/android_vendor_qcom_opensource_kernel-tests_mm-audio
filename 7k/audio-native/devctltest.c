/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include "audiotest_def.h"
#include "control.h"

#ifdef AUDIOV2
#define DEVMGR_MAX_PLAYBACK_SESSION 8
#define DEVMGR_DEFAULT_SID 65523

static int devmgr_devid;
static int devmgr_dev_count;
static int devmgr_init_flag;
static int devmgr_mixer_init_flag;
unsigned short devmgr_sid_array[DEVMGR_MAX_PLAYBACK_SESSION];
static int devmgr_sid_count;

const char *devctl_help_text =
"\n\Device Control Help: MAINLY USED FOR SWITCHING THE AUDIO DEVICE.\n\
All Active playbacks will be routed to Device mentioned in this     \n\
command. Device IDs are generated dynamically from the driver.      \n\
Usage: echo \"devctl -cmd=dev_switch_rx -dev_id=x\" > /data/audio_test \n\
       echo \"devctl -cmd=exit\" > /data/audio_test                 \n\
where x = any of the supported device IDs listed below,             \n\
exit command de-inits the device manager and destroys the thread    \n\
Note:                                                               \n\
(i)   Handset RX is set as default device for all playbacks         \n\
(ii)  After a device switch, audio will be routed to the last set   \n\
      device                                                        \n\
(iii) Device List and their corresponding IDs can be got using      \n\
      \"mm-audio-native-test -format devctl\" and also is displayed \n\
      during beginning of any playback session                      \n\
(iv)  If you gave \"exit\" command, it will route all active        \n\
      streams to the default device, then again you can change      \n\
      the device                                                    \n\
(v)   Recommended usage of this command is during the media         \n\
      playbacks";

void devctl_help_menu(void)
{

	int i, alsa_ctl, dev_cnt, device_id;
	const char **device_names;

	printf("%s\n", devctl_help_text);
	if (!devmgr_mixer_init_flag) {
		alsa_ctl = msm_mixer_open("/dev/snd/controlC0", 0);
		if (alsa_ctl < 0)
			perror("Fail to open ALSA MIXER\n");
		else
			devmgr_mixer_init_flag = 1;
	}
	if (devmgr_mixer_init_flag) {
		dev_cnt = msm_get_device_count();
		device_names = msm_get_device_list();
		for (i = 0; i < dev_cnt;) {
			device_id = msm_get_device(device_names[i]);
			if (device_id >= 0)
				printf("device name %s:dev_id: %d\n",
							device_names[i],
			device_id);
			i++;
		}
	}
}

int devmgr_disable_device(int dev_id)
{

	devmgr_dev_count--;
	if (devmgr_dev_count == 0) {
		if (msm_en_device(dev_id, 0) < 0)
			return -1;
	}
	return 0;
}

int devmgr_enable_device(int dev_id)
{

	if (msm_en_device(dev_id, 1) < 0)
		return -1;
	devmgr_dev_count++;
	return 0;
}

int devmgr_register_session(unsigned short session_id)
{

	printf("devmgr_register_session: Registering Session ID = %d\n",
								session_id);
	printf("%s\n", devctl_help_text);
	if ((devmgr_sid_count < DEVMGR_MAX_PLAYBACK_SESSION) &&
		(devmgr_sid_array[devmgr_sid_count] == DEVMGR_DEFAULT_SID))
		devmgr_sid_array[devmgr_sid_count++] = session_id;

	if (msm_route_stream(1, session_id, devmgr_devid, 1) < 0) {
		perror("could not route stream to Device\n");
		if (devmgr_disable_device(devmgr_devid) < 0)
			perror("could not disable device\n");
		return -1;
	}
	return 0;
}

int devmgr_unregister_session(unsigned short session_id)
{

	int index = 0;
	printf("devmgr_unregister_session: Unregistering Session ID = %d\n",
	session_id);
	while (index < devmgr_sid_count) {
		if (session_id == devmgr_sid_array[index])
			break;
		index++;
	}
	while (index < (devmgr_sid_count-1)) {
		devmgr_sid_array[index]  =  devmgr_sid_array[index+1];
		index++;
	}
	/* Reset the last entry */
	devmgr_sid_array[index]         = DEVMGR_DEFAULT_SID;
	devmgr_sid_count--;

	if (msm_route_stream(1, session_id, devmgr_devid, 0) < 0)
		perror("could not de-route stream to Device\n");
	return 0;
}

void audiotest_deinit_devmgr(void)
{

	int index, dev_source, dev_dest;
	const char *def_device = "handset_rx";

	if (devmgr_init_flag && devmgr_mixer_init_flag) {
		dev_source = devmgr_devid;
		dev_dest = msm_get_device(def_device);
		if (dev_source != dev_dest) {
			devmgr_devid = dev_dest;
			for (index = 0; index < devmgr_sid_count; index++) {
				msm_route_stream(1, devmgr_sid_array[index],
								 dev_dest, 1);
				if ((devmgr_disable_device(dev_source)) == 0) {
					if ((devmgr_enable_device(dev_dest))
									== 0) {
						printf("%s: Routing all streams to default device \n",__func__);
					}
				}
			}
		}
	}
}

void audiotest_init_devmgr(void)
{

	int i, alsa_ctl, dev_cnt, device_id;
	const char **device_names;
	const char *def_device = "handset_rx";

	if (!devmgr_mixer_init_flag) {
		alsa_ctl = msm_mixer_open("/dev/snd/controlC0", 0);
		if (alsa_ctl < 0)
			perror("Fail to open ALSA MIXER\n");
		else
			devmgr_mixer_init_flag = 1;
	}
	if (devmgr_mixer_init_flag) {
		printf("Device Manager: List of Devices supported: \n");
		dev_cnt = msm_get_device_count();
		device_names = msm_get_device_list();
		for (i = 0; i < dev_cnt;) {
			device_id = msm_get_device(device_names[i]);
			if (device_id >= 0)
				printf("device name %s:dev_id: %d\n", device_names[i], device_id);
			i++;
		}
		devmgr_devid = msm_get_device(def_device);
		printf("Enabling Device = %d\n", devmgr_devid);
		if (devmgr_enable_device(devmgr_devid) < 0) {
			perror("could not enable device\n");
			return;
		}
		for (i = 0; i < DEVMGR_MAX_PLAYBACK_SESSION; i++)
			devmgr_sid_array[i] = DEVMGR_DEFAULT_SID;
		devmgr_init_flag = 1;
	}
    return;
}

int devmgr_devctl_handler()
{

	char *token;
	int ret_val = 0, sid, dev_cnt, dev_source, dev_dest, index, dev_id,
		txdev_id, rxdev_id;

	token = strtok(NULL, " ");

	if (token != NULL) {
		if (!memcmp(token, "-cmd=", (sizeof("-cmd=") - 1))) {
			token = &token[sizeof("-cmd=") - 1];
			if (!strcmp(token, "dev_switch_rx")) {
				token = strtok(NULL, " ");
				if (!memcmp(token, "-dev_id=",
						(sizeof("-dev_id=") - 1))) {
					dev_dest = atoi(&token
						[sizeof("-dev_id=") - 1]);
					dev_source = devmgr_devid;
					if (devmgr_mixer_init_flag &&
							devmgr_init_flag) {
						if (dev_source != dev_dest) {
							devmgr_devid = dev_dest;
							printf("%s: Device Switch from = %d to = %d\n",	__func__, dev_source, dev_dest);
							for (index = 0;
						 index < devmgr_sid_count;
								 index++) {
								msm_route_stream
						(1, devmgr_sid_array[index],
								dev_dest, 1);
								if (
						(devmgr_disable_device
							(dev_source)) == 0) {
									if (
						(devmgr_enable_device(dev_dest)
							) == 0) {
					printf("%s: Device Switch Success\n",__func__);
									}
								}
							}
						} else {
							printf("%s(): Device has not changed as current device is:%d\n", __func__, devmgr_devid);
						}
					}
				}
			} else if (!strcmp(token, "enable_dev")) {
				token = strtok(NULL, " ");
				if (!memcmp(token, "-dev_id=", (sizeof
					("-dev_id=") - 1))) {
					dev_id = atoi(&token[sizeof("-dev_id=")
									- 1]);
					msm_en_device(dev_id, 1);
				}
			} else if (!strcmp(token, "disable_dev")) {
				token = strtok(NULL, " ");
				if (!memcmp(token, "-dev_id=", (sizeof
					("-dev_id=") - 1))) {
					dev_id = atoi(&token[sizeof("-dev_id=")
									- 1]);
					msm_en_device(dev_id, 0);
				}
			} else if (!strcmp(token, "rx_route")) {
				token = strtok(NULL, " ");
				if (!memcmp(token, "-dev_id=", (sizeof
					("-dev_id=") - 1))) {
					dev_id = atoi(&token[sizeof("-dev_id=")
									- 1]);
					token = strtok(NULL, " ");
					if (!memcmp(token, "-sid=", (sizeof
							("-sid=") - 1))) {
						sid = atoi(&token[sizeof
							("-sid=") - 1]);
						msm_route_stream
							(1, sid, dev_id, 1);
					}
				}
			} else if (!strcmp(token, "rx_deroute")) {
				token = strtok(NULL, " ");
				if (!memcmp(token, "-dev_id=", (sizeof
							("-dev_id=") - 1))) {
					dev_id = atoi(&token[sizeof("-dev_id=")
									- 1]);
					token = strtok(NULL, " ");
					if (!memcmp(token, "-sid=", (sizeof
							("-sid=") - 1))) {
						sid = atoi(&token[sizeof
							("-sid=") - 1]);
						msm_route_stream
							(1, sid, dev_id, 0);
					}
				}
			} else if (!strcmp(token, "voice_route")) {
				token = strtok(NULL, " ");
				if (!memcmp(token, "-txdev_id=", (sizeof
							("-txdev_id=") - 1))) {
					txdev_id = atoi(&token[sizeof
							("-txdev_id=") - 1]);
					token = strtok(NULL, " ");
					if (!memcmp(token, "-rxdev_id=",
						(sizeof("-rxdev_id=") - 1))) {
						rxdev_id = atoi(&token[sizeof
							("-rxdev_id=") - 1]);
						msm_route_voice
							(rxdev_id, txdev_id, 1);
					}
				}
			} else if (!strcmp(token, "voice_deroute")) {
				token = strtok(NULL, " ");
				if (!memcmp(token, "-txdev_id=", (sizeof
							("-txdev_id=") - 1))) {
					txdev_id = atoi(&token[sizeof
							("-txdev_id=") - 1]);
					token = strtok(NULL, " ");
					if (!memcmp(token, "-rxdev_id=",
						(sizeof("-rxdev_id=") - 1))) {
						rxdev_id = atoi(&token[sizeof
							("-rxdev_id=") - 1]);
						msm_route_voice
							(txdev_id, rxdev_id, 0);
					}
				}
			} else if (!strcmp(token, "voice_rx_vol")) {
				token = strtok(NULL, " ");
				if (!memcmp(token, "-volume=", (sizeof
							("-volume=") - 1))) {
					int volume = atoi(&token[sizeof
							("-volume=") - 1]);
					msm_set_voice_rx_vol(volume);
				}
			} else if (!strcmp(token, "exit")) {
				audiotest_deinit_devmgr();
			} else {
				printf("%s: Invalid command", __func__);
				printf("%s\n", devctl_help_text);
				ret_val = -1;
			}
		} else {
			printf("%s: Not a devmgr command\n", __func__);
			printf("%s\n", devctl_help_text);
			ret_val = -1;
		}
	}

	return ret_val;
}

int devctl_read_params(void)
{

	if (devmgr_devctl_handler() < 0) {
		printf("%s() Invalid Command\n", __func__);
		return -1;
	}
	return 0;
}
#endif


