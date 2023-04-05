# Android makefile for audio kernel modules
MY_LOCAL_PATH := $(call my-dir)

UAPI_OUT := $(PRODUCT_OUT)/obj/vendor/qcom/opensource/audio-kernel/include

ifeq ($(call is-board-platform-in-list,msm8909 msm8953 msm8937 sdm845 sdm710 qcs605),true)
$(shell mkdir -p $(UAPI_OUT)/linux;)
$(shell mkdir -p $(UAPI_OUT)/sound;)
$(shell rm -rf $(PRODUCT_OUT)/obj/vendor/qcom/opensource/audio-kernel/ipc/Module.symvers)
$(shell rm -rf $(PRODUCT_OUT)/obj/vendor/qcom/opensource/audio-kernel/dsp/Module.symvers)
$(shell rm -rf $(PRODUCT_OUT)/obj/vendor/qcom/opensource/audio-kernel/dsp/codecs/Module.symvers)
$(shell rm -rf $(PRODUCT_OUT)/obj/vendor/qcom/opensource/audio-kernel/soc/Module.symvers)
$(shell rm -rf $(PRODUCT_OUT)/obj/vendor/qcom/opensource/audio-kernel/asoc/Module.symvers)
$(shell rm -rf $(PRODUCT_OUT)/obj/vendor/qcom/opensource/audio-kernel/asoc/codecs/Module.symvers)
$(shell rm -rf $(PRODUCT_OUT)/obj/vendor/qcom/opensource/audio-kernel/asoc/codecs/wcd934x/Module.symvers)

include $(MY_LOCAL_PATH)/include/uapi/Android.mk
include $(MY_LOCAL_PATH)/ipc/Android.mk
include $(MY_LOCAL_PATH)/dsp/Android.mk
include $(MY_LOCAL_PATH)/dsp/codecs/Android.mk
include $(MY_LOCAL_PATH)/soc/Android.mk
include $(MY_LOCAL_PATH)/asoc/Android.mk
include $(MY_LOCAL_PATH)/asoc/codecs/Android.mk
include $(MY_LOCAL_PATH)/asoc/codecs/wcd934x/Android.mk
endif

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_DLKM_8909W)),true)
$(shell rm -rf $(PRODUCT_OUT)/obj/vendor/qcom/opensource/audio-kernel/asoc/codecs/msm_bg/Module.symvers)
include $(MY_LOCAL_PATH)/asoc/codecs/msm_bg/Android.mk
endif

ifeq ($(call is-board-platform-in-list,msm8953 msm8937 sdm710 qcs605 msm8909),true)
$(shell rm -rf $(PRODUCT_OUT)/obj/vendor/qcom/opensource/audio-kernel/asoc/codecs/sdm660_cdc/Module.symvers)
$(shell rm -rf $(PRODUCT_OUT)/obj/vendor/qcom/opensource/audio-kernel/asoc/codecs/msm_sdw/Module.symvers)
include $(MY_LOCAL_PATH)/asoc/codecs/sdm660_cdc/Android.mk
include $(MY_LOCAL_PATH)/asoc/codecs/msm_sdw/Android.mk
endif

#ifdef OPLUS_ARCH_EXTENDS
#Kaiqin.Huang@RM.MM.AudioDriver.Codec, 2019/10/12, Add for tfa9890 codec
include $(MY_LOCAL_PATH)/asoc/codecs/tfa98xx/Android.mk
#endif

#ifdef OPLUS_ARCH_EXTENDS
#Jianfeng.Qiu@PSW.MM.AudioDriver.Codec, 2021/03/19, add for tfa98xx driver build
include $(MY_LOCAL_PATH)/asoc/codecs/tfa98xx-v6/Android.mk
#endif


#ifdef OPLUS_ARCH_EXTENDS
#Jianfeng.Qiu@PSW.MM.AudioDriver.Codec, 2018/04/20, add for ak4376 driver build
include $(MY_LOCAL_PATH)/asoc/codecs/ak4376/Android.mk
#endif /* OPLUS_ARCH_EXTENDS */
#ifdef OPLUS_ARCH_EXTENDS
#xiang.fei@Multimedia.AudioDriver.Codec, 2018/10/29, Add for dbmdx
include $(MY_LOCAL_PATH)/asoc/codecs/dbmdx/Android.mk
#endif /* OPLUS_ARCH_EXTENDS */
