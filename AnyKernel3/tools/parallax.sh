INSTALLER=/tmp/anykernel
KEYCHECK=$INSTALLER/tools/keycheck
chmod 755 $KEYCHECK

keytest() {
  ui_print "- Volume Key Test -"
  ui_print "   Press Volume key Up:"
  (/system/bin/getevent -lc 1 2>&1 | /system/bin/grep VOLUME | /system/bin/grep " DOWN" > $INSTALLER/events) || return 1
  return 0
}   

choose() {
  #note from chainfire @xda-developers: getevent behaves weird when piped, and busybox grep likes that even less than toolbox/toybox grep
  while (true); do
    /system/bin/getevent -lc 1 2>&1 | /system/bin/grep VOLUME | /system/bin/grep " DOWN" > $INSTALLER/events
    if (`cat $INSTALLER/events 2>/dev/null | /system/bin/grep VOLUME >/dev/null`); then
      break
    fi
  done
  if (`cat $INSTALLER/events 2>/dev/null | /system/bin/grep VOLUMEUP >/dev/null`); then
    return 0
  else
    return 1
  fi
}

chooseold() {
  # Calling it first time detects previous input. Calling it second time will do what we want
  $KEYCHECK
  $KEYCHECK
  SEL=$?
  if [ "$1" == "UP" ]; then
    UP=$SEL
  elif [ "$1" == "DOWN" ]; then
    DOWN=$SEL
  elif [ $SEL -eq $UP ]; then
    return 0
  elif [ $SEL -eq $DOWN ]; then
    return 1
  else
    ui_print "   Vol key not detected!"
    abort "   Use name change method in TWRP"
  fi
}
ui_print " "
if keytest; then
  FUNCTION=choose
else
  FUNCTION=chooseold
  ui_print "   ! Legacy device detected! Using old keycheck method"
  ui_print " "
  ui_print "- Vol Key Programming -"
  ui_print "   Press Volume Up Again:"
  $FUNCTION "UP"
  ui_print "   Press Volume Down"
  $FUNCTION "DOWN"
fi
ui_print ""
ui_print "-> Do you want Profiles? -"
ui_print "   Volume up = Yes, Volume down = No"
if $FUNCTION; then
ui_print "-> Installing Profiles...";
ui_print "-> profiles can be changed";
ui_print "-> using spectrum app...";
MODMagisk=/data/adb/modules/parallax-kernel
# Place MOD options
rm -rf $MODMagisk
mkdir -p $MODMagisk
cp -Rf /tmp/anykernel/parallax-kernel/Profiles/* $MODMagisk
chmod 755 $MODMagisk/config/injector.sh
chmod 755 $MODMagisk/config/spectrum
chmod 755 $MODMagisk/service.sh
chmod 755 $MODMagisk/post-fs-data.sh
else
ui_print "-> Selected 'no'"
ui_print "-> Skipping Profile installtion"
fi