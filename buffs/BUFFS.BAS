5  REM Agon Buffer API test by Robogeek 2024
10 REM use buffer-api to draw a grid of rectangles
12 REM this uses 2 nested loops

20 MODE 3

30 PROC_draw_buff
40 PROC_var_buffs
50 PROC_command_buff

70 VDU 23,0,&A0, 2; 1       :REM call command buffer to exec draw

90 END

100 DEF PROC_draw_buff
101 REM Create a buffer (1) to hold draw commands
110 VDU 23,0,&A0, 1; 2      :REM clear buff 1
120 VDU 23,0,&A0, 1; 0, 15; :REM next 15 bytes for draw commands
130 VDU 25, 4, 100; 100;    :REM MOVE 100,100
140 VDU 18, 0, 1            :REM GCOL 0,0
150 VDU 25, &61, 20; 20;    :REM rect-fill +20,+20
190 ENDPROC

300 DEF PROC_var_buffs
301 REM Buffers for 2 variables (X,Y), 3 bytes each

310 REM Buffer 3 : X 
320 VDU 23,0,&A0, 3; 2                   :REM clear buffer 3
330 VDU 23,0,&A0, 3; 3, 3;               :REM create (CMD=3) buffer 3, length 3
340 VDU 23,0,&A0, 3; 5, &C2, 0; 3; 10; 0 :REM Adjust to starting X value (10). 3 bytes

350 REM Buffer 4 : Y
360 VDU 23,0,&A0, 4; 2                   :REM clear buffer 4
370 VDU 23,0,&A0, 4; 3, 3;               :REM create (CMD=3) buffer 4, length 3
380 VDU 23,0,&A0, 4; 5, &C2, 0; 3; 10; 0 :REM Adjust to starting Y value (10). 3 bytes

390 REM Buffer 5 : cnt1
400 VDU 23,0,&A0, 5; 2                   :REM clear buffer 5
410 VDU 23,0,&A0, 5; 3, 1;               :REM create (CMD=3) buffer 5, length 1
420 VDU 23,0,&A0, 5; 5,   2, 0; 16       :REM adj byte to 16

430 REM Buffer 6 : cnt2
440 VDU 23,0,&A0, 6; 2                   :REM clear buffer 6
450 VDU 23,0,&A0, 6; 3, 1;               :REM create (CMD=3) buffer 6, length 1
460 VDU 23,0,&A0, 6; 5,   2, 0; 4        :REM adj byte to 4

490 ENDPROC

500 DEF PROC_command_buff
501 REM Create a buffer (2) to hold the command queue
505 :
510 VDU 23,0,&A0, 2; 2                   :REM clear buff 2
520 VDU 23,0,&A0, 2; 0, 137;             :REM capture following commands - 137 bytes
525 :
530 VDU 23,0,&A0, 1; 5, &E2, 2; 2; 3; 0; :REM Set X in draw buff (1) to X (buff 3)  (15 bytes)
540 VDU 23,0,&A0, 1; 5, &E2, 4; 2; 4; 0; :REM Set Y in draw buff (1) to Y (buff 4)  (15)
550 VDU 23,0,&A0, 1; 1                   :REM call buff 1 (draw)                    (6)
555 :
560 VDU 23,0,&A0, 3; 5, &C4, 0; 2; 30;   :REM add 30 to X                           (13)
570 VDU 23,0,&A0, 5; 5,   3, 0, 0, 255   :REM decrement cnt1                        (10)
580 VDU 23,0,&A0, 1; 5,   3, 8, 0, 1     :REM inc colour                            (10)
590 VDU 23,0,&A0, 2; 8, 0, 5; 0;         :REM cond-jump cmd buff if cnt1!=0         (11) 80
600 :
610 VDU 23,0,&A0, 4; 5, &C4, 0; 2; 30;   :REM add 30 to Y                           (13)
620 VDU 23,0,&A0, 3; 5, &C2, 0; 2; 10;   :REM Reset X back to left                  (13)
630 VDU 23,0,&A0, 6; 5,   3, 0, 0, 255   :REM decrement cnt2                        (10)
640 VDU 23,0,&A0, 5; 5,   2, 0; 16       :REM reset cnt1 to 16                      (10)
650 VDU 23,0,&A0, 2; 8, 0, 6; 0;         :REM cond-jump cmd buff if cnt2!=0         (11) 57+80=137
660 :
690 ENDPROC

