# Android media wrapper library

droidmedia provides a device specific library that can be used by
gst-droid (https://github.com/sailfishos/gst-droid) to expose android
controlled hardware and codecs to gstreamer and the host linux system.

## Building and Hacking

Please check out the hardware adaption for your device, eg
https://docs.sailfishos.org/Develop/HW_Adaptation/Sailfish_X_Xperia_Android_11_Build_and_Flash/ for Sony Xperia 10 III.

For quick iterations it is usually sufficient to compile and transfer libdroidmedia.so:
```
make libdroidmedia
scp out/target/product/pdx213/system/lib64/libdroidmedia.so defaultuser@192.168.0.123:/home/defaultuser/
ssh defaultuser@192.168.0.123
devel-su cp libdroidmedia.so /usr/libexec/droid-hybris/system/lib64/libdroidmedia.so
```

You can revert to the currently installed version at any time using
```
zypper in -f droidmedia
```
