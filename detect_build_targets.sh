#!/bin/bash

# default to building whatever target arch defines
LIB_TARGET=

if [ "$1" == "aarch64" ]; then
    : # noop
else
    ANDROID_ARCH=`grep -h -m 1 "TARGET_ARCH *:=" device/*/*/*.mk | sed -e 's/ *TARGET_ARCH *:= *\([a-zA-Z0-9_]*\) */\1/'`
    if [ "$ANDROID_ARCH" == "arm64" ]; then
        LIB_TARGET=_32
    fi
fi

echo libdroidmedia$LIB_TARGET minimediaservice minisfservice libminisf$LIB_TARGET
