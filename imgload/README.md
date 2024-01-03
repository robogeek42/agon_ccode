# imgload

Both the C version (src/imgload.c) and the basic version (imgload.bas) demonstrate

loading an image using buffers - consolidated from 1k blocks.

The images were created in GIMP.

Process:

- Load an image into GIMP - get the aspect ration how you want it
- Reduce the size first (I think the results are better).  (Menu: Image->Scale Image)
- Now reduce the colours with the dither option:
 - Menu: Colours->Dither  
 - Set Red, Green and Blue levels to 4 and alpha to 256
 - select your dither method (I used Floyd-Stenberg)
 - mode = replace, opacity=100%
 - OK
- Finally you can export as "Raw Image Data", Standard (R,G,B), Palette (R,G,B normal)

You now have RGB8 data. 

If you want to convert this to the RGBA2 format, I have provided a utility to do this
called rgb8torgb2 (see rgb8torgb2.c)

This can be run as 

```
rgb8torgb2 [RGB|RGBA] <fInile> <fOutile> [REV]
```

You tell it if your input is 3 byte or 4 ( RGB or RGBA)

Then indicate whether you want the RGB colours reversed.


# Examples

``` 
parrot.rgb2 188x240
osaka.rgb2 320x240
agon_fast.rgb2 320x188
```

