#!/bin/bash
set -ex

echo -e "\e[1;32mInstall cross compile tools\e[0m"
# Setup tools
if [ ! -d $PWD/tools ]; then
    git clone --depth 1 https://github.com/raspberrypi/tools
fi

# Cross build for the Raspberry Pi
mkdir -p $PWD/tmp
BUILD_PREFIX=$PWD/tmp
TOOLCHAIN_HOST="arm-linux-gnueabihf"
TOOLCHAIN_PATH="/home/sappo/workspace/rpi-tools/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/bin"
SYSROOT=$PWD/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/arm-linux-gnueabihf/sysroot

CFLAGS="--sysroot=${SYSROOT} -I${BUILD_PREFIX}/include"
CPPFLAGS="--sysroot=${SYSROOT} -I${BUILD_PREFIX}/include"
CXXFLAGS="--sysroot=${SYSROOT} -I${BUILD_PREFIX}/include"
LDFLAGS="-L${BUILD_PREFIX}/lib"

CONFIG_OPTS=()
CONFIG_OPTS+=("CFLAGS=${CFLAGS}")
CONFIG_OPTS+=("CPPFLAGS=${CPPFLAGS}")
CONFIG_OPTS+=("CXXFLAGS=${CXXFLAGS}")
CONFIG_OPTS+=("LDFLAGS=${LDFLAGS}")
CONFIG_OPTS+=("PKG_CONFIG_DIR=")
CONFIG_OPTS+=("PKG_CONFIG_LIBDIR=${SYSROOT}/usr/lib/arm-linux-gnueabihf/pkgconfig:${SYSROOT}/usr/share/pkgconfig")
CONFIG_OPTS+=("PKG_CONFIG_SYSROOT=${SYSROOT}")
CONFIG_OPTS+=("PKG_CONFIG_PATH=${BUILD_PREFIX}/lib/pkgconfig")
CONFIG_OPTS+=("--prefix=${BUILD_PREFIX}")
CONFIG_OPTS+=("--host=${TOOLCHAIN_HOST}")
CONFIG_OPTS+=("--with-docs=no")

CPP="${TOOLCHAIN_PATH}/${TOOLCHAIN_HOST}-cpp"
CC="${TOOLCHAIN_PATH}/${TOOLCHAIN_HOST}-gcc"
CXX="${TOOLCHAIN_PATH}/${TOOLCHAIN_HOST}-g++"
LD="${TOOLCHAIN_PATH}/${TOOLCHAIN_HOST}-ld"
AS="${TOOLCHAIN_PATH}/${TOOLCHAIN_HOST}-as"
AR="${TOOLCHAIN_PATH}/${TOOLCHAIN_HOST}-ar"
RANLIB="${TOOLCHAIN_PATH}/${TOOLCHAIN_HOST}-ranlib"

CONFIG_OPTS+=("CPP=${CPP}")
CONFIG_OPTS+=("CC=${CC}")
CONFIG_OPTS+=("CXX=${CXX}")
CONFIG_OPTS+=("LD=${LD}")
CONFIG_OPTS+=("AS=${AS}")
CONFIG_OPTS+=("AR=${AR}")
CONFIG_OPTS+=("RANLIB=${RANLIB}")

if [ -z "$*" ]; then

if [ ! -e libzmq ]; then
    git clone --depth 1 https://github.com/zeromq/libzmq.git
fi
pushd libzmq
(
    ./autogen.sh &&
    ./configure "${CONFIG_OPTS[@]}" &&
    make -j4
    make install
) || exit 1
popd

if [ ! -e czmq ]; then
    git clone --depth 1 https://github.com/zeromq/czmq.git
fi
pushd czmq
(
    ./autogen.sh &&
    ./configure "${CONFIG_OPTS[@]}" &&
    make -j4
    make install
) || exit 1
popd

if [ ! -e zyre ]; then
    git clone --depth 1 https://github.com/zeromq/zyre.git
fi
pushd zyre
(
    ./autogen.sh &&
    ./configure "${CONFIG_OPTS[@]}" &&
    make -j4
    make install
) || exit 1
popd

fi

if [ ! -e zlogger ]; then
    git clone --depth 1 https://zenon.cs.hs-rm.de/causality-logger/zlogger.git
fi
pushd zlogger
(
    ./autogen.sh &&
    ./configure "${CONFIG_OPTS[@]}" &&
    make -j4
    make install
) || exit 1
popd

# Sync with PIs
for i in {01..20};
do
    rsync -avz tmp/ pi@pi${i}:/usr/local &
    rsync -avz ./zlogger/1337-logger.conf pi@pi${i}:/etc/rsyslog.d &
done

wait
