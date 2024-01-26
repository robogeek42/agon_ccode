#!/bin/bash -x

BASEDIR=~/agon
CEDEV_TAR=~/Downloads/CEdev-Linux.tar.gz

# this is where you want your working CEdev directory
# to be - this is where you will build and compile code
CEDEV_FINAL=$BASEDIR/CEdev
# this is where you checkout the Source Code of CEdev
# and you will compile it with the AgDev mods and your own
# customisations.
# See agdev_make.sh for a script to do this build
CEDEV_GIT=$BASEDIR/github/CEdev
# and the Source code of AgDev
AGDEV_GIT=$BASEDIR/github/AgDev

# a directory with my own programs which need to be 
# put in the CEdev working directory to be compiled
CCODE=$BASEDIR/agon_ccode/

# delete previous CEdev final directory
rm -rf $CEDEV_FINAL

# Get the full version of CEdev from the TAR file
cd /tmp ; rm -rf CEdev
tar xf ~/Downloads/CEdev-Linux.tar.gz
mv CEdev $CEDEV_FINAL

# ... but now we can get our modified version which was created with agdev_make.sh
cp -rf $CEDEV_GIT/CEdev/CEdev $CEDEV_FINAL

# Copy over the AgDev stuff - this includes the ez80_clang compiler
# and example code
cp -rf $AGDEV_GIT/* $CEDEV_FINAL/

# now copy over the built binaries and libraries
cp $CEDEV_GIT/CEdev/CEdev/bin/* $CEDEV_FINAL/bin/
cp -r $CEDEV_GIT/CEdev/CEdev/lib/libc/vdp* $CEDEV_FINAL/lib/agon/
# finally copy in the modified headers and code
cp $CCODE/vdu_mods/vdp_vdu.h $CEDEV_FINAL/include/agon/
cp $CCODE/vdu_mods/vdp_vdu.h $CEDEV_FINAL/src/libc/include
cp $CCODE/vdu_mods/vdp_vdu.c $CEDEV_FINAL/src/libc/

# Link in my ccode source tree where you can build stuff
cd $CEDEV_FINAL/ 
ln -s $CCODE cc

# Lets try compiling something
cd $CEDEV_FINAL/cc/scroll
make

