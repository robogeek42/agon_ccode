/*
  vim:ts=4
  vim:sw=4
*/
#include "agon/vdp_vdu.h"
#include "agon/vdp_key.h"
#include <mos_api.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "util.h"
#include "keydefines.h"

int sample_len = 0;

void wait() { char k=getchar(); if (k=='q') exit(0); }

void game_loop();

// returns length of sound sample
int load_sound_sample(char *fname, int sample_id)
{
	unsigned int file_len = 0;
	FILE *fp;
	fp = fopen(fname, "rb");
	if ( !fp )
	{
		printf("Fail to Open %s\n",fname);
		return 0;
	}
	// get length of file
	fseek(fp, 0, SEEK_END);
	file_len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	uint8_t *data = (uint8_t*) malloc(file_len);
	if ( !data )
	{
		fclose(fp);
		printf("Mem alloc error\n");
		return 0;
	}
	unsigned int bytes_read = fread( data, 1, file_len, fp );
	if ( bytes_read != file_len )
	{
		fclose(fp);
		printf("Err read %d bytes expected %d\n", bytes_read, file_len);
		return 0;
	}

	fclose(fp);

	vdp_audio_load_sample(sample_id, file_len, data);

	return file_len;
}

int main(int argc, char *argv[])
{
	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;
	vdp_set_key_event_handler( key_event_handler );

	vdp_mode(0);

	char fname[255];
	strcpy(fname, "steps.raw");

	if (argc>1) 
	{
		strcpy(fname, argv[1]);
	}

	COL(1); TAB(0,1); printf("SOUND TEST2 - FAST REACT");
	COL(2); TAB(0,2); printf("========================");

	COL(14); TAB(0,4); printf("Loading %s ... ", fname);
	COL(15);
	// load sound sample. sample id should be (-1 to -128).
	// Sample -1 is stored in buffer 64256. (-2 in 64257 etc)
	sample_len = load_sound_sample(fname, -1);
	if (sample_len == 0)
	{
		printf("Error\n");
		return 0;
	}
	COL(14); printf(" %d bytes loaded.", sample_len);

	// set channel 1 to use sample -1 
	// This maps to abs(-1)+64255
	vdp_audio_set_sample( 1, 64256 );

	vdp_cursor_enable(false);
	game_loop();

	vdp_cursor_enable(true);
	return 0;
}

void game_loop()
{
	int exit=0;
	int key_delay = 8;
	clock_t key_delay_ticks = clock();
	bool bSamplePlaying = false;
	bool bSampleOn = false;

	COL(14); TAB(0,6); printf("Playing: "); 
	COL(14); TAB(16,6); printf("Sound: "); 
	COL(13); TAB(10,6); printf("%s",bSamplePlaying?"Yes":"No ");
	COL(13); TAB(22,6); printf("%s",bSampleOn?"On ":"Off");
	COL(15); TAB(0,8); printf("Press P to start, K to kill\n");
	COL(15); TAB(0,9); printf("      Hold space to hear\n");

	do {
		if ( vdp_check_key_press( 0x26 ) || vdp_check_key_press( 0x2D ) ) exit=1; // q or x

		if ( vdp_check_key_press( KEY_p ) )
		{
			if ( key_delay_ticks < clock() && !bSamplePlaying )
			{
				vdp_audio_play_note(1, 100, 435, -1 );
				vdp_audio_set_volume(1, 0);
				bSamplePlaying = true;
				COL(13); TAB(10,6); printf("%s",bSamplePlaying?"Yes":"No ");
				key_delay_ticks = clock() + key_delay;
			}
		}
		if ( vdp_check_key_press( KEY_k ) )
		{
			if ( key_delay_ticks < clock() && bSamplePlaying )
			{
				vdp_audio_reset_channel(1);
				bSamplePlaying = false;
				COL(13); TAB(10,6); printf("%s",bSamplePlaying?"Yes":"No ");
				key_delay_ticks = clock() + key_delay;
			}
		}
		if ( vdp_check_key_press( KEY_space ) )
		{
			if ( bSamplePlaying && !bSampleOn )
			{
				vdp_audio_sample_seek(1, 0);
				vdp_audio_set_volume(1, 100);
				bSampleOn = true;
				COL(13); TAB(22,6); printf("%s",bSampleOn?"On ":"Off");
			}
		} else {
			if ( bSamplePlaying && bSampleOn )
			{
				vdp_audio_set_volume(1, 0);
				bSampleOn = false;
				COL(13); TAB(22,6); printf("%s",bSampleOn?"On ":"Off");
			}
		}
		vdp_update_key_state();
	} while (exit==0);

	COL(15); TAB(0,12);
}


