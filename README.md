# Android media wrapper library

droidmedia provides a device specific library that can be used by
gst-droid (https://github.com/sailfishos/gst-droid) to expose android
controlled hardware and codecs to GStreamer and the host linux system.

Please see also https://docs.sailfishos.org/Reference/Core_Areas_and_APIs/Multimedia/ for an overview of the Sailfish Multimedia system.

## Building and Hacking

Please check out the hardware adaption for your device, eg
https://docs.sailfishos.org/Develop/HW_Adaptation/Sailfish_X_Xperia_Android_11_Build_and_Flash/ for Sony Xperia 10 III.

For quick iterations it is usually sufficient to compile and transfer libdroidmedia.so:
```
sfossdk
hadk
ubu-chroot -r $PLATFORM_SDK_ROOT/sdks/ubuntu
cd $ANDROID_ROOT

source build/envsetup.sh
lunch aosp_$DEVICE-user

make libdroidmedia
scp out/target/product/pdx213/system/lib64/libdroidmedia.so defaultuser@192.168.0.123:/home/defaultuser/
ssh defaultuser@192.168.0.123
devel-su cp libdroidmedia.so /usr/libexec/droid-hybris/system/lib64/libdroidmedia.so
```

Now iterate on the code in `$ANDROID_ROOT/external/droidmedia` and repeat the last steps to build and copy as needed.

You can revert to the currently installed version at any time using
```
zypper in -f droidmedia
```
