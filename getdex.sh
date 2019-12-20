# java -> class -> dex
cd ../../java/jdk1.6.0_45/bin/

cp ../../../xlz/art/ListLeak.java .

./javac ListLeak.java

cp ListLeak.class ../../../xlz/prebuilts/sdk/tools/
cp Student.class ../../../xlz/prebuilts/sdk/tools/

 cd ../../../xlz/prebuilts/sdk/tools/

./dx --dex --output=ListLeak.dex ListLeak.class
./dx --dex --output=Student.dex Student.class

adb push ListLeak.dex /data/local/tmp
adb push Student.dex /data/local/tmp

