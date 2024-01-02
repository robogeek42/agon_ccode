# imgload

Both the C version (src/imgload.c) and the basic version (imgload.bas) demonstrate

loading an image using buffers - consolidated from 1k blocks.

The images were created in GIMP.

Process:

- Load an image into GIMP - get the aspect ration how you want it
- Reduce the colours with the dither option:
 - Menu: Colours->Dither  
 - Set Red, Green and Blue levels to 4 and alpha to 256
 - select your dither method (I used Floyd-Stenberg)
 - mode = replace, opacity=100%
 - OK
- Now reduce the size.  (Menu: Image->Scale Image)
- Finally you can export as "Raw Image Data", Standard (R,G,B), Palette (R,G,B normal)
 
