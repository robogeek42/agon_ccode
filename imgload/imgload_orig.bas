1 REM Commands as copied from gallery.bin run in debug mode for parrot.rgb2 (188x240)
2 REM note 160=&A0, 250=&FA so 0,250, = &FA00;
3 REM VDU 23,0,160,0,250,2 : REM adv bufffer cmd 2 = Clear
4 REM VDU 23,0,160,0,250,0,64,176 : REM adv buffer cmd 0 Write Block. Size=&B040=45120
5 :

10 MODE 8
20 VDU 23,0,192,0,23,1,0 : REM set logical draw and turn off cursor
25 REM Uncomment for hardcoded parrot load (from VDU ebug output)
30 REM F$="parrot.rgb2" : W%=188 : H%=240 : GOTO 2000
35 REM F$="osaka.rgb2" : W%=320 : H%=240 : GOTO 3000
40 PRINT "Enter Filename (RGBA2222 format) "; : INPUT F$
50 PRINT "Width "; : INPUT W%
60 PRINT "Height "; : INPUT H%
80 :
120 FHAN%=OPENIN(F$)
130 IF FHAN% = 0 THEN PRINT "failed to open "+F$ : GOTO 1000
140 FLEN%=EXT#FHAN% 
150 IF FLEN%<>W%*H% THEN PRINT "wrong size, expected "+STR$(W%*H%) : GOTO 1000
155 :
160 SIZE%=W%*H% : NUMBM%=1 : ROWS%=H% : BLOCK%=SIZE%
170 IF SIZE% <= 65536 THEN GOTO 200
175 :
180 PRINT "Image too large: Splitting" : NUMBM%=2 : REM hack
190 ROWS%=65536/W% : BLOCK%=W%*ROWS%
195 :
200 REM Loading of data 
210 FOR BM%=0 TO NUMBM%-1 
215 BUFID%=&FA00 + BM%
220 PRINT "Load bitmap "+STR$(BM%)+" bufId "+STR$(BUFID%)+" "+STR$(ROWS%)+" rows "+STR$(BLOCK%)+" bytes ...";
225 :
230 VDU 23,0,&A0,BUFID%;2 : REM adv bufffer cmd 2 = Clear
240 VDU 23,0,&A0,BUFID%;0,BLOCK%; : REM adv buffer cmd 0 Write Block.
245 :
250 REM Stream data
260 FOR I%=0 TO BLOCK%-1 : VDU BGET#FHAN% : NEXT I%
270 PRINT " done."
275 :
300 REM create bitmap from buffer
305 PRINT "Create bitmap "+STR$(BM%)+" W "+STR$(W%)+" H "+STR$(ROWS%)
310 VDU 23,27,0,BM% : REM select bitmap (buffer &FA00+BM%)
320 VDU 23,27,&21,W%;ROWS%;1 : REM create bitmap from buffer format 1
325 :
330 BLOCK%=SIZE%-BLOCK%
340 ROWS%=H%-ROWS%
350 NEXT BM%
360 CLOSE#FHAN%
365 :
400 PRINT "Press Return to draw";
410 INPUT A%
415 :
420 ROW%=0 : COL%=(320-W%)/2
430 FOR BM%=0 TO NUMBM%-1 
440 VDU 23,27,0,BM% : REM select bitmap (buffer &FA00+BM%)
450 IF BM%>0 THEN ROW%=65536/W%
455 REM PRINT "Draw bitmap "+STR$(BM%)" at "+STR$(COL%)+","+STR$(ROW%)
460 VDU 23,27,3,COL%;ROW%; : REM draw current bitmap
470 NEXT BM%
475 :
500 REM wait for a key
510 REPEAT UNTIL INKEY(0)=-1 : REM Clear key buffer
520 REPEAT : key=INKEY(10) : UNTIL key <> -1 
530 :
600 GOTO 1000

900 :
1000 VDU 23,0,192,1,23,1,1 : REM restore cursor and logical drawing
1010 END

1900 :
2000 REM hardcoded using data from DEBUG=1 of C code
2010 FHAN%=OPENIN(F$)
2020 VDU 23,0,160,0,250,2 : REM adv bufffer cmd 2 = Clear
2030 VDU 23,0,160,0,250,0,64,176 : REM adv buffer cmd 0 Write Block. Size=&B040=45120
2040 FOR I%=0 TO 45120-1 : VDU BGET#FHAN% : NEXT 
2050 CLOSE#FHAN%
2060 VDU 23,27,0,0
2070 VDU 23,27,33,188,0,240,0,1
2080 VDU 23,27,0,0
2090 VDU 23,27,3,66,0,0,0
2100 END

2900 :
3000 REM hardcoded using data from DEBUG=1 of C code
3010 FHAN%=OPENIN(F$)
3020 VDU 23,0,160,0,250,2 : REM adv bufffer cmd 2 = Clear
3030 VDU 23,0,160,0,250,0,0,255 : REM adv buffer cmd 0 Write Block. Size=&B040=45120
3040 FOR I%=0 TO 65280-1 : VDU BGET#FHAN% : NEXT 
3050 VDU 23,27,0,0
3060 VDU 23,27,33,64,1,204,0,1

3070 VDU 23,0,160,1,250,2 : REM adv bufffer cmd 2 = Clear
3080 VDU 23,0,160,1,250,0,0,45 : REM adv buffer cmd 0 Write Block. Size=&B040=45120
3090 FOR I%=0 TO 11520-1 : VDU BGET#FHAN% : NEXT 
3100 VDU 23,27,0,1
3110 VDU 23,27,33,64,1,36,0,1
3120 CLOSE#FHAN%

3130 VDU 23,27,0,0
3140 VDU 23,27,3,0,0,0,0
3150 VDU 23,27,0,1
3160 VDU 23,27,3,0,0,204,0

3170 END

