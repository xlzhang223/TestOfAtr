# TestOfAtr
## 1 相关环境
* 开发系统：Ubuntu-18.04
* 测试设备：Pixel 骁龙821处理器 4GB RAM
* Android版本：android-10.0.0_r2 
* build target： aosp_sailfish-userdebug
* target build type：release
## 2 安装过程
### 2.1 ART 修改部分 
#### 1. 系统源代码下载  
首先下载系统源代码，具体步骤参考谷歌官网。（<https://source.android.com/source/requirements.html> ）。选择并下载`android-10.0.0_r2`版本源代码，如果无法连接或者速度不稳定可以采用清华的的源来进行下载（<https://mirrors.tuna.tsinghua.edu.cn/help/AOSP/>）。
#### 2. 编译源代码
由于部分硬件驱动是非开源的，为了确保系统可用需要下载相应固件。在实验环境中要刷到`pixel`上面，首先要去Google下载相应的固件（**注意对应系统型号和手机型号**）
下载地址：<https://developers.google.com/android/drivers#sailfishopr6.170623.011>  
下载完成后执行以下两个文件：
```sh
./extract-google_devices-sailfish.sh
./extract-qcom-sailfish.sh
```
然后就进行编译即可,注意需要`lunch`的版本的选择，这里选择`aosp_sailfish-userdebug`
```sh
. build/envsetup.sh
lunch aosp_sailfish-userdebug
make -j8
```
#### 3. 刷机
官方教程：<https://source.android.com/setup/build/running.html#flashing-a-device>  
要刷写设备，请执行以下操作：  
在启动时按住相应的组合键或使用以下命令使设备进入 fastboot 模式：  
`adb reboot bootloader`
在设备处于 fastboot 模式后，运行以下命令：  
`fastboot flashall -w`
#### 4. 将修改的代码写入设备
用该项目替换掉`/art`文件夹（或使用patch命令对原art进行patch），之后重新编译，在源代码的根目录下执行执行：
```sh
. build/envsetup.sh
lunch aosp_sailfish-userdebug
mm
```
之后连接手机，确保手机可以 remount ：
```sh
adb devices
adb root
adb disable-verity
adb reboot
adb root 
adb remount
```
之后通过 adb 可以将编译后的库 push 到手机上：
```sh
cd out/target/product/sailfish/system/
adb push lib /system
```
#### 5. 配置文件
将需要监控的app写入配置文件app.txt并push到手机上即可。
```
adb root
adb remount
adb push app.txt data/local/tmp
adb push GC_K.txt /data/local/tmp/
adb push Tsize.txt /data/local/tmp/

app.txt 表示要监测的应用程序（与pid获得的进程名对应）
GC_K.txt 表示要监测机制触发间隔CG次数（如：8）
Tsize.txt 表示要监测的对象大小阈值（如：48）
```
以dalvikvm启动的程序为例：
将项目中的三个文本格式文件上传到手机中，即可监测dalvikvm启动的程序，并以8为间隔，追踪大于48字节的对象。
#### 6.输出结果
所有输出结果在data/local/tmp/result.txt中
可以使用
```
adb pull data/local/tmp/result.txt {your_path}
```
将输出文本dump到本地。
#### 7.eembc测试
当确保执行完步骤4（修改后的代码部署到开发机后），可将测试的配置文件及测试dex分别push到机器上进行测试。
```
adb root                                              
adb remount                                           //进行挂载
adb push app.txt data/local/tmp 
adb push GC_K.txt /data/local/tmp/
adb push Tsize.txt /data/local/tmp/                   //push相关配置
adb push eembcBM.jar data/local/tmp/
adb shell                                             //adb shell下启动eembcBM
cd data/local/tmp
dalvikvm -cp eembcBM.jar com.sun.mep.bench.main       //运行eembcBM
exit
adb pull data/local/tmp/result.txt                    //抓取结果到本地
```
项目附带了eembc及其子程序的dex，如需测试其他java程序，需要用当前版本的dx工具将.class转换为.dex。
