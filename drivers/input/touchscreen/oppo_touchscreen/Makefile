#
# Makefile for the touchscreen drivers.
#

ccflags-y += -mllvm -inline-threshold=15000
ccflags-y += -mllvm -inlinehint-threshold=10000
obj-$(CONFIG_TOUCHPANEL_OPPO) += util_interface/
obj-$(CONFIG_TOUCHPANEL_OPPO) += touchpanel_common_driver.o touchpanel_prevention.o touchpanel_healthinfo.o
obj-$(CONFIG_TOUCHPANEL_SYNAPTICS) += Synaptics/
