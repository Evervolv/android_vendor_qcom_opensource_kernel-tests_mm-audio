/*
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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
#ifndef _MSM8960_USE_CASES_H_
#define _MSM8960_USE_CASES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "alsa_ucm.h"
#include "alsa_audio.h"
#define SND_UCM_END_OF_LIST "end"

/* ACDB Device ID macros */
#define CAP_RX 0x1
#define CAP_TX 0x2
#define CAP_VOICE 0x4
#define DEVICE_HANDSET_RX_ACDB_ID                       7 // HANDSET_SPKR
#define DEVICE_HANDSET_TX_ACDB_ID                       4 // HANDSET_MIC
#define DEVICE_SPEAKER_RX_ACDB_ID                       15// SPKR_PHONE_SPKR_STEREO
#define DEVICE_SPEAKER_TX_ACDB_ID                       11// SPKR_PHONE_MIC
#define DEVICE_HEADSET_RX_ACDB_ID                       10// HEADSET_SPKR_STEREO
#define DEVICE_HEADSET_TX_ACDB_ID                       8 // HEADSET_MIC
#define DEVICE_DUALMIC_HANDSET_TX_BROADSIDE_ACDB_ID     5 // HANDSET_MIC_BROADSIDE
#define DEVICE_DUALMIC_HANDSET_TX_ENDFIRE_ACDB_ID       6 // HANDSET_MIC_ENDFIRE
#define DEVICE_DUALMIC_SPEAKER_TX_BROADSIDE_ACDB_ID     12// SPKR_PHONE_MIC_BROADSIDE
#define DEVICE_DUALMIC_SPEAKER_TX_ENDFIRE_ACDB_ID       13// SPKR_PHONE_MIC_ENDFIRE
#define DEVICE_TTY_HEADSET_MONO_RX_ACDB_ID              17// TTY_HEADSET_SPKR
#define DEVICE_TTY_HEADSET_MONO_TX_ACDB_ID              16// TTY_HEADSET_MIC
#define DEVICE_BT_SCO_RX_ACDB_ID                        22// BT_SCO_SPKR
#define DEVICE_BT_SCO_TX_ACDB_ID                        21// BT_SCO_SPKR
#define DEVICE_BT_SCO_RX_WB_ACDB_ID                     39// BT_SCO_WB_SPKR
#define DEVICE_BT_SCO_TX_WB_ACDB_ID                     38// BT_SCO_WB_MIC
#define DEVICE_SPEAKER_HEADSET_RX_ACDB_ID               DEVICE_HEADSET_RX_ACDB_ID // Use headset calibration
#define DEVICE_HDMI_STEREO_RX_ACDB_ID                   18// HDMI_SPKR
#define DEVICE_ANC_HEADSET_STEREO_RX_ACDB_ID            26//ANC RX, same as regular headset

/* mixer control type */
#define TYPE_INT            0
#define TYPE_STR            1

/* Maximum string length of use case and device combination */
#define MAX_UC_LEN 100
/* Maximum string length of use case or device */
#define MAX_STR_LEN 50

/* Get mixer control count */
#define NO_OF_CONTROLS(name) (sizeof(name)/sizeof(mixer_control_t))

/* mixer control structure */
typedef struct mixer_control {
    char *control_name;
    unsigned type;
    unsigned value;
    char *string;
}mixer_control_t;

/* Use case mixer controls structure */
typedef struct card_mctrl {
    char *case_name;
    int ena_mixer_count;
    mixer_control_t *ena_mixer_list;
    int dis_mixer_count;
    mixer_control_t *dis_mixer_list;
    char *playback_dev_name;
    char *capture_dev_name;
    int acdb_id;
    int capability;
}card_mctrl_t;

/* identifier node structure for identifier list*/
struct snd_ucm_ident_node {
    int active;
    char ident[MAX_STR_LEN];
    struct snd_ucm_ident_node *next;
};

/* Structure to maintain the valid devices and
 * modifiers list per each use case */
typedef struct use_case_verb {
    char *use_case_name;
    char **device_list;
    char **modifier_list;
    int use_case_count;
    card_mctrl_t *card_ctrl;
}use_case_verb_t;

/* SND card context structure */
typedef struct card_ctxt {
    char *card_name;
    int card_number;
    char *control_device;
    struct mixer *mixer_handle;
    char current_verb[MAX_STR_LEN];
    struct snd_ucm_ident_node *dev_list_head;
    struct snd_ucm_ident_node *mod_list_head;
    pthread_mutex_t card_lock;
    pthread_mutexattr_t card_lock_attr;
    int current_verb_index;
    use_case_verb_t *use_case_verb_list;
}card_ctxt_t;

/** use case manager structure */
struct snd_use_case_mgr {
    int snd_card_index;
    int device_list_count;
    int modifier_list_count;
    char **current_device_list;
    char **current_modifier_list;
    int current_tx_device;
    int current_rx_device;
    card_ctxt_t *card_ctxt_ptr;
};

/* Context list per each sound card */
static card_ctxt_t card_ctxt_list[] = {
    { "snd_soc_msm", 0, "/dev/snd/controlC0", NULL, "Inactive", NULL, NULL, NULL, NULL, 0, NULL,
    },
};

#define MAX_NUM_CARDS (sizeof(card_ctxt_list)/sizeof(struct card_ctxt))

/* Valid sound cards list */
static const char *card_list[] = {
    "snd_soc_msm",
};

/* New use cases, devices and modifiers added
 * which are not part of existing macros
 */
#define SND_USE_CASE_VERB_FM_REC         "FM REC"
#define SND_USE_CASE_VERB_FM_A2DP_REC   "FM A2DP REC"
#define SND_USE_CASE_VERB_HIFI_REC       "HiFi Rec"

#define SND_USE_CASE_DEV_FM_TX           "FM Tx"
#define SND_USE_CASE_DEV_ANC_HEADSET     "ANC Headset"
#define SND_USE_CASE_DEV_BTSCO_NB_RX        "BT SCO Rx"
#define SND_USE_CASE_DEV_BTSCO_NB_TX        "BT SCO Tx"
#define SND_USE_CASE_DEV_BTSCO_WB_RX        "BT SCO WB Rx"
#define SND_USE_CASE_DEV_BTSCO_WB_TX        "BT SCO WB Tx"
#define SND_USE_CASE_DEV_SPEAKER_HEADSET "Speaker Headset"
#define SND_USE_CASE_DEV_SPEAKER_ANC_HEADSET "Speaker ANC Headset"
#define SND_USE_CASE_DEV_SPEAKER_FM_TX   "Speaker FM Tx"
#define SND_USE_CASE_DEV_TTY_HEADSET_RX  "TTY Headset Rx"
#define SND_USE_CASE_DEV_TTY_HEADSET_TX  "TTY Headset Tx"
#define SND_USE_CASE_DEV_TTY_FULL_RX  "TTY Full Rx"
#define SND_USE_CASE_DEV_TTY_FULL_TX  "TTY Full Tx"
#define SND_USE_CASE_DEV_DUAL_MIC_BROADSIDE "DMIC Broadside"
#define SND_USE_CASE_DEV_DUAL_MIC_ENDFIRE "DMIC Endfire"
#define SND_USE_CASE_DEV_SPEAKER_DUAL_MIC_BROADSIDE "Speaker DMIC Broadside"
#define SND_USE_CASE_DEV_SPEAKER_DUAL_MIC_ENDFIRE "Speaker DMIC Endfire"
#define SND_USE_CASE_DEV_ALL             "all devices"

#define SND_USE_CASE_MOD_PLAY_FM         "Play FM"
#define SND_USE_CASE_MOD_CAPTURE_FM      "Capture FM"
#define SND_USE_CASE_MOD_CAPTURE_A2DP_FM "Capture A2DP FM"
#define SND_USE_CASE_MOD_PLAY_LPA        "Play LPA"
#define SND_USE_CASE_MOD_PLAY_VOIP       "Voice Call IP"
#define SND_USE_CASE_MOD_CAPTURE_VOIP    "Capture VOIP"

/* List of valid use cases */
static const char *verb_list[] = {
    SND_USE_CASE_VERB_INACTIVE,
    SND_USE_CASE_VERB_HIFI,
    SND_USE_CASE_VERB_HIFI_LOW_POWER,
    SND_USE_CASE_VERB_VOICECALL,
    SND_USE_CASE_VERB_IP_VOICECALL,
    SND_USE_CASE_VERB_DIGITAL_RADIO,
    SND_USE_CASE_VERB_FM_REC,
    SND_USE_CASE_VERB_FM_A2DP_REC,
    SND_USE_CASE_VERB_HIFI_REC,
    SND_UCM_END_OF_LIST,
};

/* List of valid devices */
static const char *device_list[] = {
    SND_USE_CASE_DEV_NONE,
    SND_USE_CASE_DEV_SPEAKER,    /* SPEAKER RX */
    SND_USE_CASE_DEV_LINE,       /* BUILTIN-MIC TX */
    SND_USE_CASE_DEV_HEADPHONES, /* HEADSET RX */
    SND_USE_CASE_DEV_HEADSET,    /* HEADSET TX */
    SND_USE_CASE_DEV_HANDSET,    /* HANDSET TX */
    SND_USE_CASE_DEV_EARPIECE,   /* HANDSET RX */
    SND_USE_CASE_DEV_ANC_HEADSET, /* ANC HEADSET RX */
    SND_USE_CASE_DEV_SPEAKER_HEADSET, /* COMBO SPEAKER+HEADSET RX */
    SND_USE_CASE_DEV_SPEAKER_ANC_HEADSET, /* COMBO SPEAKER+ANC HEADSET RX */
    SND_USE_CASE_DEV_SPEAKER_FM_TX, /* COMBO SPEAKER+FM_TX RX */
    SND_USE_CASE_DEV_BTSCO_NB_RX,      /* BTSCO RX*/
    SND_USE_CASE_DEV_BTSCO_NB_TX,      /* BTSCO TX*/
    SND_USE_CASE_DEV_BTSCO_WB_RX,      /* BTSCO RX*/
    SND_USE_CASE_DEV_BTSCO_WB_TX,      /* BTSCO TX*/
    SND_USE_CASE_DEV_FM_TX,       /* FM RX */
    SND_USE_CASE_DEV_BLUETOOTH,  /* BLUETOOTH */
    SND_USE_CASE_DEV_HDMI,       /* HDMI RX */
    SND_USE_CASE_DEV_TTY_HEADSET_RX,    /*TTY HEADSET MONO RX */
    SND_USE_CASE_DEV_TTY_HEADSET_TX,    /*TTY HEADSET MONO TX */
    SND_USE_CASE_DEV_TTY_FULL_RX,    /*TTY FULL RX */
    SND_USE_CASE_DEV_TTY_FULL_TX,    /*TTY FULL TX */
    SND_USE_CASE_DEV_DUAL_MIC_BROADSIDE, /* DMIC Broadside */
    SND_USE_CASE_DEV_DUAL_MIC_ENDFIRE,    /* DMIC Endfire */
    SND_USE_CASE_DEV_SPEAKER_DUAL_MIC_BROADSIDE, /* DMIC Broadside */
    SND_USE_CASE_DEV_SPEAKER_DUAL_MIC_ENDFIRE,    /* DMIC Endfire */
    SND_UCM_END_OF_LIST,
};

/* List of valid modifiers */
static const char *mod_list[] = {
    SND_USE_CASE_MOD_CAPTURE_VOICE,
    SND_USE_CASE_MOD_CAPTURE_MUSIC,
    SND_USE_CASE_MOD_PLAY_MUSIC,
    SND_USE_CASE_MOD_PLAY_VOICE,
    SND_USE_CASE_MOD_PLAY_FM,
    SND_USE_CASE_MOD_CAPTURE_FM,
    SND_USE_CASE_MOD_CAPTURE_A2DP_FM,
    SND_USE_CASE_MOD_PLAY_LPA,
    SND_USE_CASE_MOD_PLAY_VOIP,
    SND_USE_CASE_MOD_CAPTURE_VOIP,
    SND_UCM_END_OF_LIST,
};

/* List utility functions for maintaining enabled devices and modifiers */
static int snd_ucm_add_ident_to_list(struct snd_ucm_ident_node **head, const char *value);
static char *snd_ucm_get_value_at_index(struct snd_ucm_ident_node *head, int index);
static int snd_ucm_get_size_of_list(struct snd_ucm_ident_node *head);
static int snd_ucm_del_ident_from_list(struct snd_ucm_ident_node **head, const char *value);
static int snd_ucm_free_list(struct snd_ucm_ident_node **head);
static void snd_ucm_print_list(struct snd_ucm_ident_node *head);
static void snd_ucm_set_status_at_index(struct snd_ucm_ident_node *head, const char *ident, int status);
static int snd_ucm_get_status_at_index(struct snd_ucm_ident_node *head, const char *ident);
/* Parse functions */
static int snd_ucm_parse(snd_use_case_mgr_t **uc_mgr);
static int snd_ucm_parse_section(snd_use_case_mgr_t **uc_mgr, char **cur_str, char **nxt_str, int verb_index);
static int snd_ucm_extract_name(char *buf, char **case_name);
static int snd_ucm_extract_acdb(char *buf, int *id, int *cap);
static int snd_ucm_extract_dev_name(char *buf, char **dev_name);
static int snd_ucm_extract_controls(char *buf, mixer_control_t **mixer_list, int count);
static int snd_ucm_print(snd_use_case_mgr_t *uc_mgr);
static void snd_ucm_free_mixer_list(snd_use_case_mgr_t **uc_mgr);
#ifdef __cplusplus
}
#endif

#endif
