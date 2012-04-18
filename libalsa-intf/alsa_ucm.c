/*
 * Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
#define LOG_TAG "alsa_ucm"
#define LOG_NDDEBUG 0
#include <utils/Log.h>
#include <cutils/properties.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/poll.h>

#include <linux/ioctl.h>
#include "msm8960_use_cases.h"
#if defined(QC_PROP)
    #include "acdb-loader.h"
#else
    #define acdb_loader_init_ACDB() (-EPERM)
    #define acdb_loader_deallocate_ACDB() (-EPERM)
    #define acdb_loader_send_voice_cal(rxacdb_id, txacdb_id) (-EPERM)
    #define acdb_loader_send_audio_cal(acdb_id, capability) (-EPERM)
    #define acdb_loader_send_anc_cal(acdb_id) (-EPERM)
#endif
#define PARSE_DEBUG 0

/**
 * Create an identifier
 * fmt - sprintf like format,
 * ... - Optional arguments
 * returns - string allocated or NULL on error
 */
char *snd_use_case_identifier(const char *fmt, ...)
{
    LOGE("API not implemented for now, to be updated if required");
    return NULL;
}

/**
 * Free a list
 * list - list to free
 * items -  Count of strings
 * Return Zero on success, otherwise a negative error code
 */
int snd_use_case_free_list(const char *list[], int items)
{
    /* list points to UCM internal static tables,
     * hence there is no need to do a free call
     * just set the list to NULL and return */
    list = NULL;
    return 0;
}

/**
 * Obtain a list of entries
 * uc_mgr - UCM structure pointer or  NULL for card list
 * identifier - NULL for card list
 * list - Returns allocated list
 * returns Number of list entries on success, otherwise a negative error code
 */
int snd_use_case_get_list(snd_use_case_mgr_t *uc_mgr,
                          const char *identifier,
                          const char **list[])
{
    int verb_index, list_size, index = 0;

    if (identifier == NULL) {
        *list = card_list;
        return ((int)MAX_NUM_CARDS);
    }

    pthread_mutex_lock(&uc_mgr->card_ctxt_ptr->card_lock);
    if ((uc_mgr->snd_card_index >= (int)MAX_NUM_CARDS) ||
        (uc_mgr->snd_card_index < 0) || (uc_mgr->card_ctxt_ptr == NULL)) {
        LOGE("snd_use_case_get_list(): failed, invalid arguments");
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return -EINVAL;
    }

    if (!strncmp(identifier, "_verbs", 6)) {
        while(strncmp(uc_mgr->card_ctxt_ptr->verb_list[index], SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))) {
            LOGV("Index:%d Verb:%s", index, uc_mgr->card_ctxt_ptr->verb_list[index]);
            index++;
        }
        *list = (char ***)uc_mgr->card_ctxt_ptr->verb_list;
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return index;
    } else  if (!strncmp(identifier, "_devices", 8)) {
        if (!strncmp(uc_mgr->card_ctxt_ptr->current_verb,
                   SND_USE_CASE_VERB_INACTIVE, strlen(SND_USE_CASE_VERB_INACTIVE))) {
            LOGE("Use case verb name not set, invalid current verb");
            pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
            return -EINVAL;
        }
        while(strncmp(uc_mgr->card_ctxt_ptr->current_verb,
            uc_mgr->card_ctxt_ptr->use_case_verb_list[index].use_case_name,
            (strlen(uc_mgr->card_ctxt_ptr->use_case_verb_list[index].use_case_name)+1))) {
            index++;
        }
        verb_index = index;
        index = 0;
        while(strncmp(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].device_list[index],
                    SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))) {
            LOGV("Index:%d Device:%s", index,
                uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].device_list[index]);
            index++;
        }
        *list = uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].device_list;
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return index;
    } else  if (!strncmp(identifier, "_modifiers", 10)) {
        if (!strncmp(uc_mgr->card_ctxt_ptr->current_verb,
                    SND_USE_CASE_VERB_INACTIVE, strlen(SND_USE_CASE_VERB_INACTIVE))) {
            LOGE("Use case verb name not set, invalid current verb");
            pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
            return -EINVAL;
        }
        while(strncmp(uc_mgr->card_ctxt_ptr->current_verb,
            uc_mgr->card_ctxt_ptr->use_case_verb_list[index].use_case_name,
            (strlen(uc_mgr->card_ctxt_ptr->use_case_verb_list[index].use_case_name)+1))) {
            index++;
        }
        verb_index = index;
        index = 0;
        while(strncmp(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].modifier_list[index],
                    SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))) {
            LOGV("Index:%d Modifier:%s", index,
                uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].modifier_list[index]);
            index++;
        }
        *list = uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].modifier_list;
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return index;
    } else  if (!strncmp(identifier, "_enadevs", 8)) {
        if (uc_mgr->device_list_count) {
            for (index = 0; index < uc_mgr->device_list_count; index++) {
                free(uc_mgr->current_device_list[index]);
                uc_mgr->current_device_list[index] = NULL;
            }
            free(uc_mgr->current_device_list);
            uc_mgr->current_device_list = NULL;
            uc_mgr->device_list_count = 0;
        }
        list_size = snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->dev_list_head);
        uc_mgr->device_list_count = list_size;
	if (list_size > 0) {
            uc_mgr->current_device_list = (char **)malloc(sizeof(char *)*list_size);
            if (uc_mgr->current_device_list == NULL) {
                *list = NULL;
                pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
                return -ENOMEM;
            }
            for (index = 0; index < list_size; index++) {
                uc_mgr->current_device_list[index] =
                            snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->dev_list_head, index);
            }
        }
        *list = (const char **)uc_mgr->current_device_list;
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return (list_size);
    } else  if (!strncmp(identifier, "_enamods", 8)) {
        if (uc_mgr->modifier_list_count) {
            for (index = 0; index < uc_mgr->modifier_list_count; index++) {
                free(uc_mgr->current_modifier_list[index]);
                uc_mgr->current_modifier_list[index] = NULL;
            }
            free(uc_mgr->current_modifier_list);
            uc_mgr->current_modifier_list = NULL;
            uc_mgr->modifier_list_count = 0;
        }
        list_size = snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->mod_list_head);
        uc_mgr->modifier_list_count = list_size;
	if (list_size > 0) {
            uc_mgr->current_modifier_list = (char **)malloc(sizeof(char *) * list_size);
            if (uc_mgr->current_modifier_list == NULL) {
                *list = NULL;
                pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
                return -ENOMEM;
            }
            for (index = 0; index < list_size; index++) {
                uc_mgr->current_modifier_list[index] =
                            snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->mod_list_head, index);
            }
        }
        *list = (const char **)uc_mgr->current_modifier_list;
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return (list_size);
    } else {
        LOGE("Invalid identifier: %s", identifier);
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return -EINVAL;
    }
}


/**
 * Get current value of the identifier
 * identifier - NULL for current card
 *        _verb
 *        <Name>/<_device/_modifier>
 *    Name -    PlaybackPCM
 *        CapturePCM
 *        PlaybackCTL
 *        CaptureCTL
 * value - Value pointer
 * returns Zero if success, otherwise a negative error code
 */
int snd_use_case_get(snd_use_case_mgr_t *uc_mgr,
                     const char *identifier,
                     const char **value)
{
    char ident[MAX_STR_LEN], *ident1, *ident2, *temp_ptr;
    int index, verb_index = 0, ret = 0;

    pthread_mutex_lock(&uc_mgr->card_ctxt_ptr->card_lock);
    if ((uc_mgr->snd_card_index >= (int)MAX_NUM_CARDS) ||
        (uc_mgr->snd_card_index < 0) || (uc_mgr->card_ctxt_ptr == NULL)) {
        LOGE("snd_use_case_get(): failed, invalid arguments");
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return -EINVAL;
    }

    if (identifier == NULL) {
        if (uc_mgr->card_ctxt_ptr->card_name != NULL) {
            *value = strdup(uc_mgr->card_ctxt_ptr->card_name);
        } else {
            *value = NULL;
        }
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return 0;
    }

    if (!strncmp(identifier, "_verb", 5)) {
        if (uc_mgr->card_ctxt_ptr->current_verb != NULL) {
            *value = strdup(uc_mgr->card_ctxt_ptr->current_verb);
        } else {
            *value = NULL;
        }
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return 0;
    }

    strlcpy(ident, identifier, sizeof(ident));
    if(!(ident1 = strtok_r(ident, "/", &temp_ptr))) {
        LOGE("No valid identifier found: %s", ident);
        ret = -EINVAL;
    } else {
        if ((!strncmp(ident1, "PlaybackPCM", 11)) || (!strncmp(ident1, "CapturePCM", 10))) {
            ident2 = strtok_r(NULL, "/", &temp_ptr);
            index = 0;
            verb_index = uc_mgr->card_ctxt_ptr->current_verb_index;
            if((verb_index < 0) || (!strncmp(uc_mgr->card_ctxt_ptr->current_verb, SND_UCM_END_OF_LIST, 3)) ||
               (uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl == NULL)) {
                LOGE("Invalid current verb value: %s - %d", uc_mgr->card_ctxt_ptr->current_verb, verb_index);
                pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
                return -EINVAL;
            }
            while(strncmp(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].case_name, ident2, (strlen(ident2)+1))) {
                if (!strncmp(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].case_name,
                        SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))){
                    *value = NULL;
                    ret = -EINVAL;
                    break;
                } else {
                    index++;
                }
            }
            if (ret < 0) {
                LOGE("No valid device/modifier found with given identifier: %s", ident2);
            } else {
                if(!strncmp(ident1, "PlaybackPCM", 11)) {
                    if (uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].playback_dev_name) {
                        *value = strdup(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].playback_dev_name);
                    } else {
                        *value = NULL;
                        ret = -ENODEV;
                    }
                } else if(!strncmp(ident1, "CapturePCM", 10)) {
                    if (uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].capture_dev_name) {
                        *value = strdup(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].capture_dev_name);
                    } else {
                        *value = NULL;
                        ret = -ENODEV;
                    }
                } else {
                    LOGE("No valid device name exists for given identifier: %s", ident2);
                    *value = NULL;
                    ret = -ENODEV;
                }
            }
        } else if ((!strncmp(ident1, "PlaybackCTL", 11)) || (!strncmp(ident1, "CaptureCTL", 10))) {
            if(uc_mgr->card_ctxt_ptr->control_device != NULL) {
                *value = strdup(uc_mgr->card_ctxt_ptr->control_device);
            } else {
                LOGE("No valid control device found");
                *value = NULL;
                ret = -ENODEV;
            }
        } else {
            LOGE("Unsupported identifier value: %s", ident1);
            *value = NULL;
            ret = -EINVAL;
        }
    }
    pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
    return ret;
}

/**
 * Get current status
 * uc_mgr - UCM structure
 * identifier - _devstatus/<device>,
        _modstatus/<modifier>
 * value - result
 * returns 0 on success, otherwise a negative error code
 */
int snd_use_case_geti(snd_use_case_mgr_t *uc_mgr,
              const char *identifier,
              long *value)
{
    char ident[MAX_STR_LEN], *ident1, *ident2, *ident_value, *temp_ptr;
    int index, list_size, ret = -EINVAL;

    pthread_mutex_lock(&uc_mgr->card_ctxt_ptr->card_lock);
    if ((uc_mgr->snd_card_index >= (int)MAX_NUM_CARDS) ||
        (uc_mgr->snd_card_index < 0) || (uc_mgr->card_ctxt_ptr == NULL)) {
        LOGE("snd_use_case_geti(): failed, invalid arguments");
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return -EINVAL;
    }

    *value = 0;
    strlcpy(ident, identifier, sizeof(ident));
    if(!(ident1 = strtok_r(ident, "/", &temp_ptr))) {
        LOGE("No valid identifier found: %s", ident);
        ret = -EINVAL;
    } else {
        if (!strncmp(ident1, "_devstatus", 10)) {
            ident2 = strtok_r(NULL, "/", &temp_ptr);
            list_size = snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->dev_list_head);
            for (index = 0; index < list_size; index++) {
                ident_value = snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->dev_list_head, index);
                if (!strncmp(ident2, ident_value, (strlen(ident_value)+1))) {
                    *value = 1;
                    free(ident_value);
                    ident_value = NULL;
                    break;
                } else {
                    free(ident_value);
                    ident_value = NULL;
                }
            }
            ret = 0;
        } else if (!strncmp(ident1, "_modstatus", 10)) {
            ident2 = strtok_r(NULL, "/", &temp_ptr);
            list_size = snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->mod_list_head);
            for (index = 0; index < list_size; index++) {
                ident_value = snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->mod_list_head, index);
                if (!strncmp(ident2, ident_value, (strlen(ident_value)+1))) {
                    *value = 1;
                    free(ident_value);
                    ident_value = NULL;
                    break;
                } else {
                    free(ident_value);
                    ident_value = NULL;
                }
            }
            ret = 0;
        } else {
            LOGE("Unknown identifier: %s", ident1);
        }
    }
    pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
    return ret;
}

static int snd_use_case_apply_voice_acdb(snd_use_case_mgr_t *uc_mgr, int use_case_index)
{
    int list_size, index, verb_index, ret = 0, voice_acdb = 0, rx_id, tx_id;
    char *ident_value;

    /* Check if voice call use case/modifier exists */
    if ((!strncmp(uc_mgr->card_ctxt_ptr->current_verb, SND_USE_CASE_VERB_VOICECALL, strlen(SND_USE_CASE_VERB_VOICECALL))) ||
       (!strncmp(uc_mgr->card_ctxt_ptr->current_verb, SND_USE_CASE_VERB_IP_VOICECALL, strlen(SND_USE_CASE_VERB_IP_VOICECALL)))) {
        voice_acdb = 1;
    }
    if (voice_acdb != 1) {
        list_size = snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->mod_list_head);
        for (index = 0; index < list_size; index++) {
            ident_value = snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->mod_list_head, index);
            if ((!strncmp(ident_value, SND_USE_CASE_MOD_PLAY_VOICE, strlen(SND_USE_CASE_MOD_PLAY_VOICE))) ||
                (!strncmp(ident_value, SND_USE_CASE_MOD_PLAY_VOIP, strlen(SND_USE_CASE_MOD_PLAY_VOIP)))) {
                voice_acdb = 1;
                free(ident_value);
                break;
            }
            free(ident_value);
        }
    }

    verb_index = uc_mgr->card_ctxt_ptr->current_verb_index;
    if((verb_index < 0) || (!strncmp(uc_mgr->card_ctxt_ptr->current_verb, SND_UCM_END_OF_LIST, 3))) {
        LOGE("Invalid current verb value: %s - %d", uc_mgr->card_ctxt_ptr->current_verb, verb_index);
        return -EINVAL;
    }
    if (voice_acdb == 1) {
        list_size = snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->dev_list_head);
        for (index = 0; index < list_size; index++) {
            ident_value = snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->dev_list_head, index);
            if (strncmp(ident_value, uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].case_name,
                (strlen(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].case_name)+1))) {
                break;
            }
            free(ident_value);
            ident_value = NULL;
        }
        index = 0;
        if (ident_value != NULL) {
            while(strncmp(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].case_name, ident_value, (strlen(ident_value)+1))) {
                if (!strncmp(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].case_name,
                        SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))) {
                    ret = -EINVAL;
                    break;
                }
                index++;
            }
            if (ret < 0) {
                LOGE("No valid device found: %s",ident_value);
            } else {
                if (uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].capability == CAP_RX) {
                    rx_id = uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].acdb_id;
                    tx_id = uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].acdb_id;
                } else {
                    rx_id = uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].acdb_id;
                    tx_id = uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].acdb_id;
                }
                if(rx_id == DEVICE_SPEAKER_RX_ACDB_ID && tx_id == DEVICE_HANDSET_TX_ACDB_ID) {
                    tx_id = DEVICE_SPEAKER_TX_ACDB_ID;
                }
                if ((rx_id != uc_mgr->current_rx_device) ||
                    (tx_id != uc_mgr->current_tx_device)) {
                    uc_mgr->current_rx_device = rx_id; uc_mgr->current_tx_device = tx_id;
                    LOGD("Voice acdb: rx id %d tx id %d", uc_mgr->current_rx_device,
                          uc_mgr->current_tx_device);
                    acdb_loader_send_voice_cal(uc_mgr->current_rx_device, uc_mgr->current_tx_device);
                } else {
                    LOGV("Voice acdb: Required acdb already pushed rx id %d tx id %d",
                         uc_mgr->current_rx_device, uc_mgr->current_tx_device);
                }
            }
            free(ident_value);
        }
    } else {
        LOGV("No voice use case found");
        uc_mgr->current_rx_device = -1; uc_mgr->current_tx_device = -1;
        ret = -ENODEV;
    }
    return ret;
}

int get_use_case_index(snd_use_case_mgr_t *uc_mgr, const char *use_case)
{
    int ret = 0, index = 0, verb_index;

    verb_index = uc_mgr->card_ctxt_ptr->current_verb_index;
    if((verb_index < 0) || (!strncmp(uc_mgr->card_ctxt_ptr->current_verb, SND_UCM_END_OF_LIST, 3)) ||
       (uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl == NULL) ||
       (uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].case_name == NULL)) {
        LOGE("Invalid current verb value: %s - %d", uc_mgr->card_ctxt_ptr->current_verb, verb_index);
        return -EINVAL;
    }
    while(strncmp(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].case_name,
                use_case, (strlen(use_case)+1))) {
        if (!strncmp(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].case_name,
                SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))) {
            ret = -EINVAL;
            break;
        }
        index++;
        if (uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].case_name == NULL) {
            LOGE("Invalid case_name at index %d", index);
            index++;
            while(1) {
                if (uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].case_name == NULL) {
                    index++;
                    if (index == (uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].use_case_count+1))
                        break;
                    else
                        continue;
                } else {
                    break;
                }
            }
        }
        if ((index == (uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].use_case_count+1)) ||
            (uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].case_name == NULL)) {
            ret = -EINVAL;
            break;
        }
    }
    if (ret < 0) {
        return ret;
    } else {
        return index;
    }
}

/* Apply the required mixer controls for specific use case
 * uc_mgr - UCM structure pointer
 * use_case - use case name
 * return 0 on sucess, otherwise a negative error code
 */
int snd_use_case_apply_mixer_controls(snd_use_case_mgr_t *uc_mgr,
                const char *use_case, int enable)
{
    mixer_control_t *mixer_list;
    struct mixer_ctl *ctl;
    int i, ret = 0, index = 0, verb_index, use_case_index, mixer_count;

    verb_index = uc_mgr->card_ctxt_ptr->current_verb_index;
    if((verb_index < 0) || (!strncmp(uc_mgr->card_ctxt_ptr->current_verb, SND_UCM_END_OF_LIST, 3)) ||
       (uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl == NULL)) {
        LOGE("Invalid current verb value: %s - %d", uc_mgr->card_ctxt_ptr->current_verb, verb_index);
        return -EINVAL;
    }
    if ((use_case_index = get_use_case_index(uc_mgr, use_case)) < 0) {
        LOGE("No valid use case found with the use case: %s", use_case);
        ret = -ENODEV;
    } else {
        if (!uc_mgr->card_ctxt_ptr->mixer_handle) {
            LOGE("Control device not initialized");
            ret = -ENODEV;
        } else {
            LOGD("Set mixer controls for %s enable %d", use_case, enable);
            if (uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].acdb_id &&
                uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].capability) {
                /*TODO: Check for voice call use case or modifier and set voice calibration*/
                if (enable) {
                    if (snd_use_case_apply_voice_acdb(uc_mgr, use_case_index)) {
                        LOGD("acdb_id %d cap %d enable %d", uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].acdb_id,
                            uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].capability, enable);
                        acdb_loader_send_audio_cal(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].acdb_id,
                            uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].capability);
                    }
                }
            }
            if (enable) {
                mixer_list = uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].ena_mixer_list;
                mixer_count = uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].ena_mixer_count;
            } else {
                mixer_list = uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].dis_mixer_list;
                mixer_count = uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].dis_mixer_count;
            }
            for(index = 0; index < mixer_count; index++) {
                if (mixer_list == NULL) {
                    LOGE("No valid controls available for this case: %s", use_case);
                    break;
                }
                ctl = mixer_get_control(uc_mgr->card_ctxt_ptr->mixer_handle,
                          mixer_list[index].control_name, 0);
                if (ctl) {
                    if (mixer_list[index].type == TYPE_INT) {
                        LOGD("Setting mixer control: %s, value: %d",
                             mixer_list[index].control_name, mixer_list[index].value);
                        ret = mixer_ctl_set(ctl, mixer_list[index].value);
                    } else if (mixer_list[index].type == TYPE_MULTI_VAL) {
                        LOGD("Setting multi value: %s", mixer_list[index].control_name);
                        ret = mixer_ctl_set_value(ctl, mixer_list[index].value, mixer_list[index].mulval);
                        if (ret < 0)
                            LOGE("Failed to set multi value control %s\n", mixer_list[index].control_name);
                    } else {
                        LOGD("Setting mixer control: %s, value: %s",
                            mixer_list[index].control_name, mixer_list[index].string);
                        ret = mixer_ctl_select(ctl, mixer_list[index].string);
                    }
                    if ((ret != 0) && enable) {
                       /* Disable all the mixer controls which are already enabled before failure */
                       mixer_list = uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].dis_mixer_list;
                       mixer_count = uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[use_case_index].dis_mixer_count;
                       for(i = 0; i < mixer_count; i++) {
                           ctl = mixer_get_control(uc_mgr->card_ctxt_ptr->mixer_handle,
                                mixer_list[i].control_name, 0);
                           if (ctl) {
                               if (mixer_list[i].type == TYPE_INT) {
                                   ret = mixer_ctl_set(ctl, mixer_list[i].value);
                               } else {
                                   ret = mixer_ctl_select(ctl, mixer_list[i].string);
                               }
                           }
                       }
                       LOGE("Failed to enable the mixer controls for %s", use_case);
                       break;
                    }
                }
            }
        }
    }
    return ret;
}

/* Set/Reset mixer controls of specific use case for all current devices
 * uc_mgr - UCM structure pointer
 * ident  - use case name (verb or modifier)
 * enable - 1 for enable and 0 for disable
 * return 0 on sucess, otherwise a negative error code
 */
static int snd_use_case_ident_set_controls_for_all_devices(snd_use_case_mgr_t *uc_mgr,
    const char *ident, int enable)
{
    char *current_device, use_case[MAX_UC_LEN];
    int list_size, index, ret = 0;

    LOGV("set_use_case_ident_for_all_devices(): %s", ident);
    if (snd_use_case_apply_mixer_controls(uc_mgr, ident, enable) < 0) {
        LOGV("use case %s not valid without device combination", ident);
    }
    list_size = snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->dev_list_head);
    for (index = 0; index < list_size; index++) {
        current_device = snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->dev_list_head, index);
        if (current_device != NULL) {
            strlcpy(use_case, ident, sizeof(use_case));
            strlcat(use_case, current_device, sizeof(use_case));
            LOGV("Applying mixer controls for use case: %s", use_case);
            if ((get_use_case_index(uc_mgr, use_case)) < 0) {
                LOGV("No valid use case found: %s", use_case);
            } else {
                if (enable) {
                    ret = snd_use_case_apply_mixer_controls(uc_mgr, current_device, enable);
                    if (!ret)
                        snd_ucm_set_status_at_index(uc_mgr->card_ctxt_ptr->dev_list_head, current_device, enable);
                }
                ret = snd_use_case_apply_mixer_controls(uc_mgr, use_case, enable);
            }
            use_case[0] = 0;
            free(current_device);
        }
    }
    return ret;
}

/* Set/Reset mixer controls of specific device for all use cases
 * uc_mgr - UCM structure pointer
 * device - device name
 * enable - 1 for enable and 0 for disable
 * return 0 on sucess, otherwise a negative error code
 */
static int snd_use_case_set_device_for_all_ident(snd_use_case_mgr_t *uc_mgr,
    const char *device, int enable)
{
    char *ident_value, use_case[MAX_UC_LEN];
    int list_size, index = 0, ret = -ENODEV, flag = 0;

    LOGV("set_device_for_all_ident(): %s", device);
    if (strncmp(uc_mgr->card_ctxt_ptr->current_verb, SND_USE_CASE_VERB_INACTIVE, strlen(SND_USE_CASE_VERB_INACTIVE))) {
        strlcpy(use_case, uc_mgr->card_ctxt_ptr->current_verb, sizeof(use_case));
        strlcat(use_case, device, sizeof(use_case));
        if ((get_use_case_index(uc_mgr, use_case)) < 0) {
            LOGV("No valid use case found: %s", use_case);
        } else {
            if (enable) {
                ret = snd_use_case_apply_mixer_controls(uc_mgr, device, enable);
                if (!ret)
                    snd_ucm_set_status_at_index(uc_mgr->card_ctxt_ptr->dev_list_head, device, enable);
                flag = 1;
            }
            LOGV("set %d for use case value: %s", enable, use_case);
            ret = snd_use_case_apply_mixer_controls(uc_mgr, use_case, enable);
            if (ret != 0)
                LOGE("No valid controls exists for usecase %s and device %s, enable: %d", use_case, device, enable);
        }
        use_case[0] = 0;
    }
    snd_ucm_print_list(uc_mgr->card_ctxt_ptr->mod_list_head);
    list_size = snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->mod_list_head);
    for (index = 0; index < list_size; index++) {
        ident_value = snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->mod_list_head, index);
        strlcpy(use_case, ident_value, sizeof(use_case));
        strlcat(use_case, device, sizeof(use_case));
        if ((get_use_case_index(uc_mgr, use_case)) < 0) {
            LOGV("No valid use case found: %s", use_case);
        } else {
            if (enable && !flag) {
                snd_use_case_apply_mixer_controls(uc_mgr, device, enable);
                if (!ret)
                    snd_ucm_set_status_at_index(uc_mgr->card_ctxt_ptr->dev_list_head, device, enable);
                flag = 1;
            }
            LOGV("set %d for use case value: %s", enable, use_case);
            ret = snd_use_case_apply_mixer_controls(uc_mgr, use_case, enable);
            if (ret != 0)
                LOGE("No valid controls exists for usecase %s and device %s, enable: %d", use_case, device, enable);
        }
        use_case[0] = 0;
        free(ident_value);
    }
    if (!enable) {
        ret = snd_use_case_apply_mixer_controls(uc_mgr, device, enable);
        if (!ret)
            snd_ucm_set_status_at_index(uc_mgr->card_ctxt_ptr->dev_list_head, device, enable);
    }
    return ret;
}

/* Check if a device is valid for existing use cases to avoid disabling
 * uc_mgr - UCM structure pointer
 * device - device name
 * return 1 if valid and 0 if device can be disabled
 */
static int snd_use_case_check_device_for_disable(snd_use_case_mgr_t *uc_mgr, const char *device)
{
    char *ident_value, use_case[MAX_UC_LEN];
    int list_size, verb_index, index = 0, case_index = 0, ret = 0;

    verb_index = uc_mgr->card_ctxt_ptr->current_verb_index;
    if((verb_index < 0) || (!strncmp(uc_mgr->card_ctxt_ptr->current_verb, SND_UCM_END_OF_LIST, 3)) ||
       (uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl == NULL)) {
        LOGE("Invalid current verb value: %s - %d", uc_mgr->card_ctxt_ptr->current_verb, verb_index);
        return -EINVAL;
    }
    if (strncmp(uc_mgr->card_ctxt_ptr->current_verb, SND_USE_CASE_VERB_INACTIVE, strlen(SND_USE_CASE_VERB_INACTIVE))) {
        strlcpy(use_case, uc_mgr->card_ctxt_ptr->current_verb, sizeof(use_case));
        strlcat(use_case, device, sizeof(use_case));
        while(strncmp(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].case_name,
                     SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))) {
            if (!strncmp(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[index].case_name, use_case, (strlen(use_case)+1))) {
                ret = 1;
                break;
            }
            index++;
        }
        use_case[0] = 0;
    }
    if (ret == 0) {
        index = 0;
        list_size = snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->mod_list_head);
        for (index = 0; index < list_size; index++) {
            ident_value = snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->mod_list_head, index);
            strlcpy(use_case, ident_value, sizeof(use_case));
            strlcat(use_case, device, sizeof(use_case));
            free(ident_value);
            case_index = 0;
            while(strncmp(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].case_name,
                         SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))) {
                if (!strncmp(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].case_name, use_case, (strlen(use_case)+1))) {
                    ret = 1;
                    break;
                }
                case_index++;
            }
            use_case[0] = 0;
            if (ret == 1) {
                break;
            }
        }
    }
    return ret;
}

/**
 * Set new value for an identifier
 * uc_mgr - UCM structure
 * identifier - _verb, _enadev, _disdev, _enamod, _dismod
 *        _swdev, _swmod
 * value - Value to be set
 * returns 0 on success, otherwise a negative error code
 */
int snd_use_case_set(snd_use_case_mgr_t *uc_mgr,
                     const char *identifier,
                     const char *value)
{
    char ident[MAX_STR_LEN], *ident1, *ident2, *temp_ptr;
    int verb_index, list_size, index = 0, ret = -EINVAL;

    pthread_mutex_lock(&uc_mgr->card_ctxt_ptr->card_lock);
    if ((uc_mgr->snd_card_index >= (int)MAX_NUM_CARDS) || (value == NULL) ||
        (uc_mgr->snd_card_index < 0) || (uc_mgr->card_ctxt_ptr == NULL) ||
        (identifier == NULL)) {
        LOGE("snd_use_case_set(): failed, invalid arguments");
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return -EINVAL;
    }

    LOGD("snd_use_case_set(): uc_mgr %p identifier %s value %s", uc_mgr, identifier, value);
    strlcpy(ident, identifier, sizeof(ident));
    if(!(ident1 = strtok_r(ident, "/", &temp_ptr))) {
        LOGV("No multiple identifiers found in identifier value");
        ident[0] = 0;
    } else {
        if (!strncmp(ident1, "_swdev", 6)) {
            if(!(ident2 = strtok_r(NULL, "/", &temp_ptr))) {
                LOGD("Invalid disable device value: %s, but enabling new device", ident2);
            } else {
                ret = snd_ucm_del_ident_from_list(&uc_mgr->card_ctxt_ptr->dev_list_head, ident2);
                if (ret < 0) {
                    LOGV("Ignore device %s disable, device not part of enabled list", ident2);
                } else {
                    LOGV("swdev: device value to be disabled: %s", ident2);
                    /* Disable mixer controls for corresponding use cases and device */
                    ret = snd_use_case_set_device_for_all_ident(uc_mgr, ident2, 0);
                    if (ret < 0) {
                        LOGV("Device %s not disabled, no valid use case found: %d", ident2, errno);
                    }
                }
            }
            pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
            ret = snd_use_case_set(uc_mgr, "_enadev", value);
            if (ret < 0) {
                LOGV("Device %s not enabled, no valid use case found: %d", value, errno);
            }
            return ret;
        } else if (!strncmp(ident1, "_swmod", 6)) {
            pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
            if(!(ident2 = strtok_r(NULL, "/", &temp_ptr))) {
                LOGD("Invalid modifier value: %s, but enabling new modifier", ident2);
            } else {
                ret = snd_use_case_set(uc_mgr, "_dismod", ident2);
                if (ret < 0) {
                    LOGV("Modifier %s not disabled, no valid use case found: %d", ident2, errno);
                }
            }
            ret = snd_use_case_set(uc_mgr, "_enamod", value);
            if (ret < 0) {
                LOGV("Modifier %s not enabled, no valid use case found: %d", value, errno);
            }
            return ret;
        } else {
            LOGV("No switch device/modifier option found: %s", ident1);
        }
        ident[0] = 0;
    }

    if (!strncmp(identifier, "_verb", 5)) {
        /* Check if value is valid verb */
        while (strncmp(uc_mgr->card_ctxt_ptr->verb_list[index], SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))) {
            if (!strncmp(uc_mgr->card_ctxt_ptr->verb_list[index], value, (strlen(value)+1))) {
                ret = 0;
                break;
            }
            index++;
        }
        if ((ret < 0) && (strncmp(value, SND_USE_CASE_VERB_INACTIVE, strlen(SND_USE_CASE_VERB_INACTIVE)))) {
            LOGE("Invalid verb identifier value");
        } else {
            LOGV("Index:%d Verb:%s", index, uc_mgr->card_ctxt_ptr->verb_list[index]);
            /* Disable the mixer controls for current use case
             * for all the enabled devices */
            if (strncmp(uc_mgr->card_ctxt_ptr->current_verb, SND_USE_CASE_VERB_INACTIVE, strlen(SND_USE_CASE_VERB_INACTIVE))) {
                ret = snd_use_case_ident_set_controls_for_all_devices(uc_mgr, uc_mgr->card_ctxt_ptr->current_verb, 0);
                if (ret != 0)
                    LOGE("Failed to disable controls for use case: %s", uc_mgr->card_ctxt_ptr->current_verb);
            }
            strlcpy(uc_mgr->card_ctxt_ptr->current_verb, value, MAX_STR_LEN);
            /* Enable the mixer controls for the new use case
             * for all the enabled devices */
            if (strncmp(uc_mgr->card_ctxt_ptr->current_verb, SND_USE_CASE_VERB_INACTIVE, strlen(SND_USE_CASE_VERB_INACTIVE))) {
               uc_mgr->card_ctxt_ptr->current_verb_index = index;
               ret = snd_use_case_ident_set_controls_for_all_devices(uc_mgr, uc_mgr->card_ctxt_ptr->current_verb, 1);
            }
        }
    } else if (!strncmp(identifier, "_enadev", 7)) {
        index = 0; ret = 0;
        list_size = snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->dev_list_head);
        for (index = 0; index < list_size; index++) {
            ident1 = snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->dev_list_head, index);
            if (!strncmp(ident1, value, (strlen(value)+1))) {
                LOGV("Ignoring %s device enable, it is already part of enabled list", value);
                free(ident1);
                break;
            }
            free(ident1);
        }
        if (index == list_size) {
            LOGV("enadev: device value to be enabled: %s", value);
            snd_ucm_add_ident_to_list(&uc_mgr->card_ctxt_ptr->dev_list_head, value);
            snd_ucm_print_list(uc_mgr->card_ctxt_ptr->dev_list_head);
            /* Apply Mixer controls of all verb and modifiers for this device*/
            ret = snd_use_case_set_device_for_all_ident(uc_mgr, value, 1);
        }
    } else if (!strncmp(identifier, "_disdev", 7)) {
        ret = snd_ucm_get_status_at_index(uc_mgr->card_ctxt_ptr->dev_list_head, value);
        if ((ret < 0) || (ret == 0)) {
            LOGD("disdev: device %s not enabled or not active, no need to disable", value);
        } else {
            ret = snd_use_case_check_device_for_disable(uc_mgr, value);
            if (ret == 0) {
                ret = snd_ucm_del_ident_from_list(&uc_mgr->card_ctxt_ptr->dev_list_head, value);
                if (ret < 0) {
                    LOGE("Invalid device: Device not part of enabled device list");
                } else {
                    LOGV("disdev: device value to be disabled: %s", value);
                    /* Apply Mixer controls for corresponding device and modifier */
                    ret = snd_use_case_apply_mixer_controls(uc_mgr, value, 0);
                }
            } else {
                LOGD("Valid use cases exists for device %s, ignoring disable command", value);
            }
        }
    } else if (!strncmp(identifier, "_enamod", 7)) {
        verb_index =  uc_mgr->card_ctxt_ptr->current_verb_index; index = 0; ret = 0;
        if (verb_index < 0) {
            LOGE("Invalid verb identifier value");
        } else {
            LOGV("Index:%d Verb:%s", verb_index, uc_mgr->card_ctxt_ptr->verb_list[verb_index]);
            while(strncmp(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].modifier_list[index],
                value, (strlen(value)+1))) {
                if (!strncmp(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].modifier_list[index],
                    SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))){
                    ret = -EINVAL;
                    break;
                }
                index++;
            }
            if (ret < 0) {
                LOGE("Invalid modifier identifier value");
            } else {
                snd_ucm_add_ident_to_list(&uc_mgr->card_ctxt_ptr->mod_list_head, value);
                /* Enable the mixer controls for the new use case
                 * for all the enabled devices */
                ret = snd_use_case_ident_set_controls_for_all_devices(uc_mgr, value, 1);
            }
        }
    } else if (!strncmp(identifier, "_dismod", 7)) {
        ret = snd_ucm_del_ident_from_list(&uc_mgr->card_ctxt_ptr->mod_list_head, value);
        if (ret < 0) {
            LOGE("Modifier not enabled currently, invalid modifier");
        } else {
            LOGV("dismod: modifier value to be disabled: %s", value);
            /* Enable the mixer controls for the new use case
             * for all the enabled devices */
            ret = snd_use_case_ident_set_controls_for_all_devices(uc_mgr, value, 0);
        }
    } else {
        LOGE("Unknown identifier value: %s", identifier);
    }
    pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
    return ret;
}

/**
 * Open and initialise use case core for sound card
 * uc_mgr - Returned use case manager pointer
 * card_name - Sound card name.
 * returns 0 on success, otherwise a negative error code
 */
int snd_use_case_mgr_open(snd_use_case_mgr_t **uc_mgr, const char *card_name)
{
    snd_use_case_mgr_t *uc_mgr_ptr = NULL;
    int index, ret = -EINVAL;
    char tmp[2];

    LOGV("snd_use_case_open(): card_name %s", card_name);

    if (card_name == NULL) {
        LOGE("snd_use_case_mgr_open: failed, invalid arguments");
        return ret;
    }

    for (index = 0; index < (int)MAX_NUM_CARDS; index++) {
        if(!strncmp(card_name, card_mapping_list[index].card_name, (strlen(card_mapping_list[index].card_name)+1))) {
            ret = 0;
            break;
        }
    }

    if (ret < 0) {
        LOGE("Card %s not found", card_name);
    } else {
        uc_mgr_ptr = (snd_use_case_mgr_t *)calloc(1, sizeof(snd_use_case_mgr_t));
        if (uc_mgr_ptr == NULL) {
            LOGE("Failed to allocate memory for instance");
            return -ENOMEM;
        }
        uc_mgr_ptr->snd_card_index = index;
        uc_mgr_ptr->card_ctxt_ptr = (card_ctxt_t *)calloc(1, sizeof(card_ctxt_t));
        if (uc_mgr_ptr->card_ctxt_ptr == NULL) {
            LOGE("Failed to allocate memory for card context");
            free(uc_mgr_ptr);
            uc_mgr_ptr = NULL;
            return -ENOMEM;
        }
        uc_mgr_ptr->card_ctxt_ptr->card_number = card_mapping_list[index].card_number;
        uc_mgr_ptr->card_ctxt_ptr->card_name = (char *)malloc((strlen(card_name)+1)*sizeof(char));
        if (uc_mgr_ptr->card_ctxt_ptr->card_name == NULL) {
            LOGE("Failed to allocate memory for card name");
            free(uc_mgr_ptr->card_ctxt_ptr);
            free(uc_mgr_ptr);
            uc_mgr_ptr = NULL;
            return -ENOMEM;
        }
        strlcpy(uc_mgr_ptr->card_ctxt_ptr->card_name, card_name, ((strlen(card_name)+1)*sizeof(char)));
        uc_mgr_ptr->card_ctxt_ptr->control_device = (char *)malloc((strlen("/dev/snd/controlC")+2)*sizeof(char));
        if (uc_mgr_ptr->card_ctxt_ptr->control_device == NULL) {
            LOGE("Failed to allocate memory for control device string");
            free(uc_mgr_ptr->card_ctxt_ptr->card_name);
            free(uc_mgr_ptr->card_ctxt_ptr);
            free(uc_mgr_ptr);
            uc_mgr_ptr = NULL;
            return -ENOMEM;
        }
        strlcpy(uc_mgr_ptr->card_ctxt_ptr->control_device, "/dev/snd/controlC", 18);
        snprintf(tmp, sizeof(tmp), "%d", uc_mgr_ptr->card_ctxt_ptr->card_number);
        strlcat(uc_mgr_ptr->card_ctxt_ptr->control_device, tmp, (strlen("/dev/snd/controlC")+2)*sizeof(char));
        uc_mgr_ptr->device_list_count = 0;
        uc_mgr_ptr->modifier_list_count = 0;
        uc_mgr_ptr->current_device_list = NULL;
        uc_mgr_ptr->current_modifier_list = NULL;
        uc_mgr_ptr->current_tx_device = -1;
        uc_mgr_ptr->current_rx_device = -1;
        pthread_mutexattr_init(&uc_mgr_ptr->card_ctxt_ptr->card_lock_attr);
        pthread_mutex_init(&uc_mgr_ptr->card_ctxt_ptr->card_lock,
            &uc_mgr_ptr->card_ctxt_ptr->card_lock_attr);
        strlcpy(uc_mgr_ptr->card_ctxt_ptr->current_verb, SND_USE_CASE_VERB_INACTIVE, MAX_STR_LEN);
        /* Reset all mixer controls if any applied previously for the same card */
	snd_use_case_mgr_reset(uc_mgr_ptr);
        uc_mgr_ptr->card_ctxt_ptr->current_verb_index = -1;
        /* Parse config files and update mixer controls */
        ret = snd_ucm_parse(&uc_mgr_ptr);
        if(ret < 0) {
            LOGE("Failed to parse config files: %d", ret);
            snd_ucm_free_mixer_list(&uc_mgr_ptr);
        }
        LOGV("Open mixer device: %s", uc_mgr_ptr->card_ctxt_ptr->control_device);
        uc_mgr_ptr->card_ctxt_ptr->mixer_handle = mixer_open(uc_mgr_ptr->card_ctxt_ptr->control_device);
        LOGV("Mixer handle %p", uc_mgr_ptr->card_ctxt_ptr->mixer_handle);
        if ((acdb_loader_init_ACDB()) < 0) {
            LOGE("Failed to initialize ACDB");
        }
        *uc_mgr = uc_mgr_ptr;
    }
    LOGV("snd_use_case_open(): returning instance %p", uc_mgr_ptr);
    return ret;
}


/**
 * \brief Reload and re-parse use case configuration files for sound card.
 * \param uc_mgr Use case manager
 * \return zero if success, otherwise a negative error code
 */
int snd_use_case_mgr_reload(snd_use_case_mgr_t *uc_mgr) {
    LOGE("Reload is not implemented for now as there is no use case currently");
    return 0;
}

/**
 * \brief Close use case manager
 * \param uc_mgr Use case manager
 * \return zero if success, otherwise a negative error code
 */
int snd_use_case_mgr_close(snd_use_case_mgr_t *uc_mgr)
{
    int ret = 0;

    if ((uc_mgr->snd_card_index >= (int)MAX_NUM_CARDS) ||
        (uc_mgr->snd_card_index < 0) || (uc_mgr->card_ctxt_ptr == NULL)) {
        LOGE("snd_use_case_mgr_close(): failed, invalid arguments");
        return -EINVAL;
    }

    LOGV("snd_use_case_close(): instance %p", uc_mgr);
    ret = snd_use_case_mgr_reset(uc_mgr);
    if (ret < 0)
        LOGE("Failed to reset ucm session");
    snd_ucm_free_mixer_list(&uc_mgr);
    acdb_loader_deallocate_ACDB();
    pthread_mutexattr_destroy(&uc_mgr->card_ctxt_ptr->card_lock_attr);
    pthread_mutex_destroy(&uc_mgr->card_ctxt_ptr->card_lock);
    if (uc_mgr->card_ctxt_ptr->mixer_handle) {
        mixer_close(uc_mgr->card_ctxt_ptr->mixer_handle);
        uc_mgr->card_ctxt_ptr->mixer_handle = NULL;
    }
    uc_mgr->snd_card_index = -1;
    uc_mgr->current_tx_device = -1;
    uc_mgr->current_rx_device = -1;
    free(uc_mgr->card_ctxt_ptr->control_device);
    free(uc_mgr->card_ctxt_ptr->card_name);
    free(uc_mgr->card_ctxt_ptr);
    uc_mgr->card_ctxt_ptr = NULL;
    free(uc_mgr);
    uc_mgr = NULL;
    LOGV("snd_use_case_mgr_close(): card instace closed successfully");
    return ret;
}

/**
 * \brief Reset use case manager verb, device, modifier to deafult settings.
 * \param uc_mgr Use case manager
 * \return zero if success, otherwise a negative error code
 */
int snd_use_case_mgr_reset(snd_use_case_mgr_t *uc_mgr)
{
    char *ident_value;
    int index, list_size, ret = 0;

    LOGV("snd_use_case_reset(): instance %p", uc_mgr);
    pthread_mutex_lock(&uc_mgr->card_ctxt_ptr->card_lock);
    if ((uc_mgr->snd_card_index >= (int)MAX_NUM_CARDS) ||
        (uc_mgr->snd_card_index < 0) || (uc_mgr->card_ctxt_ptr == NULL)) {
        LOGE("snd_use_case_mgr_reset(): failed, invalid arguments");
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return -EINVAL;
    }

    /* Disable mixer controls of all the enabled modifiers */
    list_size = snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->mod_list_head);
    for (index = (list_size-1); index >= 0; index--) {
        ident_value = snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->mod_list_head, index);
        snd_ucm_del_ident_from_list(&uc_mgr->card_ctxt_ptr->mod_list_head, ident_value);
        ret = snd_use_case_ident_set_controls_for_all_devices(uc_mgr, ident_value, 0);
	if (ret != 0)
		LOGE("Failed to disable mixer controls for %s", ident_value);
	free(ident_value);
    }
    /* Clear the enabled modifiers list */
    if (uc_mgr->modifier_list_count) {
        for (index = 0; index < uc_mgr->modifier_list_count; index++) {
            free(uc_mgr->current_modifier_list[index]);
            uc_mgr->current_modifier_list[index] = NULL;
        }
        free(uc_mgr->current_modifier_list);
        uc_mgr->current_modifier_list = NULL;
        uc_mgr->modifier_list_count = 0;
    }
    /* Disable mixer controls of current use case verb */
    if(strncmp(uc_mgr->card_ctxt_ptr->current_verb, SND_USE_CASE_VERB_INACTIVE, strlen(SND_USE_CASE_VERB_INACTIVE))) {
        ret = snd_use_case_ident_set_controls_for_all_devices(uc_mgr, uc_mgr->card_ctxt_ptr->current_verb, 0);
        if (ret != 0)
            LOGE("Failed to disable mixer controls for %s", uc_mgr->card_ctxt_ptr->current_verb);
        strlcpy(uc_mgr->card_ctxt_ptr->current_verb, SND_USE_CASE_VERB_INACTIVE, MAX_STR_LEN);
    }
    /* Disable mixer controls of all the enabled devices */
    list_size = snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->dev_list_head);
    for (index = (list_size-1); index >= 0; index--) {
        ident_value = snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->dev_list_head, index);
        snd_ucm_del_ident_from_list(&uc_mgr->card_ctxt_ptr->dev_list_head, ident_value);
        ret = snd_use_case_set_device_for_all_ident(uc_mgr, ident_value, 0);
	if (ret != 0)
            LOGE("Failed to disable or no mixer controls set for %s", ident_value);
	free(ident_value);
    }
    /* Clear the enabled devices list */
    if (uc_mgr->device_list_count) {
        for (index = 0; index < uc_mgr->device_list_count; index++) {
            free(uc_mgr->current_device_list[index]);
            uc_mgr->current_device_list[index] = NULL;
        }
        free(uc_mgr->current_device_list);
        uc_mgr->current_device_list = NULL;
        uc_mgr->device_list_count = 0;
    }
    uc_mgr->current_tx_device = -1;
    uc_mgr->current_rx_device = -1;
    pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
    return ret;
}

/* 2nd stage parsing done in seperate thread */
void *second_stage_parsing_thread(void *uc_mgr_ptr)
{
    char path[200];
    struct stat st;
    int fd, index = 0, ret = 0, rc = 0;
    char *read_buf, *next_str, *current_str, *buf, *p, *verb_name, *file_name = NULL, *temp_ptr;
    snd_use_case_mgr_t **uc_mgr = (snd_use_case_mgr_t **)&uc_mgr_ptr;

    strlcpy(path, CONFIG_DIR, (strlen(CONFIG_DIR)+1));
    strlcat(path, (*uc_mgr)->card_ctxt_ptr->card_name, sizeof(path));
    LOGV("master config file path:%s", path);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOGE("failed to open config file %s error %d\n", path, errno);
        return NULL;
    }
    if (fstat(fd, &st) < 0) {
        LOGE("failed to stat %s error %d\n", path, errno);
        close(fd);
        return NULL;
    }
    read_buf = (char *) mmap(0, st.st_size, PROT_READ | PROT_WRITE,
               MAP_PRIVATE, fd, 0);
    if (read_buf == MAP_FAILED) {
        LOGE("failed to mmap file error %d\n", errno);
        close(fd);
        return NULL;
    }
    current_str = read_buf;
    verb_name = NULL;
    while (*current_str != (char)EOF)  {
        next_str = strchr(current_str, '\n');
        if (!next_str)
            break;
        *next_str++ = '\0';
        if (verb_name == NULL) {
            buf = strstr(current_str, "SectionUseCase");
            if (buf == NULL) {
                if((current_str = next_str) == NULL)
                    break;
                else
                    continue;
            }
            /* Ignore parsing first use case (HiFi) as it is already parsed
             * in 1st stage of parsing */
            if (index == 0) {
                index++;
                if((current_str = next_str) == NULL)
                    break;
                else
                    continue;
            }
            p = strtok_r(buf, ".", &temp_ptr);
            while (p != NULL) {
                p = strtok_r(NULL, "\"", &temp_ptr);
                if (p == NULL)
                    break;
                verb_name = (char *)malloc((strlen(p)+1)*sizeof(char));
                if(verb_name == NULL) {
                    ret = -ENOMEM;
                    break;
                }
                strlcpy(verb_name, p, (strlen(p)+1)*sizeof(char));
                break;
            }
        } else {
            buf = strstr(current_str, "File");
            if (buf == NULL) {
                if((current_str = next_str) == NULL)
                    break;
                else
                    continue;
            }
            p = strtok_r(buf, "\"", &temp_ptr);
            while (p != NULL) {
                p = strtok_r(NULL, "\"", &temp_ptr);
                if (p == NULL)
                    break;
                file_name = (char *)malloc((strlen(p)+1)*sizeof(char));
                if(file_name == NULL) {
                    ret = -ENOMEM;
                    break;
                }
                strlcpy(file_name, p, (strlen(p)+1)*sizeof(char));
                break;
            }
            if (file_name != NULL) {
                ret = snd_ucm_parse_verb(uc_mgr, file_name, index);
                pthread_mutex_lock(&(*uc_mgr)->card_ctxt_ptr->card_lock);
                (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].use_case_name =
                    (char *)malloc((strlen(verb_name)+1)*sizeof(char));
                strlcpy((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].use_case_name,
                    verb_name, ((strlen(verb_name)+1)*sizeof(char)));
                /* Verb list might have been appended with END OF LIST in 1st stage parsing
                 * Delete this entry so that new verbs are appended from here and END OF LIST
                 * will be added again at the end of 2nd stage parsing
                 */
                if((*uc_mgr)->card_ctxt_ptr->verb_list[index]) {
                    free((*uc_mgr)->card_ctxt_ptr->verb_list[index]);
                    (*uc_mgr)->card_ctxt_ptr->verb_list[index] = NULL;
                }
                (*uc_mgr)->card_ctxt_ptr->verb_list[index] =
                    (char *)malloc((strlen(verb_name)+1)*sizeof(char));
                strlcpy((*uc_mgr)->card_ctxt_ptr->verb_list[index], verb_name, ((strlen(verb_name)+1)*sizeof(char)));
                index++;
                (*uc_mgr)->card_ctxt_ptr->verb_list[index] =
                    (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
                strlcpy((*uc_mgr)->card_ctxt_ptr->verb_list[index], SND_UCM_END_OF_LIST,
                       ((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char)));
                pthread_mutex_unlock(&(*uc_mgr)->card_ctxt_ptr->card_lock);
                free(verb_name);
                verb_name = NULL;
                free(file_name);
                file_name = NULL;
            } else {
                pthread_mutex_lock(&(*uc_mgr)->card_ctxt_ptr->card_lock);
                index++;
                (*uc_mgr)->card_ctxt_ptr->verb_list[index] =
                    (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
                strlcpy((*uc_mgr)->card_ctxt_ptr->verb_list[index], SND_UCM_END_OF_LIST,
                        ((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char)));
                pthread_mutex_unlock(&(*uc_mgr)->card_ctxt_ptr->card_lock);
            }
        }
        if((current_str = next_str) == NULL)
            break;
    }
    munmap(read_buf, st.st_size);
    close(fd);
#if PARSE_DEBUG
        /* Prints use cases and mixer controls parsed from config files */
        snd_ucm_print((*uc_mgr));
#endif
    if(ret < 0)
        LOGE("Failed to parse config files: %d", ret);
    LOGE("Exiting parsing thread uc_mgr %p\n", uc_mgr);
    return NULL;
}

/* Parse config files and update mixer controls for the use cases
 * 1st stage parsing done to parse HiFi config file
 * uc_mgr - use case manager structure
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_parse(snd_use_case_mgr_t **uc_mgr)
{
    struct stat st;
    int fd, verb_count, index = 0, ret = 0, rc;
    char *read_buf, *next_str, *current_str, *buf, *p, *verb_name, *file_name = NULL, *temp_ptr;
    char path[200];

    strlcpy(path, CONFIG_DIR, (strlen(CONFIG_DIR)+1));
    strlcat(path, (*uc_mgr)->card_ctxt_ptr->card_name, sizeof(path));
    LOGV("master config file path:%s", path);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOGE("failed to open config file %s error %d\n", path, errno);
        return -EINVAL;
    }
    if (fstat(fd, &st) < 0) {
        LOGE("failed to stat %s error %d\n", path, errno);
        close(fd);
        return -EINVAL;
    }
    read_buf = (char *) mmap(0, st.st_size, PROT_READ | PROT_WRITE,
               MAP_PRIVATE, fd, 0);
    if (read_buf == MAP_FAILED) {
        LOGE("failed to mmap file error %d\n", errno);
        close(fd);
        return -EINVAL;
    }
    current_str = read_buf;
    verb_count = get_verb_count(current_str);
    (*uc_mgr)->card_ctxt_ptr->use_case_verb_list =
        (use_case_verb_t *)malloc((verb_count+1)*(sizeof(use_case_verb_t)));
    (*uc_mgr)->card_ctxt_ptr->verb_list =
        (char **)malloc((verb_count+2)*(sizeof(char *)));
    verb_name = NULL;
    while (*current_str != (char)EOF)  {
        next_str = strchr(current_str, '\n');
        if (!next_str)
            break;
        *next_str++ = '\0';
        if (verb_name == NULL) {
            buf = strstr(current_str, "SectionUseCase");
            if (buf == NULL) {
                if((current_str = next_str) == NULL)
                    break;
                else
                    continue;
            }
            p = strtok_r(buf, ".", &temp_ptr);
            while (p != NULL) {
                p = strtok_r(NULL, "\"", &temp_ptr);
                if (p == NULL)
                    break;
                verb_name = (char *)malloc((strlen(p)+1)*sizeof(char));
                if(verb_name == NULL) {
                    ret = -ENOMEM;
                    break;
                }
                strlcpy(verb_name, p, (strlen(p)+1)*sizeof(char));
                (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].use_case_name =
                    (char *)malloc((strlen(verb_name)+1)*sizeof(char));
                strlcpy((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].use_case_name,
                    verb_name, ((strlen(verb_name)+1)*sizeof(char)));
                (*uc_mgr)->card_ctxt_ptr->verb_list[index] =
                    (char *)malloc((strlen(verb_name)+1)*sizeof(char));
                strlcpy((*uc_mgr)->card_ctxt_ptr->verb_list[index], verb_name,
                        ((strlen(verb_name)+1)*sizeof(char)));
                break;
            }
        } else {
            buf = strstr(current_str, "File");
            if (buf == NULL) {
                if((current_str = next_str) == NULL)
                    break;
                else
                    continue;
            }
            p = strtok_r(buf, "\"", &temp_ptr);
            while (p != NULL) {
                p = strtok_r(NULL, "\"", &temp_ptr);
                if (p == NULL)
                    break;
                file_name = (char *)malloc((strlen(p)+1)*sizeof(char));
                if(file_name == NULL) {
                    ret = -ENOMEM;
                    break;
                }
                strlcpy(file_name, p, (strlen(p)+1)*sizeof(char));
                break;
            }
            if (file_name != NULL) {
                ret = snd_ucm_parse_verb(uc_mgr, file_name, index);
                if (ret < 0)
                    LOGE("Failed to parse config file %s\n", file_name);
                free(verb_name);
                verb_name = NULL;
                free(file_name);
                file_name = NULL;
            }
            index++;
            /* Break here so that only one first use case config file (HiFi)
             * from master config file is parsed initially and all other
             * config files are parsed in seperate thread created below so
             * that audio HAL can initialize faster during boot-up
             */
            break;
        }
        if((current_str = next_str) == NULL)
            break;
    }
    munmap(read_buf, st.st_size);
    close(fd);
    (*uc_mgr)->card_ctxt_ptr->verb_list[index] =
        (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
    strlcpy((*uc_mgr)->card_ctxt_ptr->verb_list[index], SND_UCM_END_OF_LIST,
            ((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char)));
    if (!ret) {
        LOGD("Creating Parsing thread uc_mgr %p\n", uc_mgr);
        rc = pthread_create(&(*uc_mgr)->thr, 0, second_stage_parsing_thread, (void*)(*uc_mgr));
        if(rc < 0) {
            LOGE("Failed to create parsing thread rc %d errno %d\n", rc, errno);
        } else {
            LOGV("Prasing thread created successfully\n");
        }
    }
    return ret;
}

/* Gets the number of use case verbs defined by master config file */
static int get_verb_count(const char *nxt_str)
{
    char *current_str, *next_str, *str_addr, *buf, *p;
    int count = 0;

    next_str = (char *)malloc((strlen(nxt_str)+1)*sizeof(char));
    if (next_str == NULL) {
        LOGE("Failed to allocate memory");
        return -ENOMEM;
    }
    strlcpy(next_str, nxt_str, ((strlen(nxt_str)+1)*sizeof(char)));
    str_addr = next_str;
    current_str = next_str;
    while(1) {
        next_str = strchr(current_str, '\n');
        if (!next_str)
            break;
        *next_str++ = '\0';
        buf = strstr(current_str, "SectionUseCase");
        if (buf != NULL)
            count++;
        if (*next_str == (char)EOF)
            break;
        if((current_str = next_str) == NULL)
            break;
    }
    free(str_addr);
    return count;
}

/* Parse a use case verb config files and update mixer controls for the verb
 * uc_mgr - use case manager structure
 * file_name - use case verb config file name
 * index - index of the verb in the list
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_parse_verb(snd_use_case_mgr_t **uc_mgr, const char *file_name, int index)
{
    struct stat st;
    card_mctrl_t *list;
    int device_count, modifier_count;
    int fd, ret = 0, parse_count = 0;
    char *read_buf, *next_str, *current_str, *verb_ptr;
    char path[200];

    strlcpy(path, CONFIG_DIR, (strlen(CONFIG_DIR)+1));
    strlcat(path, file_name, sizeof(path));
    LOGV("path:%s", path);
    while(1) {
        device_count = 0; modifier_count = 0;
        if (parse_count == 0) {
            (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].use_case_count = 0;
            (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].device_list = NULL;
            (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].modifier_list = NULL;
            (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].card_ctrl = NULL;
        }
        fd = open(path, O_RDONLY);
        if (fd < 0) {
             LOGE("failed to open config file %s error %d\n", path, errno);
             return -EINVAL;
        }
        if (fstat(fd, &st) < 0) {
            LOGE("failed to stat %s error %d\n", path, errno);
            close(fd);
            return -EINVAL;
        }
        read_buf = (char *) mmap(0, st.st_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE, fd, 0);
        if (read_buf == MAP_FAILED) {
            LOGE("failed to mmap file error %d\n", errno);
            close(fd);
            return -EINVAL;
        }
        current_str = read_buf;
        while (*current_str != (char)EOF)  {
            next_str = strchr(current_str, '\n');
            if (!next_str)
                break;
            *next_str++ = '\0';
            if (!strncasecmp(current_str, "SectionVerb", 11)) {
                if (parse_count == 0) {
                    (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].use_case_count++;
                } else {
                    ret = snd_ucm_parse_section(uc_mgr, &current_str, &next_str, index);
                    if (ret < 0)
                        break;
                }
            } else if (!strncasecmp(current_str, "SectionDevice", 13)) {
                if (parse_count == 0) {
                    (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].use_case_count++;
                    device_count++;
                } else {
                    ret = snd_ucm_parse_section(uc_mgr, &current_str, &next_str, index);
                    if (ret < 0) {
                        break;
                    } else {
                        list = ((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].card_ctrl +
                            ((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].use_case_count - 1));
                        verb_ptr = (char *)malloc((strlen(list->case_name)+1)*sizeof(char));
                        if (verb_ptr == NULL) {
                            ret = -ENOMEM;
                            break;
                        }
                        strlcpy(verb_ptr, list->case_name, ((strlen(list->case_name)+1)*sizeof(char)));
                        (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].device_list[device_count]
                            = verb_ptr;
                        device_count++;
                    }
                }
            } else if (!strncasecmp(current_str, "SectionModifier", 15)) {
                if (parse_count == 0) {
                    (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].use_case_count++;
                    modifier_count++;
                } else {
                    ret = snd_ucm_parse_section(uc_mgr, &current_str, &next_str, index);
                    if (ret < 0) {
                        break;
                    } else {
                        list = ((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].card_ctrl +
                            ((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].use_case_count - 1));
                        verb_ptr = (char *)malloc((strlen(list->case_name)+1)*sizeof(char));
                        if (verb_ptr == NULL) {
                            ret = -ENOMEM;
                            break;
                        }
                        strlcpy(verb_ptr, list->case_name, ((strlen(list->case_name)+1)*sizeof(char)));
                        (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].modifier_list[modifier_count]
                            = verb_ptr;
                        modifier_count++;
                    }
                }
            }
            if((current_str = next_str) == NULL)
                break;
        }
        munmap(read_buf, st.st_size);
        close(fd);
        if(ret < 0)
            return ret;
        if (parse_count == 0) {
            (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].device_list =
                (char **)malloc((device_count+1)*sizeof(char *));
            if ((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].device_list == NULL)
                return -ENOMEM;
            (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].modifier_list =
                (char **)malloc((modifier_count+1)*sizeof(char *));
            if ((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].modifier_list == NULL)
                return -ENOMEM;
            parse_count = (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].use_case_count;
            (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].card_ctrl =
                (card_mctrl_t *)malloc((parse_count+1)*sizeof(card_mctrl_t));
            if ((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].card_ctrl == NULL) {
               ret = -ENOMEM;
               break;
            }
            (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].use_case_count = 0;
            continue;
        } else {
            verb_ptr = (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
            if (verb_ptr == NULL)
                return -ENOMEM;
            strlcpy(verb_ptr, SND_UCM_END_OF_LIST, ((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char)));
            (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].device_list[device_count]
                = verb_ptr;
            verb_ptr = (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
            if (verb_ptr == NULL)
                return -ENOMEM;
            strlcpy(verb_ptr, SND_UCM_END_OF_LIST, ((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char)));
            (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].modifier_list[modifier_count]
                = verb_ptr;
            list = ((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].card_ctrl +
                    (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].use_case_count);
            list->case_name = (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
            if(list->case_name == NULL) {
                free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[index].card_ctrl);
                return -ENOMEM;
            }
            strlcpy(list->case_name, SND_UCM_END_OF_LIST,
               (strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
            list->ena_mixer_list = NULL;
            list->dis_mixer_list = NULL;
            list->ena_mixer_count = 0;
            list->dis_mixer_count = 0;
            list->playback_dev_name = NULL;
            list->capture_dev_name = NULL;
            list->acdb_id = 0;
            list->capability = 0;
            parse_count = 0;
            break;
        }
    }
    return ret;
}

/* Print mixer controls extracted from config files
 * uc_mgr - use case manager structure
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_print(snd_use_case_mgr_t *uc_mgr)
{
    int i, j, verb_index = 0;

    pthread_mutex_lock(&uc_mgr->card_ctxt_ptr->card_lock);
    while(strncmp(uc_mgr->card_ctxt_ptr->verb_list[verb_index], SND_UCM_END_OF_LIST, 3)) {
        card_mctrl_t *list = uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl;
        LOGD("\nuse case verb: %s\n", uc_mgr->card_ctxt_ptr->verb_list[verb_index]);
        if(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].device_list) {
            LOGD("\tValid device list:");
            i = 0;
            while(strncmp(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].device_list[i], SND_UCM_END_OF_LIST, 3)) {
                LOGD("\t\t%s", uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].device_list[i]);
                i++;
            }
        }
        if(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].modifier_list) {
            LOGD("\tValid modifier list:");
            i = 0;
            while(strncmp(uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].modifier_list[i], SND_UCM_END_OF_LIST, 3)) {
                LOGD("\t\t%s", uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].modifier_list[i]);
                i++;
            }
        }
        for(i=0; i<uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].use_case_count; i++) {
            LOGD("\tcase name: %s\n", list[i].case_name);
            LOGD("\tEnable sequence: %d\n", list[i].ena_mixer_count);
            for(j=0; j<list[i].ena_mixer_count; j++) {
                LOGE("\t\t%s : %d : %d: %s\n", list[i].ena_mixer_list[j].control_name,
                    list[i].ena_mixer_list[j].type, list[i].ena_mixer_list[j].value,
                    list[i].ena_mixer_list[j].string);
            }
            LOGD("\tDisable sequence: %d\n", list[i].dis_mixer_count);
            for(j=0; j<list[i].dis_mixer_count; j++) {
                LOGE("\t\t%s : %d : %d : %s\n", list[i].dis_mixer_list[j].control_name,
                    list[i].dis_mixer_list[j].type, list[i].dis_mixer_list[j].value,
                    list[i].dis_mixer_list[j].string);
            }
        }
        verb_index++;
    }
    pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
    return 0;
}

/* Gets the number of controls for specific sequence of a use cae */
static int get_controls_count(const char *nxt_str)
{
    char *current_str, *next_str, *str_addr;
    int count = 0;

    next_str = (char *)malloc((strlen(nxt_str)+1)*sizeof(char));
    if (next_str == NULL) {
        LOGE("Failed to allocate memory");
        return -ENOMEM;
    }
    strlcpy(next_str, nxt_str, ((strlen(nxt_str)+1)*sizeof(char)));
    str_addr = next_str;
    while(1) {
        current_str = next_str;
        next_str = strchr(current_str, '\n');
        if ((!next_str) || (!strncasecmp(current_str, "EndSection", 10)))
            break;
        *next_str++ = '\0';
        if (strcasestr(current_str, "EndSequence") != NULL) {
            break;
        } else {
            count++;
        }
        if (*next_str == (char)EOF)
            break;
        if(!strncasecmp(current_str, "EndSection", 10))
            break;
    }
    free(str_addr);
    return count;
}

/* Parse a section of config files
 * uc_mgr - use case manager structure
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_parse_section(snd_use_case_mgr_t **uc_mgr, char **cur_str,
    char **nxt_str, int verb_index)
{
    card_mctrl_t *list;
    int enable_seq = 0, disable_seq = 0, controls_count = 0, ret = 0;
    char *p, *current_str, *next_str, *name;

    list = ((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl +
            (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].use_case_count);
    list->case_name = NULL;
    list->ena_mixer_list = NULL;
    list->dis_mixer_list = NULL;
    list->ena_mixer_count = 0;
    list->dis_mixer_count = 0;
    list->playback_dev_name = NULL;
    list->capture_dev_name = NULL;
    list->acdb_id = 0;
    list->capability = 0;
    current_str = *cur_str; next_str = *nxt_str;
    while(strncasecmp(current_str, "EndSection", 10)) {
        current_str = next_str;
        next_str = strchr(current_str, '\n');
        if ((!next_str) || (!strncasecmp(current_str, "EndSection", 10)))
            break;
        *next_str++ = '\0';
        if (strcasestr(current_str, "EndSequence") != NULL) {
            if (enable_seq == 1)
                enable_seq = 0;
            else if (disable_seq == 1)
                disable_seq = 0;
            else
                LOGE("Error: improper config file\n");
        }
        if (enable_seq == 1) {
            ret = snd_ucm_extract_controls(current_str, &list->ena_mixer_list, list->ena_mixer_count);
            if (ret < 0) {
                LOGV("Failed extracting a control, ignore and parse next control\n");
                break;
            } else {
                list->ena_mixer_count++;
            }
        } else if (disable_seq == 1) {
            ret = snd_ucm_extract_controls(current_str, &list->dis_mixer_list, list->dis_mixer_count);
            if (ret < 0) {
                LOGV("Failed extracting a control, ignore and parse next control\n");
                break;
            } else {
                list->dis_mixer_count++;
            }
        } else if (strcasestr(current_str, "Name") != NULL) {
            ret = snd_ucm_extract_name(current_str, &list->case_name);
            if (ret < 0)
                break;
            LOGV("Name of section is %s\n", list->case_name);
        } else if (strcasestr(current_str, "PlaybackPCM") != NULL) {
            ret = snd_ucm_extract_dev_name(current_str, &list->playback_dev_name);
            if (ret < 0)
                break;
            LOGV("Device name of playback is %s\n", list->playback_dev_name);
        } else if (strcasestr(current_str, "CapturePCM") != NULL) {
            ret = snd_ucm_extract_dev_name(current_str, &list->capture_dev_name);
            if (ret < 0)
                break;
            LOGV("Device name of capture is %s\n", list->capture_dev_name);
        } else if (strcasestr(current_str, "ACDBID") != NULL) {
            ret = snd_ucm_extract_acdb(current_str, &list->acdb_id, &list->capability);
            if (ret < 0)
                break;
            LOGV("ACDB ID: %d CAPABILITY: %d\n", list->acdb_id, list->capability);
        }
        if (strcasestr(current_str, "EnableSequence") != NULL) {
            controls_count = get_controls_count(next_str);
            if (controls_count < 0) {
                ret = -ENOMEM;
                break;
            }
            list->ena_mixer_list = (mixer_control_t *)malloc((controls_count*sizeof(mixer_control_t)));
            if (list->ena_mixer_list == NULL) {
                ret = -ENOMEM;
                break;
            }
            enable_seq = 1;
        } else if (strcasestr(current_str, "DisableSequence") != NULL) {
            controls_count = get_controls_count(next_str);
            if (controls_count < 0) {
                ret = -ENOMEM;
                break;
            }
            list->dis_mixer_list = (mixer_control_t *)malloc((controls_count*sizeof(mixer_control_t)));
            if (list->dis_mixer_list == NULL) {
                ret = -ENOMEM;
                break;
            }
            disable_seq = 1;
        }
        if (*next_str == (char)EOF)
             break;
    }
    if ((list->case_name == NULL) && (ret == 0)) {
        list->case_name = (char *)malloc((strlen((*uc_mgr)->card_ctxt_ptr->card_name)+1)*sizeof(char));
        if(list->case_name == NULL) {
            free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl);
            return -ENOMEM;
        }
        strlcpy(list->case_name, (*uc_mgr)->card_ctxt_ptr->card_name,
            (strlen((*uc_mgr)->card_ctxt_ptr->card_name)+1)*sizeof(char));
    }
    if(ret == 0) {
        *cur_str = current_str; *nxt_str = next_str;
        (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].use_case_count++;
    }
    return ret;
}

/* Extract a mixer control name from config file
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_extract_name(char *buf, char **case_name)
{
    int ret = 0;
    char *p, *name = *case_name, *temp_ptr;

    p = strtok_r(buf, "\"", &temp_ptr);
    while (p != NULL) {
        p = strtok_r(NULL, "\"", &temp_ptr);
        if (p == NULL)
            break;
        name = (char *)malloc((strlen(p)+1)*sizeof(char));
        if(name == NULL) {
            ret = -ENOMEM;
            break;
        }
        strlcpy(name, p, (strlen(p)+1)*sizeof(char));
        *case_name = name;
        break;
    }
    return ret;
}

/* Extract a ACDB ID and capability of use case from config file
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_extract_acdb(char *buf, int *id, int *cap)
{
    char *p, key[] = "0123456789", *temp_ptr;

    p = strpbrk(buf, key);
    if (p == NULL) {
        *id = 0;
        *cap = 0;
    } else {
        p = strtok_r(p, ":", &temp_ptr);
        while (p != NULL) {
            *id = atoi(p);
            p = strtok_r(NULL, "\0", &temp_ptr);
            if (p == NULL)
                break;
            *cap = atoi(p);
            break;
        }
    }
    return 0;
}

/* Extract a playback and capture device name of use case from config file
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_extract_dev_name(char *buf, char **dev_name)
{
    char key[] = "0123456789";
    char *p, *name = *dev_name;
    char dev_pre[] = "hw:0,";
    char *temp_ptr;

    p = strpbrk(buf, key);
    if (p == NULL) {
        *dev_name = NULL;
    } else {
        p = strtok_r(p, "\r\n", &temp_ptr);
        if (p == NULL) {
            *dev_name = NULL;
        } else {
            name = (char *)malloc((strlen(p)+strlen(dev_pre)+1)*sizeof(char));
            if(name == NULL)
                 return -ENOMEM;
            strlcpy(name, dev_pre, (strlen(p)+strlen(dev_pre)+1)*sizeof(char));
            strlcat(name, p, (strlen(p)+strlen(dev_pre)+1)*sizeof(char));
            *dev_name = name;
        }
    }
    return 0;
}

static int get_num_values(const char *buf)
{
    char *buf_addr, *p;
    int count = 0;
    char *temp_ptr;

    buf_addr = (char *)malloc((strlen(buf)+1)*sizeof(char));
    if (buf_addr == NULL) {
        LOGE("Failed to allocate memory");
        return -ENOMEM;
    }
    strlcpy(buf_addr, buf, ((strlen(buf)+1)*sizeof(char)));
    p = strtok_r(buf_addr, " ", &temp_ptr);
    while (p != NULL) {
        count++;
        p = strtok_r(NULL, " ", &temp_ptr);
        if (p == NULL)
            break;
    }
    free(buf_addr);
    return count;
}

/* Extract a mixer control from config file
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_extract_controls(char *buf, mixer_control_t **mixer_list, int size)
{
    uint32_t temp;
    int ret = -EINVAL, i, index = 0, count = 0;
    char *p, *ps, *pmv, temp_coeff[20];
    mixer_control_t *list;
    static const char *const seps = "\r\n";
    char *temp_ptr, *temp_vol_ptr;

    p = strtok_r(buf, "'", &temp_ptr);
    while (p != NULL) {
        p = strtok_r(NULL, "'", &temp_ptr);
        if (p == NULL)
            break;
        list = ((*mixer_list)+size);
        list->control_name = (char *)malloc((strlen(p)+1)*sizeof(char));
        if(list->control_name == NULL) {
            ret = -ENOMEM;
            break;
        }
        strlcpy(list->control_name, p, (strlen(p)+1)*sizeof(char));
        p = strtok_r(NULL, ":", &temp_ptr);
        if (p == NULL) {
            free(list->control_name);
            break;
        }
        if(!strncmp(p, "0", 1)) {
            list->type = TYPE_STR;
        } else if(!strncmp(p, "1", 1)) {
            list->type = TYPE_INT;
        } else if(!strncmp(p, "2", 1)) {
            list->type = TYPE_MULTI_VAL;
        } else {
            LOGE("Unknown type: p %s\n", p);
        }
        p = strtok_r(NULL, seps, &temp_ptr);
        if (p == NULL) {
            free(list->control_name);
            break;
        }
        if(list->type == TYPE_INT) {
            list->value = atoi(p);
            list->string = NULL;
            list->mulval = NULL;
        } else if(list->type == TYPE_STR) {
            list->value = -1;
            list->string = (char *)malloc((strlen(p)+1)*sizeof(char));
            list->mulval = NULL;
            if(list->string == NULL) {
                ret = -ENOMEM;
                free(list->control_name);
                break;
            }
            strlcpy(list->string, p, (strlen(p)+1)*sizeof(char));
        } else if(list->type == TYPE_MULTI_VAL) {
            if (p != NULL) {
                count = get_num_values(p);
                list->mulval = (char **)malloc(count*sizeof(char *));
                index = 0;
                /* To support volume values in percentage */
                if ((count == 1) && (strstr(p, "%") != NULL)) {
                    pmv = strtok_r(p, " ", &temp_vol_ptr);
                    while (pmv != NULL) {
                        list->mulval[index] = (char *)malloc((strlen(pmv)+1)*sizeof(char));
                        strlcpy(list->mulval[index], pmv, (strlen(pmv)+1));
                        index++;
                        pmv = strtok_r(NULL, " ", &temp_vol_ptr);
                        if (pmv == NULL)
                            break;
                    }
                } else {
                    pmv = strtok_r(p, " ", &temp_vol_ptr);
                    while (pmv != NULL) {
                        temp = (uint32_t)strtoul(pmv, &ps, 16);
                        snprintf(temp_coeff, sizeof(temp_coeff),"%lu", temp);
                        list->mulval[index] = (char *)malloc((strlen(temp_coeff)+1)*sizeof(char));
                        strlcpy(list->mulval[index], temp_coeff, (strlen(temp_coeff)+1));
                        index++;
                        pmv = strtok_r(NULL, " ", &temp_vol_ptr);
                        if (pmv == NULL)
                            break;
                    }
                }
                list->value = count;
                list->string = NULL;
            }
        } else {
            LOGE("Unknown type: p %s\n", p);
            list->value = -1;
            list->string = NULL;
        }
        ret = 0;
        break;
    }
    return ret;
}

void snd_ucm_free_mixer_list(snd_use_case_mgr_t **uc_mgr)
{
    int case_index = 0, index = 0, verb_index = 0, mul_index = 0;

    pthread_mutex_lock(&(*uc_mgr)->card_ctxt_ptr->card_lock);
    while(strncmp((*uc_mgr)->card_ctxt_ptr->verb_list[verb_index], SND_UCM_END_OF_LIST, 3)) {
        for(case_index = 0; case_index < (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].use_case_count; case_index++) {
            for(index = 0; index < (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].ena_mixer_count; index++) {
                if((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].ena_mixer_list[index].control_name) {
                    free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].ena_mixer_list[index].control_name);
                }
                if((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].ena_mixer_list[index].string) {
                    free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].ena_mixer_list[index].string);
                }
                if((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].ena_mixer_list[index].mulval) {
                    for(mul_index = 0;
                        mul_index < (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].ena_mixer_list[index].value;
                        mul_index++) {
                        free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].ena_mixer_list[index].mulval[mul_index]);
                    }
                    if((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].ena_mixer_list[index].mulval)
                        free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].ena_mixer_list[index].mulval);
                }
            }
            for(index = 0; index < (*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].dis_mixer_count; index++) {
                if((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].dis_mixer_list[index].control_name) {
                    free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].dis_mixer_list[index].control_name);
                }
                if((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].dis_mixer_list[index].string) {
                    free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].dis_mixer_list[index].string);
                }
            }
            if((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].case_name) {
                free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].case_name);
            }
            if((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].ena_mixer_list) {
                free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].ena_mixer_list);
            }
            if((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].dis_mixer_list) {
                free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].dis_mixer_list);
            }
            if((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].playback_dev_name) {
                free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].playback_dev_name);
            }
            if((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].capture_dev_name) {
                free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].card_ctrl[case_index].capture_dev_name);
            }
        }
        index = 0;
        while(1) {
            if ((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].device_list[index]) {
                if (!strncmp((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].device_list[index],
                    SND_UCM_END_OF_LIST, 3)) {
                    free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].device_list[index]);
                    break;
                } else {
                    free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].device_list[index]);
                    index++;
                }
            }
        }
        if ((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].device_list)
                free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].device_list);
        index = 0;
        while(1) {
            if ((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].modifier_list[index]) {
                if (!strncmp((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].modifier_list[index],
                    SND_UCM_END_OF_LIST, 3)) {
                    free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].modifier_list[index]);
                    break;
                } else {
                    free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].modifier_list[index]);
                    index++;
                }
            }
        }
        if ((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].modifier_list)
                free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].modifier_list);
        if((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].use_case_name)
            free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list[verb_index].use_case_name);
        if((*uc_mgr)->card_ctxt_ptr->verb_list[verb_index]) {
            free((*uc_mgr)->card_ctxt_ptr->verb_list[verb_index]);
        }
        verb_index++;
    }
    if((*uc_mgr)->card_ctxt_ptr->use_case_verb_list)
        free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list);
    if((*uc_mgr)->card_ctxt_ptr->verb_list)
        free((*uc_mgr)->card_ctxt_ptr->verb_list);
    pthread_mutex_unlock(&(*uc_mgr)->card_ctxt_ptr->card_lock);
}

/* Add an identifier to the respective list
 * head - list head
 * value - node value that needs to be added
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_add_ident_to_list(struct snd_ucm_ident_node **head, const char *value)
{
    struct snd_ucm_ident_node *temp, *node;

    node = (struct snd_ucm_ident_node *)malloc(sizeof(struct snd_ucm_ident_node));
    if (node == NULL) {
        LOGE("Failed to allocate memory for new node");
        return -ENOMEM;
    } else {
        node->next = NULL;
        strlcpy(node->ident, value, MAX_STR_LEN);
        node->active = 0;
    }
    if (*head == NULL) {
        *head = node;
    } else {
        temp = *head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = node;
    }
    LOGV("add_to_list: head %p, value %s", *head, node->ident);
    return 0;
}

/* Get the status of identifier at particulare index of the list
 * head - list head
 * ident - identifier value for which status needs to be get
 * status - status to be set (1 - active, 0 - inactive)
 */
static int snd_ucm_get_status_at_index(struct snd_ucm_ident_node *head, const char *ident)
{
    while (head != NULL) {
        if(!strncmp(ident, head->ident, (strlen(head->ident)+1))) {
            break;
        }
        head = head->next;
    }
    if (head == NULL) {
        LOGV("Element not found in the list");
    } else {
        return(head->active);
    }
    return -EINVAL;
}

/* Set the status of identifier at particulare index of the list
 * head - list head
 * ident - identifier value for which status needs to be set
 * status - status to be set (1 - active, 0 - inactive)
 */
static void snd_ucm_set_status_at_index(struct snd_ucm_ident_node *head, const char *ident, int status)
{
    while (head != NULL) {
        if(!strncmp(ident, head->ident, (strlen(head->ident)+1))) {
            break;
        }
        head = head->next;
    }
    if (head == NULL) {
        LOGE("Element not found to set the status");
    } else {
        head->active = status;
    }
}

/* Get the identifier value at particulare index of the list
 * head - list head
 * index - node index value
 * Returns node idetifier value at index on sucess, NULL otherwise
 */
static char *snd_ucm_get_value_at_index(struct snd_ucm_ident_node *head, int index)
{
    if (head == NULL) {
        LOGV("Empty list");
        return NULL;
    }

    if ((index < 0) || (index >= (snd_ucm_get_size_of_list(head)))) {
        LOGE("Element with given index %d doesn't exist in the list", index);
        return NULL;
    }

    while (index) {
        head = head->next;
        index--;
    }

    return (strdup(head->ident));
}

/* Get the size of the list
 * head - list head
 * Returns size of list on sucess, negative error code otherwise
 */
static int snd_ucm_get_size_of_list(struct snd_ucm_ident_node *head)
{
    int index = 0;

    if (head == NULL) {
        LOGV("Empty list");
        return 0;
    }

    while (head->next != NULL) {
        index++;
        head = head->next;
    }

    return (index+1);
}

static void snd_ucm_print_list(struct snd_ucm_ident_node *head)
{
    int index = 0;

    LOGV("print_list: head %p", head);
    if (head == NULL) {
        LOGV("Empty list");
        return;
    }

    while (head->next != NULL) {
        LOGV("index: %d, value: %s", index, head->ident);
        index++;
        head = head->next;
    }
    LOGV("index: %d, value: %s", index, head->ident);
}

/* Delete an identifier from respective list
 * head - list head
 * value - node value that needs to be deleted
 * Returns 0 on sucess, negative error code otherwise
 *
 */
static int snd_ucm_del_ident_from_list(struct snd_ucm_ident_node **head, const char *value)
{
    struct snd_ucm_ident_node *temp1, *temp2;
    int ret = -EINVAL;

    if (*head == NULL) {
        LOGE("del_from_list: Empty list");
        return -EINVAL;
    } else if (!strncmp((*head)->ident, value, (strlen(value)+1))) {
            temp2 = *head;
            *head = temp2->next;
            ret = 0;
    } else {
        temp1 = *head;
        temp2 = temp1->next;
        while (temp2 != NULL) {
            if (!strncmp(temp2->ident, value, (strlen(value)+1))) {
                temp1->next = temp2->next;
                ret = 0;
                break;
            }
            temp1 = temp1->next;
            temp2 = temp1->next;
        }
    }
    if (ret < 0) {
        LOGE("Element not found in enabled list");
    } else {
        temp2->next = NULL;
        temp2->ident[0] = 0;
        temp2->active = 0;
        free(temp2);
        temp2 = NULL;
    }
    return ret;
}
