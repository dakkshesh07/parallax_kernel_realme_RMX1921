#!/system/bin/sh
# Please don't hardcode /magisk/modname/... ; instead, please use $MODDIR/...
# This will make your scripts compatible even if Magisk change its mount point in the future
MODDIR=${0%/*}

# This script will be executed in post-fs-data mode
# More info in the main Magisk thread

# Changes system.prop setting at Boot

#parallax touch tweaks

setprop touch.presure.scale=0.001
setprop persist.service.lgospd.enable=0
setprop persist.service.pcsync.enable=0
setprop ro.ril.enable.a52=1
setprop ro.ril.enable.a53=0

setprop persist.sys.ui.hw=1
setprop view.scroll_friction=10
setprop debug.composition.type=gpu
setprop debug.performance.tuning=1

#advanced touch settings

setprop touch.deviceType=touchScreen 
setprop touch.orientationAware=1 
setprop touch.size.calibration=diameter 
setprop touch.size.scale=1
setprop touch.size.bias=0
setprop touch.size.isSummed=0
setprop touch.pressure.calibration=physical 
setprop touch.pressure.scale=0.001 
setprop touch.orientation.calibration=none 
setprop touch.distance.calibration=none
setprop touch.distance.scale=0
setprop touch.coverage.calibration=box
setprop touch.gestureMode=spots
setprop MultitouchSettleInterval=1ms
setprop MultitouchMinDistance=1px
setprop TapInterval=1ms
setprop TapSlop=1px