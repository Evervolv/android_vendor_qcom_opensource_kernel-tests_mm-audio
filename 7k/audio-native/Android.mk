#--------------------------------------------------------------------------
#Copyright (c) 2009, Code Aurora Forum. All rights reserved.

#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#    * Neither the name of Code Aurora nor
#      the names of its contributors may be used to endorse or promote
#      products derived from this software without specific prior written
#      permission.

#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
#ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#--------------------------------------------------------------------------

ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
#                               Common definitons
# ---------------------------------------------------------------------------------

mm-audio-native-def := -g -O3
mm-audio-native-def += -DQC_MODIFIED
mm-audio-native-def += -D_ANDROID_
mm-audio-native-def += -DFEATURE_EXPORT_SND
mm-audio-native-def += -DFEATURE_AUDIO_AGC
mm-audio-native-def += -DFEATURE_VOC_PCM_INTERFACE
mm-audio-native-def += -DFEATURE_VOICE_PLAYBACK
mm-audio-native-def += -DFEATURE_VOICE_RECORD
mm-audio-native-def += -DVERBOSE
mm-audio-native-def += -D_DEBUG

ifeq ($(BOARD_USES_QCOM_AUDIO_V2), true)
mm-audio-native-def += -DAUDIOV2
endif

ifeq ($(strip $(QC_PROP)),true)
mm-audio-native-def += -DQC_PROP
endif
# ---------------------------------------------------------------------------------
#                       Make the apps-test (mm-audio-native-test)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

ifeq ($(strip $(QC_PROP)),true)
mm-audio-native-inc     += $(TARGET_OUT_HEADERS)/mm-audio/audio-alsa
endif

LOCAL_MODULE            := mm-audio-native-test
LOCAL_CFLAGS            := $(mm-audio-native-def)
LOCAL_PRELINK_MODULE    := false

ifeq ($(strip $(QC_PROP)),true)
LOCAL_C_INCLUDES        := $(mm-audio-native-inc)
LOCAL_SHARED_LIBRARIES  := libaudioalsa
endif

LOCAL_SRC_FILES := audiotest.c
LOCAL_SRC_FILES += mp3test.c
LOCAL_SRC_FILES += pcmtest.c
LOCAL_SRC_FILES += qcptest.c
LOCAL_SRC_FILES += aactest.c
LOCAL_SRC_FILES += amrnbtest.c
LOCAL_SRC_FILES += amrwbtest.c
LOCAL_SRC_FILES += wmatest.c
LOCAL_SRC_FILES += wmaprotest.c
LOCAL_SRC_FILES += voicememotest.c
LOCAL_SRC_FILES += audioprofile.c
LOCAL_SRC_FILES += snddevtest.c

ifeq ($(BOARD_USES_QCOM_AUDIO_V2), true)
LOCAL_SRC_FILES += adpcmtest.c
endif

include $(BUILD_EXECUTABLE)

endif #BUILD_TINY_ANDROID

# ---------------------------------------------------------------------------------
#                                       END
# ---------------------------------------------------------------------------------

