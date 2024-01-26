#!/bin/bash -x

BASEDIR=~/agon
GITHUB=$BASEDIR/github
CCODE=$BASEDIR/agon_ccode/

# before you start you must have the modified CEdev
# extracted and CEdev/bin in your path
# this is so it can find the ez80-clang compiler
# cd $BASEDIR; tar xf ~/Downloads/CEdev-Linux.tar.gz ; PATH=$PATH:$BASEDIR/CEdev/bin

#----------------------------------------
# get latest AgDev code
# Only do this if you want AgDev updates
#----------------------------------------
#cd $GITHUB
#rm -rf $GITHUB/AgDev
#git clone https://github.com/pcawte/AgDev.git

AGDEV_GIT=$BASEDIR/github/AgDev

#----------------------------------------
# get latest CEdev code
# Only do this if you want toolchain updates
#----------------------------------------
#cd $GITHUB
#rm -rf $GITHUB/CEdev
#git clone https://github.com/CE-Programming/toolchain.git CEdev
#cd CEdev
#git submodule update --init --recursive

CEDEV_GIT=$GITHUB/CEdev

#----------------------------------------
# Copy in AgDev code to CEdev toolchain code
#----------------------------------------
cp -r $AGDEV_GIT/src/* $CEDEV_GIT/src/
cp -r $AGDEV_GIT/include/* $CEDEV_GIT/src/include

#----------------------------------------
# Copy over the customised vdp code to the CEdev github dir
# That directoy was prepared by checking out CEdev and copying over
# source code and libraries from AgDev to it
#----------------------------------------
cp $CCODE/vdu_mods/vdp_vdu.h $CEDEV_GIT/include/agon/
cp $CCODE/vdu_mods/vdp_vdu.h $CEDEV_GIT/src/include/agon/
cp $CCODE/vdu_mods/vdp_vdu.h $CEDEV_GIT/src/libc/include/
cp $CCODE/vdu_mods/vdp_vdu.c $CEDEV_GIT/src/libc/

# Remove the previous build directory and make
cd $CEDEV_GIT
rm -rf CEdev
#make clean
make install

# will result in a install directory $CEDEV_GIT/CEdev/CEdev
# because of the strange way the AgDev common.mk file sets up the
# install directories
