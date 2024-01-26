# agon_ccode
C code for Agon using AgDev

Instructions 
- get CEdev toolchain and unzip it
- https://github.com/CE-Programming/toolchain
- get the AgDev repo and copy into the CEdev directory
- https://github.com/pcawte/AgDev
- under CEdev clone this repo
  
  ```
  cd <CEdev>/agon_ccode/gallery
  make
  cp -rf bin/*bin img <your agon sdcard>
  ```

I also include some additions to vdp_vdu.c/h which are convenience functions
for some more of the VDP commands.

To build a new version of CEdev/AgDev with these modifications,
see the scripts agdev_make.sh and agdev_install.sh


