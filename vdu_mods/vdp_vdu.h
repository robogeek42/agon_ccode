#ifndef _VDP_VDU_H
#define _VDP_VDU_H

#include <stdbool.h>
#include <stdint.h>
#include <mos_api.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VDP_PUTS(S) mos_puts( (char *)&(S), sizeof(S), 0)

// ========= VDU Commands ==========
volatile SYSVAR *vdp_vdu_init( void );
// VDU 1: Send next character to "printer" (if "printer" is enabled)
void vdp_send_to_printer( char ch );
// VDU 2: Enable "printer"
void vdp_enable_printer( void );
// VDU 3: Disable "printer"
void vdp_disable_printer( void );
// VDU 4: Write text at text cursor
void vdp_write_at_text_cursor( void );
// VDU 5: Write text at graphics cursor
void vdp_write_at_graphics_cursor( void );
// VDU 6: Enable screen (opposite of VDU 21)
void vdp_enable_screen( void );
// VDU 7: Make a short beep (BEL)
void vdp_bell( void );
// VDU 8: Move cursor back one character
void vdp_cursor_left( void );
// VDU 9: Move cursor forward one character
void vdp_cursor_right( void );
// VDU 10: Move cursor down one line
void vdp_cursor_down( void );
// VDU 11: Move cursor up one line
void vdp_cursor_up( void );
// VDU 12: Clear text area (CLS)
void vdp_clear_screen( void );
#define vdp_cls() vdp_clear_screen()
// VDU 13: Carriage return
void vdp_carriage_return( void );
// VDU 14: Page mode On
void vdp_page_mode_on( void );
// VDU 15: Page mode Off
void vdp_page_mode_off( void );
// VDU 16: Clear graphics area (CLG)
void vdp_clear_graphics( void );
#define vdp_clg() vdp_clear_graphics()
// VDU 17, colour: Set text colour
void vdp_set_text_colour( int colour );
// VDU 18, mode, colour: Set graphics colour (GCOL mode, colour)
void vdp_set_graphics_colour( int mode, int colour );
#define vdp_gcol( M, C ) vdp_set_graphics_colour( M, C )
// VDU 19, l, p, r, g, b: Define logical colour
void vdp_define_colour(int logical, int physical, int red, int green, int blue );
// VDU 20: Reset palette and text/graphics colours and drawing modes
void vdp_reset_graphics( void );
// VDU 21: Disable screen
void vdp_disable_screen( void );
// VDU 22, n: Select screen mode (MODE n)
int vdp_mode( int mode );

// VDU 23, n: Re-program display character
void vdp_redefine_character( int chnum, uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6, uint8_t b7 );
// VDU 23, 0, <command>, [<arguments>]: System commands
//   see below
// VDU 23, 1, n: Cursor control
void vdp_cursor_enable( bool flag );
// VDU 23, 6, n1, n2, n3, n4, n5, n6, n7, n8: Set dotted line pattern
void vdp_set_dotted_line_pattern( uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6, uint8_t b7 );
// VDU 23, 7, extent, direction, movement: Scroll
void vdp_scroll_screen(int direction, int speed);
void vdp_scroll_screen_extent( int extent, int direction, int speed );
// VDU 23, 16, setting, mask: Define cursor movement behaviour
void vdp_cursor_behaviour( int setting, int mask );
// VDU 23, 23, n: Set line thickness
void vdp_set_line_thickness( int pixels );
// VDU 23, 27, <command>, [<arguments>]: Bitmap and sprite commands
// see below

// VDU 24, left; bottom; right; top;: Set graphics viewport
void vdp_set_graphics_viewport( int left, int bottom, int right, int top );
//VDU 25, mode, x; y;: PLOT command
void vdp_plot( int plot_mode, int x, int y );
void vdp_move_to( int x, int y );
void vdp_line_to( int x, int y );
void vdp_point( int x, int y );
void vdp_triangle( int x, int y );
void vdp_circle_radius( int x, int y );
void vdp_circle( int x, int y );
void vdp_filled_rect( int x, int y );

// VDU 26: Reset graphics and text viewports
void vdp_reset_viewports( void );
// VDU 27, char: Output character to screen
// -- not implemented -- 
// VDU 28, left, bottom, right, top: Set text viewport
void vdp_set_text_viewport( int left, int bottom, int right, int top );
// VDU 29, x; y;: Set graphics origin
void vdp_graphics_origin( int x, int y );
// VDU 30: Home cursor
void vdp_cursor_home( void );
// VDU 31, x, y: Move text cursor to x, y text position
void vdp_cursor_tab( int row, int col );

// ========= System Commands ==========
// VDU 23, 0, <command>, [<arguments>]: System commands
// Many of these are not yet implemented here

// VDU 23, 0, &0A, n: Set cursor start line and appearance
// VDU 23, 0, &0B, n: Set cursor end line
// VDU 23, 0, &81, n: Set the keyboard locale
// VDU 23, 0, &82: Request text cursor position
// VDU 23, 0, &83, x; y;: Get ASCII code of character at character position x, y
// VDU 23, 0, &84, x; y;: Get colour of pixel at pixel position x, y
// VDU 23, 0, &85, channel, command, <args>: Audio commands
//   see below
// VDU 23, 0, &86: Fetch the screen dimensions
void vdp_get_scr_dims( bool );
// VDU 23, 0, &87: RTC control
// VDU 23, 0, &88, delay; rate; led: Keyboard Control
// VDU 23, 0, &89, command, [<args>]: Mouse control
// VDU 23, 0, &8A, n: Set the cursor start column
// VDU 23, 0, &8B, n: Set the cursor end column
// VDU 23, 0, &8C, x; y;: Relative cursor movement (by pixels)
// VDU 23, 0, &90, n, b1, b2, b3, b4, b5, b6, b7, b8: Redefine character n (0-255)
// VDU 23, 0, &91: Reset all system font characters to original definition
// VDU 23, 0, &93, x; y;: Get ASCII code of character at graphics position x, y
// VDU 23, 0, &94, n: Read colour palette entry n (returns a pixel colour data packet)
// VDU 23, 0, &95, <command>, [<args>]: Font management commands
// VDU 23, 0, &98, n: Turn control keys on and off
// VDU 23, 0, &9C: Set the text viewport using graphics coordinates
// VDU 23, 0, &9D: Set the graphics viewport using graphics coordinates
// VDU 23, 0, &9E: Set the graphics origin using graphics coordinates
// VDU 23, 0, &9F: Move the graphics origin and viewports
// VDU 23, 0, &A0, <bufferId>, <command>, [<args>]
//   see below
// VDU 23, 0, &C0, n: Turn logical screen scaling on and off
void vdp_logical_scr_dims( bool flag );
// VDU 23, 0, &C1, n: Switch legacy modes on or off
// VDU 23, 0, &C3: Swap the screen buffer and/or wait for VSYNC
void vdp_swap( void );
// VDU 23, 0, &C8, <command>, [<args>]: Context management API 
// VDU 23, 0, &CA: Flush current drawing commands
// VDU 23, 0, &F2, n: Set dot-dash pattern length §


// ========= Bitmaps and Sprites ==========
// VDU 23, 27, 0, n: Select bitmap n
void vdp_select_bitmap( int n );
// VDU 23, 27, 1, w; h; b1, b2 ... bn: Load colour bitmap data into current bitmap
void vdp_load_bitmap( int width, int height, uint32_t *data );
// helper function to load bitmap from file
int vdp_load_bitmap_file( const char *fname, int width, int height );
// VDU 23, 27, 1, n, 0, 0;: Capture screen data into bitmap n
// -- not implemented --
// VDU 23, 27, 2, w; h; col1; col2;: Create a solid colour rectangular bitmap
void vdp_solid_bitmap( int width, int height, int r, int g, int b, int a );
// VDU 23, 27, 3, x; y;: Draw current bitmap on screen at pixel position x, y
void vdp_draw_bitmap( int x, int y );
// VDU 23, 27, &20, bufferId;: Select bitmap using a 16-bit buffer ID
void vdp_adv_select_bitmap(int bufferId);
// VDU 23, 27, &21, w; h; format: Create bitmap from selected buffer
void vdp_adv_bitmap_from_buffer(int width, int height, int format);
// VDU 23, 27, &21, bitmapId; 0; Capture screen data into bitmap using a 16-bit buffer ID
// -- not implemented --

// VDU 23, 27, 4, n: Select sprite n
void vdp_select_sprite( int n );
// VDU 23, 27, 5: Clear frames in current sprite
void vdp_clear_sprite( void );
// VDU 23, 27, 6, n: Add bitmap n as a frame to current sprite (where bitmap's buffer ID is 64000+n)
void vdp_add_sprite_bitmap( int n );
// VDU 23, 27, 7, n: Activate n sprites
void vdp_activate_sprites( int n );
// VDU 23, 27, 8: Select next frame of current sprite
void vdp_next_sprite_frame( void );
// VDU 23, 27, 9: Select previous frame of current sprite
void vdp_prev_sprite_frame( void );
// VDU 23, 27, 10, n: Select the nth frame of current sprite
void vdp_nth_sprite_frame( int n );
// VDU 23, 27, 11: Show current sprite
void vdp_show_sprite( void );
// VDU 23, 27, 12: Hide current sprite
void vdp_hide_sprite( void );
// VDU 23, 27, 13, x; y;: Move current sprite to pixel position x, y
void vdp_move_sprite_to( int x, int y );
// VDU 23, 27, 14, x; y;: Move current sprite by x, y pixels
void vdp_move_sprite_by( int x, int y );
// VDU 23, 27, 15: Update the sprites in the GPU
void vdp_refresh_sprites( void );
// VDU 23, 27, 16: Reset bitmaps and sprites and clear all data
void vdp_reset_sprites( void );
// VDU 23, 27, 17: Reset sprites (only) and clear all data
// VDU 23, 27, 18, n: Set the current sprite GCOL paint mode to n **
// VDU 23, 27, &26, n;: Add bitmap n as a frame to current sprite using a 16-bit buffer ID

// Helper function to load bitmaps from file and assign to a sprite
int vdp_load_sprite_bitmaps( const char *fname_prefix, const char *fname_format,
							int width, int height, int num, int bitmap_num );
// Helper function to assign sequence of bitmaps to a sprite (activates all sprites to n)
void vdp_create_sprite( int sprite, int bitmap_num, int frames );

// VDU 23, 27, &40, hotX, hotY: Setup a mouse cursor


// ========= Advanced Buffer Commands =========
void vdp_adv_write_block(int bufferID, int length);
void vdp_adv_clear_buffer(int bufferID);
void vdp_adv_create(int bufferID, int length);
void vdp_adv_stream(int bufferID);
void vdp_adv_adjust(int bufferID, int operation, int offset);
void vdp_adv_consolidate(int bufferID);

int vdp_adv_load_sprite_bitmaps( const char *fname_prefix, const char *fname_format, int width, int height, int num, int bitmap_num );
void vdp_adv_add_sprite_bitmap( int b );
void vdp_adv_create_sprite( int sprite, int bitmap_num, int frames );

// ========= Audio Commands =========
void vdp_audio_play_note( int channel, int volume, int frequency, int duration);
void vdp_audio_status( int channel );
void vdp_audio_set_volume( int channel, int volume );
void vdp_audio_set_frequency( int channel, int frequency );
#define VDP_AUDIO_WAVEFORM_SQUARE 0
#define VDP_AUDIO_WAVEFORM_TRIANGLE 1
#define VDP_AUDIO_WAVEFORM_SAWTOOTH 2
#define VDP_AUDIO_WAVEFORM_SINEWAVE 3
#define VDP_AUDIO_WAVEFORM_NOISE 4
#define VDP_AUDIO_WAVEFORM_VICNOISE 5
void vdp_audio_set_waveform( int channel, int waveform );
void vdp_audio_set_sample( int channel, int bufferID );
void vdp_audio_load_sample( int sample, int length, uint8_t *data);
void vdp_audio_clear_sample( int sample );
#define VDP_AUDIO_SAMPLE_FORMAT_8BIT_SIGNED 0
#define VDP_AUDIO_SAMPLE_FORMAT_8BIT_UNSIGNED 1
#define VDP_AUDIO_SAMPLE_FORMAT_SAMPLE_RATE_FOLLOWS 8
#define VDP_AUDIO_SAMPLE_FORMAT_SAMPLE_TUNEABLE 8
void vdp_audio_create_sample_from_buffer( int channel, int bufferID, int format);
void vdp_audio_set_sample_frequency( int sample, int frequency );
void vdp_audio_set_buffer_frequency( int channel, int bufferID, int frequency );
void vdp_audio_set_sample_repeat_start( int sample, int repeatStart );
void vdp_audio_set_buffer_repeat_start( int channel, int bufferID, int repeatStart );
void vdp_audio_set_sample_repeat_length( int sample, int repeatLength );
void vdp_audio_set_buffer_repeat_length( int channel, int bufferID, int repeatLength );

void vdp_audio_volume_envelope_disable( int channel );
void vdp_audio_volume_envelope_ADSR( int channel, int attack, int decay, int sustain, int release );
void vdp_audio_volume_envelope_multiphase_ADSR( int channel ); // variable length parameters to be send separately
void vdp_audio_frequency_envelope_disable( int channel );
#define VDP_AUDIO_FREQ_ENVELOPE_CONTROL_REPEATS 1
#define VDP_AUDIO_FREQ_ENVELOPE_CONTROL_CUMULATIVE 2
#define VDP_AUDIO_FREQ_ENVELOPE_CONTROL_RESTRICT 4
void vdp_audio_frequency_envelope_stepped( int channel, int phaseCount, int controlByte, int stepLength );

void vdp_audio_enable_channel( int channel );
void vdp_audio_disable_channel( int channel );
void vdp_audio_reset_channel( int channel );

void vdp_audio_sample_seek( int channel, int position );
void vdp_audio_sample_duration( int channel, int duration );
void vdp_audio_sample_rate( int channel, int rate );
void vdp_audio_set_waveform_parameter( int channel, int parameter, int value );

#ifdef __cplusplus
}
#endif

#endif
