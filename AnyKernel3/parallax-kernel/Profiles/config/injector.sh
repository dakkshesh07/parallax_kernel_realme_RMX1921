#!/system/bin/sh

config=/data/adb/modules/profiles/config

sleep 10
if [ -e $config/fkm ]; then

if [ -e $config/spectrum ]; then
       setprop persist.spectrum.profile 0
fi

