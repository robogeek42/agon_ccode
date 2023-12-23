#include "agon/vdp_vdu.h"
#include "agon/vdp_key.h"
#include <stdio.h>
#include <mos_api.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define LOAD_BMAP_BLOCK 65535
#define DESC_MAX 40

#define DEBUG 0

void debug_vdu(unsigned char *vdu, int len);

#if DEBUG==1
//#define MY_VDP_PUTS(S) debug_vdu((unsigned char *)&(S), sizeof(S)); mos_puts( (char *)&(S), sizeof(S), 0)
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
	char desc[DESC_MAX];
	char fname[DESC_MAX];
	int width;
	int height;
	char type[10];
} FINFO;
FINFO *images[20];
int num_images=0;

char infofilename[]="img/gallery.info";
FILE *infofile;

FILE *open_file( const char *fname, const char *mode);
int close_file( FILE *fp );
int parse_info(FILE *fp, FINFO **images);
int read_str(FILE *fp, char *str, char stop);
int load_bitmap_file( const char *fname, int width, int height );

int main()
{
	bool bExit=false;

	vdp_vdu_init();
	if ( vdp_key_init() == -1 ) return 1;

	int iMode = 8;

	my_mode.n = iMode;
	VDP_PUTS( my_mode );

	logical_scr_dims(false);

	printf("Gallery - parse %s\r\n", infofilename);

	infofile = open_file(infofilename, "r");
	if (infofile == NULL) return -1;

	num_images=parse_info(infofile, images);

	close_file(infofile);

	do {
		vdp_clear_screen();

		vdp_set_text_colour(45);
		printf("Gallery. Small RGBA2 Image Viewer\r\n--------------------------------\r\n");

		vdp_set_text_colour(2);
		for (int i=0; i<num_images; i++)
		{
			printf(" %2d : %s\r\n",i+1,images[i]->desc);
		}
		vdp_set_text_colour(15);
		printf("\r\nEnter number (q/x to quit):");	
		int num=0;
		char buf[40];
		scanf("%[^\n]s",buf);
		num=atoi(buf);
		if (num>0 && num<=num_images)
		{
			if (images[num-1]->width*images[num-1]->height > 65535)
			{
				printf("Image too large\n");
			}
			else
			{
				if ( load_bitmap_file( images[num-1]->fname, images[num-1]->width, images[num-1]->height ) ) {
					printf( "Error loading bitmap.\n" );
				} else {
					vdp_clear_screen();
					draw_bitmap((320-images[num-1]->width)/2, (240-images[num-1]->height)/2);
					// wait for a key press
					getchar();
				}
			}
		}
		else 
		{
			if (buf[0]=='q' || buf[0]=='Q' || buf[0]=='x' || buf[0]=='X')
			{
				bExit=true;
			}
		}
	} while (!bExit);

	return 0;
}

/*
 * File IO and parsing of config file
 */


// Open file
FILE *open_file( const char *fname, const char *mode )
{

	FILE *fp ;

	if ( !(fp = fopen( fname, mode )) ) {
		printf( "Error opening file %s\r\n", fname);
		return NULL;
	}
	return fp;
}

// Close file
int close_file( FILE *fp )
{
	if ( fclose( fp ) ){
		puts( "Error closing file.\r\n" );
		return EOF;
	}
	return 0;
}

int parse_info(FILE *fp, FINFO **images)
{
	int ret = 0; int record=0;
	char buf[100];
	do {
		void *p = malloc(sizeof(FINFO));
		if (p==NULL) {printf("malloc failed\r\n"); return -1; }

		FINFO *finfop = (FINFO*) p;

		ret = read_str(fp, buf, 44);
		strcpy(finfop->desc, buf);

		ret = read_str(fp, buf, 44);
		strcpy(finfop->fname, buf);

		ret = read_str(fp, buf, 44);
		finfop->width = atoi(buf);

		ret = read_str(fp, buf, 44);
		finfop->height = atoi(buf);

		ret = read_str(fp, buf, 13);
		strcpy(finfop->type, buf);

		fread(buf, 1, 1, fp);

		if (ret >0)
		{
			images[record++]=finfop;
		}

	} while (ret>0);
	printf("Read %d records\r\n",record);
	return record;
}

int read_str(FILE *fp, char *str, char stop) {
	char c;
	int cnt=0, bytes=0;
	do {
		bytes = fread(&c, 1, 1, fp);
		if (bytes != 0 && c != stop) {
			str[cnt++]=c;
		}
	} while (c != stop && bytes != 0);
	str[cnt++]=0;
	return bytes;
}
	
/*
 * My VDP commands 
 */

void debug_vdu(unsigned char *vdu, int len){
	printf ("VDU ");
	for (int i=0;i<len;i++){
		printf("%02X ",vdu[i]);
		//printf("%u,",vdu[i]);
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



int load_bitmap_file( const char *fname, int width, int height )
{
	FILE *fp;
	char *buffer;
	int exit_code = 0;

	if ( !(buffer = (char *)malloc( LOAD_BMAP_BLOCK ) ) ) {
		printf( "Failed to allocated memory for buffer.\n" );
		return -1;
	}
	if ( !(fp = fopen( fname, "rb" ) ) ) {
		printf( "Error opening file \"%sa\". Quitting.\n", fname );
		return -1;
	}

	adv_clear_buffer(0xFA00);
	adv_write_block(0xFA00, width*height);

	int size = width * height;
	if ( size==0 || size > LOAD_BMAP_BLOCK ) return -1;

	if ( fread( buffer, 1, size, fp ) != (size_t)size ) exit_code = -1;
	mos_puts( buffer, size, 0 );

	fclose( fp );
	free( buffer );

	select_bitmap(0);
	adv_bitmap_from_buffer(width, height, 1); // RGBA2

	return exit_code;
}

