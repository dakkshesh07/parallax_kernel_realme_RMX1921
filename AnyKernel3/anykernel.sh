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

write_boot;
ui_print "Thank you for installing Parallax Kernel"
## end boot install
