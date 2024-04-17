 10 REM Load a sample into the VDP
 20 REM infile=OPENIN "sfx_step_grass_l.raw"
 22 infile=OPENIN "shoot.raw"
 30 length=EXT#infile
 40 REM Send sample info to the VDP for sample -1
 50 VDU 23, 0, &85, -1, 5, 0, length MOD 256, length DIV 256, length DIV 65536
 60 REM Send sample data to the VDP
 70 REPEAT
 80   VDU BGET#infile
 90 UNTIL EOF#infile
100 CLOSE#infile
110 REM Set channel 1 to use sample -1
120 VDU 23, 0, &85, 1, 4, -1
130 REM Play sample on channel 1
140 SOUND 1, -10, 10, (length/16) / 50
