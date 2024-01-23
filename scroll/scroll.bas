10 MODE 8
15 VDU 23,0,192,0,23,1,0 : REM unset logical draw and turn off cursor
20 PROCloadImages
30 PRINT"done"
35 CLS
40 PROCshowImages
50 PROCloadmap

990 VDU 23,0,192,1,23,1,1 : REM restore cursor and logical drawing
999 END

1000 DEF PROCloadImages
1010 FOR I%=0 TO 15
1020 F$ = "img/ts"+RIGHT$( "000"+STR$(I%+1) ,2)+".rgb2"
1040 FHAN%=OPENIN(F$)
1050 IF FHAN% = 0 THEN PRINT "failed to open "+F$ : END
1055 PRINT "Load "+F$
1060 PROCloadBitmapRGB2(FHAN%, I%, 16, 16)
1065 CLOSE#FHAN%
1070 NEXT
1090 ENDPROC

1200 DEF PROCloadBitmapRGB2(FHAN%, BM%, W%, H%)
1210 LOCAL BUFID%, I%
1220 BUFID%=&FA00 + BM%
1230 VDU 23,0,&A0,BUFID%;2 : REM adv bufffer cmd 2 = Clear
1240 VDU 23,0,&A0,BUFID%;0,W%*H%; : REM adv buffer cmd 0 Write Block.
1260 FOR I%=0 TO W%*H%-1 : VDU BGET#FHAN% : NEXT I%
1300 REM create bitmap from buffer
1310 VDU 23,27,0,BM% : REM select bitmap (buffer &FA00+BM%)
1320 VDU 23,27,&21,W%;H%;1 : REM create bitmap from buffer format 1
1330 ENDPROC

1500 DEF PROCshowImages
1505 LOCAL X%,Y%
1507 VDU 5 : REM write text at graphics cursor
1510 FOR I%=0 TO 15
1530 VDU 23,27,0,I%
1540 X% = 20 + (I% MOD 8) * 20
1545 Y% = 40 + (I% DIV 8) * 40
1550 VDU 23,27,3,X%;Y%;
1560 MOVE X%,Y%-10 : PRINT STR$(I%);
1570 NEXT
1580 VDU 4 : REM write text at text cursor
1590 ENDPROC
