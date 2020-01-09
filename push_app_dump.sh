adb root
adb remount
adb push dump.sh data/local/tmp
adb push app.txt data/local/tmp
adb push GC_K.txt /data/local/tmp/
adb push Tsize.txt /data/local/tmp/
