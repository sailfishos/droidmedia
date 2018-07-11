#!/bin/bash

# default to building whatever target arch defines
LIB_TARGET=

if [ "$1" == "aarch64" ]; then
    : # noop
else
    ANDROID_ARCH=`cat lunch_arch`
    if [ "$ANDROID_ARCH" == "arm64" ]; then
        LIB_TARGET=_32
    fi
fi

echo libdroidmedia$LIB_TARGET minimediaservice minisfservice libminisf$LIB_TARGET
