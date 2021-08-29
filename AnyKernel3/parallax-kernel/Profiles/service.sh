#!/system/bin/sh
MODDIR=${0%/*}
# Magisk Busybox Symlink for Apps
ln -s /sbin/.magisk/busybox/* /system/bin/

for i in $MODDIR/config/*; do
  case $i in
    *-ls|*-ls.sh);;
    *) if [ -f "$i" -a -x "$i" ]; then $i & fi;;
  esac
done