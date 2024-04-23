10 REM Code to test a 16-bit add operation using buffer adjust
20 CLS
100 REM Create a buffer (1) to hold draw commands
110 VDU 23,0,&A0, 1; 2      :REM clear buf 1
120 VDU 23,0,&A0, 1; 0, 12; :REM next 12 bytes for 2 draw commands
130 VDU 25, 4, 100; 100;    :REM MOVE 100,100
140 VDU 25, &61, 20; 20;    :REM rect-fill +20,+20
150 :
200 REM Create a buffer (2) to hold the command queue
210 VDU 23,0,&A0, 2; 2      :REM clear buf 2
220 VDU 23,0,&A0, 2; 0, 25; :REM capture following commands - 25bytes
230 VDU 23,0,&A0, 1; 1      :REM call buf 1 (draw)
240 VDU 23,0,&A0, 1; 5, &C4, 2; 2; 200; :REM 16bit add-with-carry
250 VDU 23,0,&A0, 1; 1      :REM call buf 1 (draw) again
280 :
300 REM call the command buffer
320 VDU 23,0,&A0, 2; 1      :REM call command buffer to exec draw
