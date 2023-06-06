# Copyright (C), 2008-2030, OPPO Mobile Comm Corp., Ltd
### All rights reserved.
###
### File: - OplusKernelEnvConfig.mk
### Description:
###     you can get the oplus feature variables set in android side in this file
###     this file will add global macro for common oplus added feature
###     BSP team can do customzation by referring the feature variables
### Version: 1.0
### Date: 2020-03-18
### Author: Liang.Sun
###
### ------------------------------- Revision History: ----------------------------
### <author>                        <date>       <version>   <desc>
### ------------------------------------------------------------------------------
### Liang.Sun@TECH.Build              2020-03-18   1.0         Create this moudle
##################################################################################

-include ./oplus_native_features.mk

ALLOWED_MCROS := OPLUS_BUG_COMPATIBILITY \
OPLUS_BUG_STABILITY \
OPLUS_ARCH_EXTENDS \
OPLUS_FEATURE_AUDIO_FTM \
OPLUS_FEATURE_KTV \
VENDOR_EDIT \
OPLUS_FEATURE_CHG_BASIC  \
OPLUS_FEATURE_DUMPDEVICE


$(foreach myfeature,$(ALLOWED_MCROS),\
         $(eval KBUILD_CFLAGS += -D$(myfeature)) \
         $(eval KBUILD_CPPFLAGS += -D$(myfeature)) \
         $(eval CFLAGS_KERNEL += -D$(myfeature)) \
         $(eval CFLAGS_MODULE += -D$(myfeature)) \
)

ifeq ($(OPLUS_FEATURE_DUMPDEVICE),yes)
export CONFIG_OPLUS_FEATURE_DUMP_DEVICE_INFO=y
KBUILD_CFLAGS += -DCONFIG_OPLUS_FEATURE_DUMP_DEVICE_INFO
endif

export CONFIG_BRAND_SHOW_FLAG=realme
KBUILD_CFLAGS += -DCONFIG_BRAND_SHOW_FLAG
