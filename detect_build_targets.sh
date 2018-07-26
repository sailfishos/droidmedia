#!/bin/bash

RPM_ARCH=$1
ANDROID_ARCH=
# default to building whatever target arch defines
LIB_TARGET=

if [ $# -gt 1 ]; then # override arch
  ANDROID_ARCH=$2
else
  if [ -x "$(which droid-cmd)" ]; then # On OBS with new cmd
    >&2 droid-cmd "gettargetarch > lunch_arch || echo unknown > lunch_arch"
  elif [ -x "$(which droid-make)" ]; then # On OBS
    >&2 droid-make "clean > /dev/null; gettargetarch > lunch_arch || echo unknown > lunch_arch"
  fi

  if [ -f lunch_arch ]; then
    ANDROID_ARCH=`cat lunch_arch`
  fi
fi

if [ "$ANDROID_ARCH" == "" ]; then
  >&2 echo "Could not determine android architecture. Please pass it as 2nd argument using gettargetarch."
  exit 1
fi

if [ "$RPM_ARCH" != "aarch64" ] && [ "$ANDROID_ARCH" = "arm64" ]; then
    >&2 echo "Using 32bit targets for build"
    LIB_TARGET=_32
fi

echo libdroidmedia$LIB_TARGET minimediaservice minisfservice libminisf$LIB_TARGET
