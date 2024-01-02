#include "agon/vdp_vdu.h"
#include "agon/vdp_key.h"
#include <stdio.h>
#include <mos_api.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/timers.h>

#define DESC_MAX 40

#define DEBUG 1

void debug_vdu(unsigned char *vdu, int len);

#if DEBUG==1
//#define MY_VDP_PUTS(S) debug_vdu((char *)&(S), sizeof(S)); mos_puts( (char *)&(S), sizeof(S), 0)
#define MY_VDP_PUTS(S) debug_vdu((unsigned char *)&(S), sizeof(S)); 
#else
#define MY_VDP_PUTS(S) mos_puts( (char *)&(S), sizeof(S), 0)
#endif

// VDU 22, n: Mode n
typedef struct { uint8_t A; uint8_t n; } MY_VDU_mode;
static MY_VDU_mode		my_mode 	= { 22, 0 };

// VDU 23, 27, 0, n : REM Select bitmap n (equating to buffer ID numbered 64000+n)
typedef struct { uint8_t A; uint8_t B; uint8_t C; uint8_t n; } MY_VDU_select_bitmap;
static MY_VDU_select_bitmap	my_select_bitmap = { 23, 27, 0, 0 };
void select_bitmap( int n );

// VDU 23, 27, 3, x; y; : REM Draw current bitmap on screen at pixel position x, y (a valid bitmap must be selected first)
typedef struct { uint8_t A; uint8_t B; uint8_t C; uint16_t x; uint16_t y; } MY_VDU_draw_bitmap;
static MY_VDU_draw_bitmap	my_draw_bitmap 	= { 23, 27, 3, 0, 0 };
void draw_bitmap( int x, int y );

// VDU 23, 0, &C0, n : REM Turn logical screen scaling on and off, where 1=on and 0=off (Requires MOS 1.03 or above)
typedef struct { uint8_t A; uint8_t B; uint8_t C; uint8_t flag; } MY_VDU_logical_scr_dims;
static MY_VDU_logical_scr_dims	my_logical_scr_dims = { 23,  0, 0xC0, 0 }; 
void logical_scr_dims( bool flag );

// VDU 23, 0 &A0, bufferId; 0 : REM write block to buffer
typedef struct { uint8_t A; uint8_t B; uint8_t C; uint16_t bid; uint8_t op; uint16_t length; } MY_VDU_adv_write_block;
static MY_VDU_adv_write_block  	my_adv_write_block = { 23,  0, 0xA0, 0xFA00, 0, 0};
void adv_write_block(int bufferID, int length);

// VDU 23, 0 &A0, bufferId; 2
typedef struct { uint8_t A; uint8_t B; uint8_t C; uint16_t bid; uint8_t op; } MY_VDU_adv_clear_buffer;
static MY_VDU_adv_clear_buffer  	my_adv_clear_buffer = { 23,  0, 0xA0, 0xFA00, 2};
void adv_clear_buffer(int bufferID);

// VDU 23, 27, &21, width; height; format  : REM Create bitmap from buffer
typedef struct { uint8_t A; uint8_t B; uint8_t C; uint16_t width; uint16_t height; uint8_t format; } MY_VDU_adv_bitmap_from_buffer;
static MY_VDU_adv_bitmap_from_buffer	my_adv_bitmap_from_buffer = { 23, 27, 0x21, 0, 0, 1};
void adv_bitmap_from_buffer(int width, int height, int format);

typedef struct {
	int num_bitmaps;
	int width;
	int rows[4];
} IMG_LOAD_INFO;
IMG_LOAD_INFO img_load_info;


int load_bitmap_file( const char *fname, int width, int height, IMG_LOAD_INFO *load_info );

int main(int argc, char* argv[])
{
	char *filename;
	int width, height;
	int num_bitmaps=0;

	if (argc<4) {
		printf("usage:\r\nload loadimg.bin\r\nrun &40000 <file> <width> <height>\r\n");
		printf("\r\nFormat is RGBA2222 so file length = width*height\r\n");
		return -1;
	}
	filename=argv[1];
	width = atoi(argv[2]);
	height = atoi(argv[3]);

	printf("Load file %s %dx%d\n", filename, width, height);
	printf("Press a key\n");
	getchar();

	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;

	int iMode = 8;

	my_mode.n = iMode; VDP_PUTS( my_mode );

	logical_scr_dims(false);


	vdp_clear_screen();

	num_bitmaps = load_bitmap_file(filename, width, height, &img_load_info);
	if (num_bitmaps == 0)
	{
		printf( "Error loading bitmap.\r\n" );
		return -1;
	}
#if DEBUG==0
	//vdp_clear_screen();
#endif
	
	printf("loaded %d bitmaps ... \r\n",num_bitmaps);
	for (int bm=0;bm<num_bitmaps;bm++)
	{
		printf("  Bitmap %d : %d rows\r\n",bm,img_load_info.rows[bm]);
	}
	printf("press a key to display\r\n");
	getchar();

	int row_start = (240-height)/2;
	for (int bm=0;bm<num_bitmaps;bm++)
	{
		select_bitmap(bm);
		draw_bitmap((320-width)/2, row_start);
		row_start+=img_load_info.rows[bm];
	}

	// wait for a key press
	getchar();

	return 0;
}


/*
 * My VDP commands 
 */

void debug_vdu(unsigned char *vdu, int len){
	printf ("VDU ");
	for (int i=0;i<len;i++){
		//printf("%02X ",vdu[i]);
		printf("%u,",vdu[i]);
	}
	printf("\r\n");
}
void select_bitmap( int n )
{
	my_select_bitmap.n = n;
	MY_VDP_PUTS(my_select_bitmap);
}

void logical_scr_dims( bool flag )
{
	my_logical_scr_dims.flag = 0;
	if ( flag ) my_logical_scr_dims.flag = 1;
	MY_VDP_PUTS(my_logical_scr_dims);
}

void draw_bitmap( int x, int y )
{
	my_draw_bitmap.x = x;
	my_draw_bitmap.y = y;
	MY_VDP_PUTS(my_draw_bitmap);
}

void adv_write_block(int bufferID, int length)
{
	my_adv_write_block.bid = bufferID;
	my_adv_write_block.length = length;
	MY_VDP_PUTS(my_adv_write_block);
}

void adv_clear_buffer(int bufferID)
{
	my_adv_clear_buffer.bid = bufferID;
	MY_VDP_PUTS(my_adv_clear_buffer);
}

void adv_bitmap_from_buffer(int width, int height, int format)
{
	my_adv_bitmap_from_buffer.width = width;
	my_adv_bitmap_from_buffer.height = height;
	my_adv_bitmap_from_buffer.format = format; // RGBA2=1
	MY_VDP_PUTS(my_adv_bitmap_from_buffer);
}



int load_bitmap_file( const char *fname, int width, int height, IMG_LOAD_INFO *load_info )
{
	FILE *fp;
	char *buffer;
	int bmap_id = 0;
	int max_rows = 240;
	int max_bytes = 65535;
	int rows_remain = height;
	int bytes_remain = width * height;

	if (width*height < 65536) { 
		max_rows = height;
		max_bytes = width*height;
	} else {
		max_rows = 65536/width;
		max_bytes = max_rows * width;
	}

	printf("WxH %dx%d, max rows %d, max bytes %d\n",width,height, max_rows, max_bytes);
	
	if ( !(buffer = (char *)malloc( max_bytes ) ) ) {
		printf( "Failed to allocate %d bytes for buffer.\n",max_bytes );
		return -1;
	}
	if ( !(fp = fopen( fname, "rb" ) ) ) {
		printf( "Error opening file \"%sa\". Quitting.\n", fname );
		return -1;
	}

	rows_remain = height;
	bytes_remain = width * height;

	load_info->width = width;
	load_info->rows[0]=0;
	load_info->num_bitmaps=0;

	while (bytes_remain > 0)
	{
		int size = (bytes_remain>max_bytes)?max_bytes:bytes_remain;
		int rows = (rows_remain>max_rows)?max_rows:rows_remain;

		printf("Create bitmap ID %d. Size %d. Rows%d.\n",bmap_id, size, rows);
		adv_clear_buffer(0xFA00+bmap_id);
		adv_write_block(0xFA00+bmap_id, size);
		ticksleep(100); // add a delay to try and fix issue on real Agon

		if ( fread( buffer, 1, size, fp ) != (size_t)size ) return 0;
#if DEBUG==0
		mos_puts( buffer, size, 0 );
#else
		printf("write %d bytes, %d rows\n",size,rows);
#endif

		select_bitmap(bmap_id);
		adv_bitmap_from_buffer(width, rows, 1); // RGBA2
		load_info->rows[bmap_id] = rows;

		rows_remain -= rows;
		bytes_remain -= size;
		bmap_id++;
	}
	load_info->num_bitmaps=bmap_id;
	
	fclose( fp );
	free( buffer );

	return bmap_id;
}

