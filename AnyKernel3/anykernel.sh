# AnyKernel3 Ramdisk Mod Script
# osm0sis @ xda-developers

## AnyKernel setup
# begin properties
properties() { '
kernel.string=Parallax Kernel For Realme XT
do.devicecheck=1
do.modules=0
do.systemless=0
do.cleanup=1
do.cleanuponabort=0
device.name1=RMX1921
device.name2=RMX1921EU
supported.versions=11 - 12
'; } # end properties

# shell variables
block=/dev/block/bootdevice/by-name/boot;
is_slot_device=0;
ramdisk_compression=auto;
patch_vbmeta_flag=auto;


## AnyKernel methods (DO NOT CHANGE)
# import patching functions/variables - see for reference
. tools/ak3-core.sh;

## AnyKernel boot install
dump_boot;

if [ -d /data/adb/modules/parallax-kernel ]; then
  ui_print ""
  ui_print "  • Parallax Profile installation found, removing"
  rm -rf /data/adb/modules/parallax-kernel
fi
cp $home/parallax-kernel/image/Image.gz-dtb $home/Image.gz-dtb;

ui_print ""

if mountpoint -q /data; then
  # Optimize F2FS extension list (@arter97)
  for list_path in $(find /sys/fs/f2fs* -name extension_list); do
    hash="$(md5sum $list_path | cut -d' ' -f1)"

    # Skip update if our list is already active
    if [[ $hash == "0e2627998f2f06aa951f90d2af30e859" ]]; then
      echo "Extension list up-to-date: $list_path"
      continue
    fi

    ui_print "  • Optimizing F2FS extension list"
    echo "Updating extension list: $list_path"

    echo "Clearing extension list"

    hot_count="$(cat $list_path | grep -n 'hot file extension' | cut -d : -f 1)"
    cold_count="$(($(cat $list_path | wc -l) - $hot_count))"

    cold_list="$(head -n$(($hot_count - 1)) $list_path | grep -v ':')"
    hot_list="$(tail -n$cold_count $list_path)"

    for ext in $cold_list; do
      [ ! -z $ext ] && echo "[c]!$ext" > $list_path
    done

    for ext in $hot_list; do
      [ ! -z $ext ] && echo "[h]!$ext" > $list_path
    done

    echo "Writing new extension list"

    for ext in $(cat $home/f2fs-cold.list | grep -v '#'); do
      [ ! -z $ext ] && echo "[c]$ext" > $list_path
    done

    for ext in $(cat $home/f2fs-hot.list); do
      [ ! -z $ext ] && echo "[h]$ext" > $list_path
    done
  done
fi

write_boot;
ui_print "Thank you for installing Parallax Kernel"
## end boot install
