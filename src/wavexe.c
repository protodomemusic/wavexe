/*H************************************************************
*  FILENAME:     wavexe.c
*  DESCRIPTION:  Executable Wavetable Synth
*
*  NOTES:        A simple wavetable synth piped directly into
*                a wave file. Sequencing powered by:
*                https://github.com/protodomemusic/mmml
*
*                To generate the required "wavexe-mmml-data.h"
*                sequence data file, use the mmml-compiler.c
*                program and export with the -t wavexe flag.
*
*                TO DO:
*                - Weird 'notch' in waveform when switching
*                  between pitches.
*                - Sort out the weird mixing/balancing to work
*                  better w/ variable voices and ensure no
*                  clipping.
*                - Decays take varying lengths depending on the
*                  starting volume.
*                - Make volumes ('v' commands) more dynamic.
*                - Vibrato.
*                - Portamento.
*
*  AUTHOR:       Blake 'PROTODOME' Troise
*  PLATFORM:     Command Line Application (Windows/OSX/Linux)
*  DATE:         10th February 2021
************************************************************H*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define PLAY_TIME        20     // duration of recording in seconds
#define SAMPLE_RATE      44100  // cd quality audio
#define AMPLITUDE        127    // waveform high position (maximum from DC zero is 127)
#define DC_OFFSET        127    // waveform low position (127 is DC zero)
#define BITS_PER_SAMPLE  8      // 8-bit wave files require 8 bits per sample
#define AUDIO_FORMAT     1      // for PCM data
#define TOTAL_CHANNELS   1      // mono file
#define SUBCHUNK_1_SIZE  16
#define BYTE_RATE        SAMPLE_RATE * TOTAL_CHANNELS * BITS_PER_SAMPLE / 8
#define BLOCK_ALIGN      TOTAL_CHANNELS * BITS_PER_SAMPLE / 8
#define TOTAL_SAMPLES    PLAY_TIME * SAMPLE_RATE
#define SUBCHUNK_2_SIZE  TOTAL_SAMPLES * TOTAL_CHANNELS * BITS_PER_SAMPLE / 8
#define CHUNK_SIZE       4 + (8 + SUBCHUNK_1_SIZE) + (8 + SUBCHUNK_2_SIZE)

// holds the header information
typedef struct wavfile_header_s
{
	int8_t  chunk_id[4];
	int32_t chunk_size;
	int8_t  format[4];
	int8_t  subchunk_1_id[4];
	int32_t subchunk_1_size;
	int16_t audio_format;
	int16_t total_channels;
	int32_t sample_rate;
	int32_t byte_rate;
	int16_t block_align;
	int16_t bits_per_sample;
	int8_t  subchunk_2_id[4];
	int32_t subchunk_2_size;
}
wavfile_header_t;

// where we'll store our final wave data
unsigned char buffer[TOTAL_SAMPLES];

//===================//
// mmml header stuff //
//===================//

#include "wavexe-mmml-data.h"

// functions of convenience
void mmml();
void configure_instrument(unsigned char voice, unsigned char instrument_id);
void update_wavetable(unsigned char voice, unsigned char sample, unsigned char volume, unsigned char gain);

// wavetable
#define TOTAL_VOICES     8
#define TOTAL_NOTES      72
#define TOTAL_WAVECYCLES 7
#define WAVETABLE_SIZE   256
#define MAX_VOLUME       32

// mmml
#define MAXLOOPS         5

const unsigned int notes[TOTAL_NOTES] =
{
//  c    c#   d    d#   e    f    f#   g    g#   a    a#   b	
	131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247,
	262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
	523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988,
	1046,1109,1175,1244,1318,1397,1480,1568,1661,1760,1865,1976,
	2093,2217,2349,2489,2637,2794,2960,3136,3322,3520,3729,3951,
	4186,4435,4699,4978,5274,5588,5920,6272,6645,7040,7459,7902,
};

const signed char wavetable_data[WAVETABLE_SIZE * TOTAL_WAVECYCLES] =
{
	// pure
	0,3,6,9,12,15,18,21,24,28,31,34,37,40,43,46,48,51,54,57,60,63,65,68,71,73,76,78,81,83,85,88,90,92,94,96,98,100,102,104,106,108,109,111,112,114,115,116,118,119,120,121,122,123,124,124,125,126,126,127,127,127,127,127,127,127,127,127,127,127,126,126,125,124,124,123,122,121,120,119,118,117,115,114,112,111,109,108,106,104,102,100,99,97,94,92,90,88,86,83,81,78,76,73,71,68,65,63,60,57,54,52,49,46,43,40,37,34,31,28,25,22,18,15,12,9,6,3,0,-2,-6,-9,-12,-15,-18,-21,-24,-27,-30,-33,-36,-39,-42,-45,-48,-51,-54,-57,-60,-62,-65,-68,-70,-73,-76,-78,-81,-83,-85,-88,-90,-92,-94,-96,-98,-100,-102,-104,-106,-108,-109,-111,-112,-114,-115,-116,-118,-119,-120,-121,-122,-123,-124,-124,-125,-126,-126,-126,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-126,-126,-125,-124,-124,-123,-122,-121,-120,-119,-118,-117,-115,-114,-113,-111,-109,-108,-106,-104,-103,-101,-99,-97,-95,-92,-90,-88,-86,-83,-81,-79,-76,-73,-71,-68,-66,-63,-60,-57,-55,-52,-49,-46,-43,-40,-37,-34,-31,-28,-25,-22,-19,-16,-12,-9,-6,-3,
	// warm
	0,-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-11,-12,-13,-14,-15,-16,-17,-18,-19,-20,-21,-22,-23,-24,-25,-26,-27,-28,-29,-30,-31,-32,-33,-34,-35,-36,-36,-37,-38,-39,-40,-41,-42,-43,-44,-45,-46,-47,-48,-49,-50,-51,-52,-53,-53,-54,-55,-56,-57,-58,-59,-60,-61,-62,-63,-64,-64,-65,-66,-67,-68,-69,-70,-71,-72,-73,-73,-74,-75,-76,-77,-78,-79,-80,-81,-81,-82,-83,-84,-85,-86,-87,-87,-88,-89,-90,-91,-92,-93,-93,-94,-95,-96,-97,-98,-99,-99,-100,-101,-102,-103,-104,-104,-105,-106,-107,-108,-109,-109,-110,-111,-112,-112,-114,-114,-115,-115,-117,-117,-117,-71,4,41,71,91,105,114,120,123,126,127,127,127,127,126,125,124,123,122,121,120,119,117,116,115,114,112,111,110,109,107,106,105,104,102,101,100,99,97,96,95,94,92,91,90,89,88,86,85,84,83,82,80,79,78,77,76,75,73,72,71,70,69,68,66,65,64,63,62,61,59,58,57,56,55,54,53,51,50,49,48,47,46,45,44,42,41,40,39,38,37,36,35,34,32,31,30,29,28,27,26,25,24,23,22,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,1,0,
	// soft
	0,2,6,10,15,21,26,31,35,38,40,43,44,46,49,52,56,61,66,70,73,76,77,77,78,78,79,81,84,87,90,94,96,98,98,97,97,96,97,98,100,103,107,110,113,114,115,115,114,113,113,115,117,119,122,125,127,127,126,126,124,122,120,120,120,122,123,124,125,124,123,121,118,114,111,109,107,107,107,107,106,104,101,97,92,87,83,78,75,72,71,69,67,65,60,55,49,43,37,32,27,24,21,19,16,13,9,3,-2,-9,-16,-21,-26,-30,-33,-36,-39,-43,-47,-52,-59,-65,-72,-78,-83,-87,-89,-91,-94,-96,-99,-102,-106,-111,-115,-119,-122,-123,-124,-123,-122,-122,-121,-121,-122,-122,-123,-123,-122,-120,-117,-113,-108,-104,-100,-97,-94,-91,-89,-85,-81,-77,-72,-67,-61,-56,-51,-47,-43,-39,-36,-33,-30,-26,-23,-19,-15,-12,-10,-8,-6,-5,-5,-5,-5,-5,-4,-4,-4,-5,-6,-7,-9,-11,-14,-16,-19,-21,-24,-26,-29,-32,-35,-38,-42,-45,-48,-52,-55,-57,-60,-62,-64,-66,-68,-70,-72,-74,-76,-77,-79,-80,-81,-81,-81,-81,-81,-80,-80,-80,-80,-79,-79,-77,-75,-73,-70,-67,-63,-60,-57,-54,-52,-49,-47,-44,-40,-35,-30,-25,-20,-15,-11,-8,-4,-2,
	// dull
	0,-18,-37,-56,-74,-90,-104,-114,-121,-125,-124,-119,-110,-98,-82,-63,-42,-19,4,28,52,73,92,107,118,125,126,122,112,97,78,55,30,4,-21,-46,-70,-90,-106,-118,-124,-124,-120,-112,-99,-83,-64,-43,-21,1,23,45,64,82,97,109,118,124,126,125,121,114,104,92,78,62,46,29,12,-3,-19,-34,-49,-63,-75,-87,-96,-104,-111,-116,-120,-123,-124,-125,-124,-123,-121,-118,-114,-111,-106,-102,-97,-92,-87,-82,-78,-73,-68,-64,-59,-55,-51,-48,-44,-41,-38,-36,-34,-32,-31,-30,-29,-28,-28,-29,-29,-30,-32,-33,-35,-38,-40,-44,-47,-51,-55,-59,-63,-68,-73,-78,-82,-88,-93,-97,-102,-107,-111,-114,-118,-120,-122,-123,-124,-123,-121,-119,-115,-109,-103,-96,-87,-77,-66,-55,-42,-29,-16,2,15,27,39,51,62,73,83,91,99,106,112,117,121,124,126,127,127,127,126,124,121,118,115,111,107,102,98,93,88,83,78,72,67,63,58,53,49,44,40,36,33,29,26,23,20,18,16,14,13,11,11,10,10,10,10,11,12,13,15,17,19,22,25,29,32,37,41,46,51,56,62,68,74,80,86,92,98,103,109,114,118,122,124,126,126,125,123,119,112,104,94,82,68,53,36,18,
	// sharp
	0,4,10,18,29,43,58,75,92,106,117,124,127,127,127,127,127,126,123,118,113,107,102,99,98,99,102,107,113,119,125,127,126,121,113,102,91,81,74,70,68,68,68,67,64,56,44,24,-4,-37,-66,-90,-106,-117,-123,-126,-127,-127,-127,-127,-126,-124,-120,-114,-106,-97,-90,-88,-92,-101,-112,-122,-127,-125,-116,-102,-86,-70,-57,-46,-39,-34,-32,-33,-37,-43,-51,-60,-71,-81,-90,-97,-101,-103,-101,-96,-88,-77,-64,-51,-37,-24,-13,-3,5,12,17,20,22,21,20,17,14,10,7,5,3,3,4,6,9,14,19,25,31,37,43,48,52,54,56,57,56,56,55,55,56,58,61,67,74,82,92,103,113,121,126,127,126,123,121,120,122,125,127,127,125,121,116,111,107,106,107,111,117,124,127,124,104,61,-10,-72,-104,-120,-125,-127,-127,-127,-127,-124,-117,-105,-85,-59,-29,-1,15,19,10,-12,-44,-76,-102,-119,-127,-127,-122,-114,-107,-100,-95,-92,-90,-90,-91,-93,-94,-96,-96,-96,-94,-91,-86,-81,-74,-67,-60,-54,-49,-44,-41,-40,-40,-40,-42,-45,-48,-50,-52,-53,-52,-49,-44,-37,-29,-19,-10,0,11,22,32,41,48,53,56,55,52,46,39,31,23,15,9,3,0,0,
	// hollow
	0,18,32,40,41,36,25,7,-16,-44,-73,-100,-119,-127,-123,-108,-85,-57,-29,-4,16,30,39,42,39,31,17,-1,-25,-50,-76,-99,-116,-126,-127,-121,-111,-98,-86,-77,-72,-73,-79,-90,-103,-116,-125,-127,-120,-102,-75,-40,9,43,69,91,104,113,117,119,119,116,109,99,83,61,34,3,-28,-58,-84,-103,-116,-124,-127,-127,-127,-127,-127,-127,-124,-117,-103,-81,-51,-16,21,57,88,110,123,127,126,120,115,111,109,111,116,122,126,127,123,111,91,66,37,7,-20,-43,-62,-75,-83,-86,-84,-78,-67,-49,-26,2,34,66,95,116,127,125,114,95,72,50,32,19,13,13,19,32,49,70,91,110,123,127,124,112,95,75,55,38,25,17,16,20,30,45,64,85,106,121,127,123,106,78,44,8,-23,-50,-70,-84,-92,-94,-93,-87,-75,-58,-34,-6,25,57,86,108,122,127,126,122,115,110,107,107,110,116,123,127,126,118,99,69,31,-9,-48,-81,-105,-119,-126,-127,-127,-126,-126,-126,-127,-127,-124,-115,-99,-76,-45,-11,22,53,78,97,109,117,120,121,120,117,110,98,79,52,17,-20,-59,-92,-115,-126,-126,-118,-105,-91,-78,-68,-64,-64,-70,-80,-93,-107,-119,-126,-127,-119,-102,-78,-51,-24,
	// bitter
	0,-2,-4,-5,-6,-7,-7,-7,-7,-6,-5,-3,-1,0,3,6,9,13,17,22,27,33,40,47,56,65,74,85,96,106,116,123,127,125,113,87,42,-24,-79,-110,-125,-126,-119,-104,-87,-69,-51,-34,-19,-4,9,25,42,62,82,104,121,127,113,65,-28,-99,-125,-125,-108,-86,-62,-40,-20,-2,14,34,55,80,103,123,125,102,25,-73,-118,-127,-114,-92,-67,-45,-24,-6,11,29,49,72,96,116,127,119,82,-1,-84,-119,-127,-118,-101,-80,-60,-41,-25,-10,3,17,31,47,63,80,97,112,123,127,123,107,76,27,-33,-77,-104,-120,-126,-127,-124,-118,-111,-103,-95,-87,-79,-72,-66,-60,-55,-50,-46,-43,-40,-37,-35,-34,-33,-32,-32,-32,-33,-34,-36,-38,-40,-43,-46,-50,-55,-60,-66,-72,-78,-85,-93,-101,-108,-116,-122,-126,-127,-124,-115,-98,-69,-25,31,75,104,121,127,125,118,106,92,77,62,47,33,21,9,-2,-14,-27,-41,-57,-74,-91,-108,-121,-127,-122,-100,-53,22,84,115,127,124,113,98,80,63,46,31,18,5,-6,-18,-31,-46,-61,-77,-93,-107,-119,-126,-127,-115,-92,-49,19,69,100,119,126,127,122,115,106,97,86,76,67,58,50,43,36,30,25,20,15,11,8,4,2,
};

// oscillator variables
unsigned int  osc_accumulator   [TOTAL_VOICES];
unsigned int  osc_pitch         [TOTAL_VOICES];
unsigned int  osc_target_pitch  [TOTAL_VOICES];
unsigned int  osc_tie_flag      [TOTAL_VOICES];

// wavetable variables
unsigned int  osc_sample        [TOTAL_VOICES];
signed   char osc_volume        [TOTAL_VOICES];
signed   char osc_wavetable     [TOTAL_VOICES][WAVETABLE_SIZE];

// envelope variables
unsigned int  env_length        [TOTAL_VOICES];
unsigned int  env_tick          [TOTAL_VOICES];
unsigned char env_type          [TOTAL_VOICES];

// pitch sweep variables
unsigned char swp_length        [TOTAL_VOICES];
unsigned int  swp_tick          [TOTAL_VOICES];
unsigned char swp_type          [TOTAL_VOICES];
unsigned char swp_target        [TOTAL_VOICES];

// effect variables
unsigned char osc_gain          [TOTAL_VOICES];

// mmml variables
unsigned char  mmml_octave      [TOTAL_VOICES],
               mmml_length      [TOTAL_VOICES],
               mmml_volume      [TOTAL_VOICES],
               mmml_note        [TOTAL_VOICES],
               mmml_env_type    [TOTAL_VOICES],
               loops_active     [TOTAL_VOICES],
               current_length   [TOTAL_VOICES];
unsigned int   mmml_env_length  [TOTAL_VOICES],
               data_pointer     [TOTAL_VOICES],
               loop_duration    [MAXLOOPS][TOTAL_VOICES],
               loop_point       [MAXLOOPS][TOTAL_VOICES],
               pointer_location [TOTAL_VOICES];

// temporary data storage variables
unsigned char  buffer1         = 0,
               buffer2         = 0,
               buffer3         = 0;
unsigned int   buffer4         = 0;

// main timer variables
unsigned int   tick_counter    = 0,
               tick_speed      = 0;

//===============//
// sampler stuff //
//===============//

#define DIVISOR 2
#define SAMPLE_1_LENGTH 669 // kick
#define SAMPLE_2_LENGTH 56  // click
#define SAMPLE_3_LENGTH 918 // snare
#define SAMPLE_4_LENGTH 582 // beep
#define SAMPLE_5_LENGTH 93  // clave
#define SAMPLE_6_LENGTH 895 // tom 
#define SAMPLE_7_LENGTH 255 // hh 
#define TOTAL_SAMPLE_LENGTH SAMPLE_1_LENGTH + SAMPLE_2_LENGTH + SAMPLE_3_LENGTH + SAMPLE_4_LENGTH + SAMPLE_5_LENGTH + SAMPLE_6_LENGTH + SAMPLE_7_LENGTH
#define TOTAL_SAMPLER_SAMPLES 7

#define SAMPLER_SPEED 8

const unsigned int sample_index[TOTAL_SAMPLER_SAMPLES + 1] = 
{
	0,
	SAMPLE_1_LENGTH,
	SAMPLE_1_LENGTH + SAMPLE_2_LENGTH,
	SAMPLE_1_LENGTH + SAMPLE_2_LENGTH + SAMPLE_3_LENGTH,
	SAMPLE_1_LENGTH + SAMPLE_2_LENGTH + SAMPLE_3_LENGTH + SAMPLE_4_LENGTH,
	SAMPLE_1_LENGTH + SAMPLE_2_LENGTH + SAMPLE_3_LENGTH + SAMPLE_4_LENGTH + SAMPLE_5_LENGTH,
	SAMPLE_1_LENGTH + SAMPLE_2_LENGTH + SAMPLE_3_LENGTH + SAMPLE_4_LENGTH + SAMPLE_5_LENGTH + SAMPLE_6_LENGTH,
	SAMPLE_1_LENGTH + SAMPLE_2_LENGTH + SAMPLE_3_LENGTH + SAMPLE_4_LENGTH + SAMPLE_5_LENGTH + SAMPLE_6_LENGTH + SAMPLE_7_LENGTH

};

const signed char wavetable[TOTAL_SAMPLE_LENGTH] =
{
	// kick - 669
	1,3,7,78,-34,-48,18,90,95,38,-52,-63,-43,5,50,86,111,111,99,88,67,31,-19,-32,10,68,107,119,124,121,123,116,100,52,-23,-65,-87,-104,-109,-118,-117,-121,-113,-116,-99,-91,-67,-23,29,61,81,98,110,117,125,124,127,122,117,107,78,36,-7,-42,-54,-72,-77,-84,-87,-91,-92,-92,-90,-89,-86,-76,-55,-17,18,52,64,81,90,96,103,106,108,109,109,104,104,98,94,83,61,38,8,-18,-30,-45,-56,-59,-66,-68,-70,-71,-70,-68,-62,-60,-54,-45,-37,-18,0,28,50,67,78,87,92,97,99,101,102,102,100,96,95,89,84,76,67,56,40,18,-13,-42,-66,-81,-90,-97,-103,-107,-110,-112,-113,-113,-113,-110,-107,-103,-97,-91,-79,-71,-55,-40,-14,15,43,67,81,91,99,106,110,114,116,119,121,122,123,123,123,123,122,120,119,116,113,109,103,92,79,62,43,23,5,-9,-24,-35,-47,-55,-65,-72,-79,-85,-91,-96,-100,-104,-107,-110,-112,-114,-116,-116,-118,-118,-118,-117,-116,-112,-107,-98,-89,-77,-63,-51,-42,-33,-25,-18,-10,-3,2,8,13,18,22,26,29,32,35,37,39,41,44,46,49,52,55,58,61,63,65,65,65,63,61,59,57,55,54,55,57,58,60,62,63,64,65,67,68,68,69,69,69,69,69,68,67,66,65,63,61,60,58,56,54,52,50,49,48,49,50,51,52,53,52,49,45,39,33,26,18,10,3,-5,-13,-21,-29,-37,-45,-51,-57,-63,-67,-71,-75,-78,-81,-84,-86,-88,-89,-90,-91,-93,-93,-94,-95,-96,-97,-98,-99,-101,-103,-105,-107,-109,-111,-113,-115,-116,-117,-116,-116,-115,-114,-112,-111,-108,-106,-102,-99,-94,-88,-82,-74,-66,-57,-47,-37,-26,-15,-4,6,16,26,37,47,56,65,72,80,86,92,97,101,105,108,111,114,117,119,121,123,125,126,127,127,127,127,127,127,126,125,124,123,122,121,119,117,115,112,109,105,101,96,91,84,78,71,63,55,47,38,29,21,13,5,-2,-9,-17,-24,-30,-37,-43,-49,-54,-59,-64,-68,-73,-77,-82,-85,-89,-92,-96,-99,-102,-104,-106,-109,-111,-112,-114,-115,-115,-116,-117,-117,-118,-118,-118,-118,-118,-118,-118,-117,-117,-116,-115,-114,-112,-111,-109,-107,-105,-103,-100,-97,-93,-89,-84,-79,-74,-68,-62,-55,-48,-42,-35,-27,-19,-11,-3,4,13,21,29,36,43,50,57,64,71,77,82,87,91,96,99,103,105,107,109,111,112,113,114,115,115,116,116,116,117,117,117,117,117,117,117,117,116,116,116,115,115,115,115,115,115,114,114,114,113,112,111,110,109,107,106,104,102,100,98,95,92,89,85,80,76,70,63,56,48,40,30,21,12,1,-8,-17,-27,-36,-45,-53,-61,-68,-75,-81,-87,-93,-97,-102,-105,-109,-112,-114,-117,-119,-120,-122,-124,-125,-126,-127,-127,-127,-126,-125,-124,-123,-121,-119,-116,-114,-111,-107,-104,-100,-96,-93,-88,-84,-80,-75,-71,-67,-62,-58,-54,-50,-46,-42,-38,-34,-31,-28,-25,-22,-19,-17,-15,-13,-12,-10,-9,-9,-8,-7,-7,-7,-6,-6,-6,-6,-5,-5,-5,-5,-4,-4,-4,-4,-4,-3,-3,-3,-3,-3,-3,-3,-2,-2,-2,-2,-2,-2,-2,-2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,

	//click - 56
	0,5,-7,0,2,4,-8,-1,113,127,85,-29,-99,-95,-74,22,85,68,60,-32,-78,-44,-41,13,76,25,13,-4,-62,-19,1,-4,46,26,-17,3,-29,-25,17,4,10,24,-14,-9,-3,-17,5,11,-2,9,-2,-14,0,-2,-3,1,

	// snare - 918
	5,-6,120,124,104,71,48,-11,-32,-46,-124,-107,-95,-61,-109,-66,-108,-38,-65,-55,-46,-35,-17,21,-24,46,81,-27,69,127,54,77,62,127,-18,52,9,13,-32,-126,-35,-53,-53,-76,-110,-49,-77,-63,26,-87,-55,7,59,64,36,87,47,-19,-3,120,-62,-18,51,-100,45,-88,-72,-19,-39,-47,-26,-78,14,-9,78,46,96,71,28,92,-2,104,17,-3,94,-94,-18,-3,-64,-28,-81,-79,-15,48,-83,-59,-51,-20,-1,36,11,54,92,19,9,93,-20,-24,16,-6,3,-39,17,-66,45,-9,31,48,-62,-31,127,-8,50,19,32,47,51,35,33,-7,69,4,-1,46,-26,9,-41,39,-101,-49,-59,-37,-13,13,0,42,-40,53,8,-11,46,34,10,-20,41,-8,-7,-7,10,44,-6,-73,-20,0,22,37,7,23,-39,93,-4,23,0,33,53,-25,2,43,26,-3,-31,10,-1,-27,-60,11,-20,-2,-7,-32,8,-41,10,-27,-50,-39,87,-17,7,19,28,-40,3,-14,19,-30,27,33,-58,79,-27,-45,-19,-11,68,0,-28,53,7,-68,49,18,7,-51,79,-24,-25,0,-14,-49,53,-20,-64,43,-53,-36,-28,-35,36,14,-42,-47,30,42,-69,55,-10,43,-43,12,-4,59,-33,27,26,-36,8,64,-59,31,12,-20,1,24,28,-46,-4,53,-20,-19,7,-6,8,-29,25,-30,35,-6,-80,24,28,-69,-10,-45,39,-47,24,31,-73,25,25,-36,32,18,11,-19,9,-4,-6,-5,92,-48,26,2,-18,49,-45,30,1,32,-40,3,-23,15,25,13,-30,-31,51,-21,-27,30,-16,1,-17,-3,-30,-9,-52,57,-13,-21,12,-38,-3,24,-2,-17,46,-31,-3,4,-2,0,63,-33,7,16,20,5,-26,24,1,-4,23,5,-20,16,-5,25,-38,7,-2,-6,18,-25,17,-15,-7,31,-19,-35,-4,-36,36,0,-35,5,-28,-6,59,-37,-34,30,9,-5,21,-34,26,0,19,6,2,30,-33,0,49,-40,21,26,-15,-18,-13,65,-30,-16,5,-24,10,30,-39,-24,45,-16,-30,31,-18,-42,27,0,-25,-24,32,-4,-48,58,-36,19,-32,23,14,-18,-31,45,21,-18,0,-2,21,-20,42,7,-39,41,-11,30,-5,-18,5,2,-8,-18,42,-6,-3,-16,-7,10,-7,-23,4,21,-27,-21,32,-29,-20,22,0,-32,35,-7,-9,-13,13,-21,25,0,14,-27,19,31,-26,18,-11,17,-15,52,-16,-1,14,-30,45,-6,-39,49,-19,16,-25,-8,32,-25,-3,-6,-1,-2,-21,16,-15,19,-12,-1,-14,-18,38,-37,-5,13,6,-3,-2,6,-3,-26,52,-3,-30,42,-4,-25,29,2,21,-16,-7,17,13,8,1,-30,10,40,-33,2,-6,4,5,-22,25,-12,-18,-7,10,-7,14,-29,-1,9,-18,17,-11,-21,33,-18,6,3,-5,-7,-7,42,-12,-2,12,-7,-2,26,-17,20,1,4,3,10,-26,14,8,8,-1,-10,0,5,0,6,-4,-24,-1,14,7,-15,-4,4,-14,0,-3,-13,12,2,-14,4,6,3,-13,1,-7,-3,30,-2,-4,-8,17,4,12,-28,15,22,-23,27,-2,-9,-9,36,-16,-6,9,-7,22,-25,15,-29,20,-1,-4,1,-8,2,-13,8,-15,16,-23,-4,16,-4,1,-16,0,16,-3,1,-16,4,14,2,7,-6,0,-14,25,5,3,-3,-4,21,-20,5,16,-4,-2,6,1,-4,-1,-1,-10,10,0,6,-13,-7,10,-18,9,6,-8,-19,12,0,-7,0,-15,22,-11,12,-14,0,19,-7,-3,-7,13,9,-3,5,-4,-1,19,-9,11,-15,8,12,-19,16,8,-20,11,0,-10,13,-11,5,-5,1,-2,0,-12,-6,17,-2,-12,-3,-2,4,-4,10,-4,-12,2,6,-1,2,3,-13,19,-11,10,-11,7,10,2,0,-10,17,-4,-6,13,-5,7,1,-13,17,-5,-7,7,-8,10,-7,-8,6,2,1,-12,0,5,-2,-4,-7,1,4,-11,6,9,-10,-1,9,-9,-2,10,-3,-3,8,-6,8,5,0,-9,0,16,-6,2,-2,7,0,-2,-4,9,-2,-2,0,6,-6,-4,2,-6,7,0,-4,-4,0,-5,8,-9,6,-1,-5,-8,13,0,-5,-8,15,-9,5,0,-5,6,-7,10,-7,10,-3,-7,12,1,-7,4,3,-3,5,-4,0,2,-1,2,-7,6,-5,5,-3,1,-7,0,5,-3,-3,-3,-1,2,-2,3,0,-5,1,-3,1,6,-7,4,-2,1,0,4,0,-3,4,0,4,-2,0,0,0,0,7,-5,2,3,-2,-4,4,5,-8,0,4,-5,3,0,-3,-1,0,

	// beep - 582
	-6,15,121,117,127,67,-70,-114,-112,-110,-107,-104,-44,73,109,103,102,100,94,12,-70,-90,-89,-87,-85,-59,6,75,90,88,87,85,46,-14,-54,-69,-72,-66,-51,-19,21,50,64,65,56,37,11,-11,-23,-26,-23,-16,-8,0,1,-1,-6,-10,-11,-5,7,24,37,46,47,38,16,-15,-41,-56,-63,-59,-46,-13,32,66,84,87,79,54,4,-45,-70,-79,-79,-70,-45,6,60,85,85,85,78,38,-22,-62,-77,-79,-76,-63,-25,32,74,82,80,80,63,11,-41,-68,-76,-75,-68,-46,0,49,76,79,78,70,36,-13,-50,-66,-70,-66,-54,-24,18,51,68,72,64,45,10,-25,-47,-56,-57,-49,-33,-5,21,38,45,43,34,16,-5,-22,-30,-32,-29,-23,-12,-1,5,7,6,3,0,-2,-2,0,4,7,7,4,-4,-15,-25,-31,-33,-30,-20,-2,16,30,39,39,31,12,-14,-36,-50,-56,-53,-43,-20,11,38,54,59,53,36,3,-31,-53,-64,-66,-59,-41,-7,29,53,65,65,53,25,-13,-44,-61,-68,-65,-53,-27,10,41,59,65,58,40,8,-27,-49,-61,-63,-56,-40,-10,21,42,53,53,43,22,-7,-32,-46,-52,-50,-40,-22,1,22,33,37,33,22,4,-14,-26,-32,-33,-29,-21,-8,2,9,12,11,8,2,-3,-7,-9,-8,-6,-5,-4,-6,-9,-12,-15,-15,-13,-8,0,9,15,18,17,11,0,-14,-25,-32,-34,-31,-22,-5,13,26,34,35,29,14,-7,-26,-38,-44,-43,-35,-18,5,25,38,44,40,28,7,-17,-35,-44,-47,-42,-30,-8,16,33,43,44,36,20,-3,-25,-38,-44,-43,-35,-19,3,22,34,39,36,26,8,-12,-27,-35,-37,-33,-24,-8,9,21,28,29,24,13,0,-14,-22,-25,-25,-20,-11,0,8,13,15,13,9,3,-3,-8,-10,-10,-8,-6,-3,-1,0,-1,-2,-2,-2,-1,1,4,6,7,6,3,-2,-8,-13,-16,-16,-13,-7,2,11,16,19,18,14,4,-7,-17,-22,-24,-22,-16,-4,9,19,25,26,22,13,0,-14,-23,-27,-27,-22,-12,2,16,24,28,27,20,8,-7,-18,-25,-27,-25,-18,-5,8,19,25,26,22,14,1,-11,-19,-23,-23,-19,-11,0,11,18,21,20,15,7,-3,-11,-16,-17,-15,-11,-4,4,9,13,13,11,7,1,-3,-7,-8,-8,-6,-3,0,2,3,4,3,2,1,0,0,0,1,2,1,0,-1,-3,-5,-6,-5,-4,0,3,7,9,10,9,6,0,-5,-9,-12,-12,-10,-5,1,8,12,15,14,11,5,-3,-9,-13,-15,-14,-10,-3,5,12,16,17,15,10,1,-6,-12,-15,-15,-13,-7,0,8,13,16,16,12,6,-1,-8,-12,-14,-13,-10,-3,4,9,13,14,12,8,2,-4,-8,-10,-11,-9,-5,0,5,8,9,9,7,4,0,-3,-5,-6,-5,-3,-1,0,2,2,2,1,1,0,

	//clave - 93
	2,-17,35,108,-69,-97,69,89,-87,-55,107,35,-118,0,120,-32,-107,65,78,-93,-45,109,4,-105,40,89,-74,-49,98,5,-96,41,73,-78,-30,89,-20,-74,61,35,-78,12,67,-52,-29,69,-17,-53,52,13,-60,28,35,-53,5,45,-40,-11,47,-27,-21,43,-16,-27,38,-8,-29,33,-3,-29,29,-1,-27,26,0,-24,24,-2,-20,22,-5,-16,21,-8,-11,18,-11,-4,12,-7,0,1,0,

	//tom - 895
	3,-16,17,124,104,97,85,81,76,67,20,-24,-58,-85,-106,-119,-126,-125,-122,-119,-116,-115,-112,-110,-106,-100,-52,-13,0,11,19,29,37,51,71,89,102,100,97,95,93,92,92,91,91,91,91,92,79,48,16,-1,-6,-11,-17,-27,-36,-42,-45,-56,-70,-82,-86,-84,-84,-82,-83,-82,-83,-82,-84,-81,-86,-69,-22,-4,3,4,6,8,13,18,24,30,38,53,68,82,87,86,85,84,84,84,84,84,84,84,84,82,58,27,4,-4,-8,-13,-21,-30,-36,-38,-44,-57,-68,-79,-83,-81,-81,-80,-81,-80,-81,-80,-81,-80,-84,-52,-13,-2,4,4,7,9,14,17,23,27,33,45,59,70,80,85,84,83,83,83,83,83,83,82,83,81,78,61,35,13,-2,-9,-17,-26,-32,-35,-36,-44,-55,-65,-74,-81,-82,-81,-81,-81,-81,-81,-81,-81,-81,-82,-77,-35,-8,0,4,6,9,12,15,19,23,26,33,45,56,65,73,79,83,83,83,83,83,83,82,80,77,74,70,68,57,34,16,0,-12,-22,-28,-32,-33,-38,-47,-56,-64,-71,-77,-82,-83,-83,-83,-83,-83,-83,-83,-83,-83,-84,-77,-39,-12,-1,4,8,12,15,18,21,23,30,40,49,57,64,69,73,77,78,79,79,78,76,74,72,69,66,62,58,53,46,30,11,-2,-13,-20,-25,-27,-33,-42,-50,-57,-63,-69,-73,-76,-79,-81,-83,-84,-84,-83,-82,-80,-77,-73,-69,-63,-57,-40,-16,-2,6,11,16,18,22,30,38,45,52,58,62,66,68,70,70,70,70,68,67,64,61,58,54,50,45,39,32,25,17,9,0,-7,-16,-24,-32,-40,-47,-53,-58,-63,-67,-70,-73,-74,-75,-76,-75,-74,-73,-70,-67,-63,-58,-53,-47,-40,-32,-24,-16,-7,1,10,18,26,34,41,47,52,56,60,62,64,65,65,65,64,62,60,57,54,50,46,41,35,29,23,16,8,1,-6,-14,-21,-29,-36,-43,-49,-55,-59,-63,-67,-70,-72,-73,-74,-74,-73,-72,-70,-67,-64,-60,-55,-50,-44,-37,-30,-20,-7,2,8,12,18,26,32,39,45,50,54,58,60,62,64,65,65,64,63,61,59,56,52,48,44,39,33,28,21,15,8,1,-6,-13,-20,-27,-33,-40,-45,-50,-55,-59,-62,-65,-67,-68,-69,-69,-69,-68,-66,-64,-61,-57,-53,-48,-43,-37,-30,-24,-17,-9,-2,4,12,19,25,31,37,42,47,51,54,57,59,61,61,61,61,60,58,56,53,50,46,42,38,33,27,21,15,9,3,-3,-10,-16,-22,-29,-34,-40,-45,-49,-53,-57,-60,-62,-64,-65,-65,-65,-65,-64,-62,-59,-57,-53,-49,-45,-40,-34,-28,-22,-16,-9,-3,3,9,16,22,28,33,38,42,46,50,52,55,56,57,58,58,57,56,54,52,49,46,43,39,34,29,24,19,14,8,2,-3,-9,-15,-20,-26,-31,-36,-41,-45,-48,-52,-54,-57,-58,-60,-60,-60,-60,-59,-58,-56,-53,-50,-47,-43,-39,-34,-29,-24,-19,-13,-7,-1,4,9,15,20,25,30,35,39,42,45,48,50,52,53,53,53,53,52,50,49,46,44,40,37,33,29,25,20,15,10,5,0,-4,-10,-15,-20,-25,-29,-33,-37,-41,-44,-47,-49,-51,-53,-54,-54,-54,-54,-53,-52,-51,-48,-46,-43,-40,-36,-32,-28,-23,-18,-14,-9,-3,1,6,11,15,20,24,28,32,35,38,41,43,45,46,47,48,48,47,46,45,44,41,39,36,33,30,26,23,19,14,10,6,1,-2,-7,-11,-16,-20,-24,-28,-31,-34,-37,-40,-42,-44,-46,-47,-48,-48,-48,-48,-47,-46,-44,-43,-40,-38,-35,-32,-28,-25,-21,-17,-13,-8,-4,0,3,8,12,16,20,23,26,30,32,35,37,38,40,41,42,42,42,41,40,39,38,36,34,31,29,26,23,20,16,12,9,5,1,-2,-6,-9,-13,-17,-20,-23,-26,-29,-32,-34,-36,-38,-39,-41,-41,-42,-42,-42,-41,-41,-40,-38,-37,-35,-32,-30,-27,-24,-21,-18,-14,-11,-7,-4,0,3,6,10,13,16,19,22,25,27,29,31,32,34,35,35,36,36,36,35,34,33,32,30,28,26,24,22,19,16,13,10,7,4,1,-2,-5,-8,-11,-14,-17,-19,-22,-24,-26,-27,-28,-29,-30,-30,-30,-30,-30,-29,-28,-27,-26,-24,-22,-20,-19,-17,-15,-13,-11,-9,-7,-5,-4,-2,-1,0,0,1,2,3,3,3,3,3,3,3,3,3,2,2,2,1,1,1,0,

	// hh - 255
	0,0,107,-67,-20,80,-100,-61,127,-19,-81,97,-115,58,8,14,0,127,-67,46,-37,42,48,-57,60,-28,38,-13,24,12,-45,73,-10,5,19,-1,19,2,5,50,-59,39,9,0,23,-8,30,-23,62,-67,45,7,1,15,0,15,-3,26,-40,79,-36,22,0,12,0,13,-3,17,12,-46,62,-33,35,-20,31,-22,37,-41,69,-42,17,0,9,-3,14,-11,23,-23,17,24,-28,23,-8,8,0,0,6,-3,-11,42,-36,21,-9,6,0,-1,6,-7,-1,29,-37,23,-12,6,-1,-2,6,-13,20,-17,-12,23,-21,16,-14,10,-11,7,-7,-8,28,-33,15,-9,3,-4,0,-1,-3,5,-24,36,-34,15,-13,7,-11,7,-13,10,-18,8,3,-24,15,-14,4,-7,0,-4,-2,-4,4,-27,25,-22,5,-7,-2,-2,-6,1,-10,3,-6,-16,17,-21,7,-10,0,-6,-2,-2,-6,4,-22,23,-24,6,-9,0,-7,0,-7,0,-7,-2,0,-20,16,-18,4,-8,0,-6,-1,-6,0,-10,6,-20,10,-8,-6,1,-8,1,-8,0,-7,0,-4,-7,6,-17,2,-3,-6,1,-8,2,-9,4,-11,7,-16,11,-13,-1,0,-6,0,-5,0,-4,0,-5,1,-6,4,-8,3,-3,0,
};

unsigned int  sample_slowdown_counter;
unsigned int  sample_accumulator;
unsigned int  sample_start;
unsigned int  sample_end;
unsigned char current_sample;
unsigned char pause;

//===== BEGIN THE PROGRAM =====//

int main(int argc, char *argv[])
{
	// define & open the output file
	FILE* output_wave_file;
	output_wave_file = fopen("./output.wav", "w");

	//===== WAVE HEADER CODE =====//

	wavfile_header_t wav_header;

	wav_header.chunk_id[0] = 'R';
	wav_header.chunk_id[1] = 'I';
	wav_header.chunk_id[2] = 'F';
	wav_header.chunk_id[3] = 'F';

	wav_header.chunk_size = CHUNK_SIZE;

	wav_header.format[0] = 'W';
	wav_header.format[1] = 'A';
	wav_header.format[2] = 'V';
	wav_header.format[3] = 'E';

	wav_header.subchunk_1_id[0] = 'f';
	wav_header.subchunk_1_id[1] = 'm';
	wav_header.subchunk_1_id[2] = 't';
	wav_header.subchunk_1_id[3] = ' ';

	wav_header.subchunk_1_size  = SUBCHUNK_1_SIZE;
	wav_header.audio_format     = AUDIO_FORMAT;
	wav_header.total_channels   = TOTAL_CHANNELS;
	wav_header.sample_rate      = SAMPLE_RATE;
	wav_header.byte_rate        = BYTE_RATE;
	wav_header.block_align      = BLOCK_ALIGN;
	wav_header.bits_per_sample  = BITS_PER_SAMPLE;

	wav_header.subchunk_2_id[0] = 'd';
	wav_header.subchunk_2_id[1] = 'a';
	wav_header.subchunk_2_id[2] = 't';
	wav_header.subchunk_2_id[3] = 'a';
	wav_header.subchunk_2_size  = SUBCHUNK_2_SIZE;

	// write header to file
	fwrite(&wav_header, sizeof(wavfile_header_t), 1, output_wave_file);

	//===== WAVE SAMPLE DATA CODE =====//

	unsigned long current_sample;

	printf("Starting synthesis...\n");

	// initial variables
	for (unsigned char v = 0; v < TOTAL_VOICES; v++)
	{
		// sampler default values
		osc_pitch [v] = 0;
		osc_volume[v] = 0;
		osc_sample[v] = 0;
		env_type  [v] = 0;
		env_length[v] = 0;
		swp_type  [v] = 0;
		swp_length[v] = 0;
		osc_gain  [v] = 1;

		// mmml default values
		mmml_volume[v]     = 24;
		mmml_octave[v]     = 2;
		mmml_env_type[v]   = 0;
		mmml_env_length[v] = 20;

		data_pointer[v] = (unsigned char)mmml_data[(v*2)] << 8;
		data_pointer[v] = data_pointer[v] | (unsigned char)mmml_data[(v*2)+1];

		update_wavetable(v,osc_sample[v],osc_volume[v],osc_gain[v]);
	}

	//=====================//
	// MAIN SYNTHESIS LOOP //
	//=====================//

	for(unsigned long t = 0; t < TOTAL_SAMPLES - 1; t++)
	{

		//==================//
		// generate samples //
		//==================//

		int output = 0;

		// oscillators
		for (unsigned char v = 0; v < TOTAL_VOICES - 1; v++) // all voices minus the last (sampler)
			output += osc_wavetable[v][(unsigned char)((osc_accumulator[v] += osc_pitch[v]) >> 9)];

		// sampler
		int sample_pos = sample_start + (sample_accumulator++ / (mmml_octave[TOTAL_VOICES-1] + 2));

		if (sample_pos > sample_end)
			pause = 1;

		if (pause == 0)
			output += (float)wavetable[sample_pos] * (1.0 / ((MAX_VOLUME - 16) / (float)mmml_volume[TOTAL_VOICES-1]));

		// mix it all together
		output = 127 + (output / (TOTAL_VOICES + 1));
		buffer[t] = output;

		//=====================//
		// envelope management //
		//=====================//

		for (unsigned char v = 0; v < TOTAL_VOICES - 1; v++)
		{
			// decay
			if (env_type[v] == 0 && osc_volume[v] > -1 && env_tick[v] == 0)
			{
				update_wavetable(v,osc_sample[v],osc_volume[v]--,osc_gain[v]);
				env_tick[v] = env_length[v] << 6;
			}
			// swell
			else if (env_type[v] == 1 && osc_volume[v] < (mmml_volume[v] + 1) && env_tick[v] == 0)
			{
				update_wavetable(v,osc_sample[v],osc_volume[v]++,osc_gain[v]);
				env_tick[v] = env_length[v] << 6;
			}
			else if (env_tick[v] > 0)
				env_tick[v]--;

			//==================//
			// sweep management //
			//==================//

			/* sweep mode 0 used for no sweep */

			// fall to target
			if (swp_type[v] == 1 && osc_pitch[v] > osc_target_pitch[v] && swp_tick[v] == 0)
			{
				osc_pitch[v]--;
				swp_tick[v] = swp_length[v] << 2;
			}
			// rise to target
			else if (swp_type[v] == 2 && osc_pitch[v] < osc_target_pitch[v] && swp_tick[v] == 0)
			{
				osc_pitch[v]++;
				swp_tick[v] = swp_length[v] << 2;
			}
			else if (swp_tick[v] > 0)
				swp_tick[v]--;
		}

		mmml();

		//=====================//
	}

	printf("Writing to buffer...\n");

	// write sample data to file
	fwrite(buffer, sizeof(unsigned char), TOTAL_SAMPLES-1, output_wave_file);

	printf("Successfully written!\n");
}

//=================//
// music sequencer //
//=================//

void mmml()
{
	if(tick_counter-- == 0)
	{
		// Variable tempo, sets the fastest / smallest possible clock event.
		tick_counter = tick_speed;

		for(unsigned char v = 0; v < TOTAL_VOICES; v++)
		{
			// If the note ended, start processing the next byte of data.
			if(mmml_length[v] == 0){
				LOOP:

				// Temporary storage of data for quick processing.
				// first nibble of data
				buffer1 = ((unsigned char)mmml_data[(data_pointer[v])] >> 4);
				// second nibble of data
				buffer2 = (unsigned char)mmml_data[data_pointer[v]] & 0x0F;

				// function command
				if(buffer1 == 15)
				{
					// Another buffer for commands that require an additional byte.
					buffer3 = (unsigned char)mmml_data[data_pointer[v] + 1];

					// loop start
					if(buffer2 == 0)
					{
						loops_active[v]++;
						loop_point[loops_active[v] - 1][v] = data_pointer[v] + 2;
						loop_duration[loops_active[v] - 1][v] = buffer3 - 1;
						data_pointer[v]+= 2;
					}
					// loop end
					else if(buffer2 == 1)
					{
						/* If we reach the end of the loop and the duration isn't zero,
						 * decrement our counter and start again. */
						if(loop_duration[loops_active[v] - 1][v] > 0)
						{
							data_pointer[v] = loop_point[loops_active[v] - 1 ][v];
							loop_duration[loops_active[v] - 1][v]--;
						}
						// If we're done, move away from the loop.
						else
						{
							loops_active[v]--;
							data_pointer[v]++;
						}
					}
					// macro
					else if(buffer2 == 2)
					{
						pointer_location[v] = data_pointer[v] + 2;
						
						data_pointer[v] = (unsigned char)mmml_data[(buffer3 + TOTAL_VOICES) * 2] << 8;
						data_pointer[v] = data_pointer[v] | (unsigned char)mmml_data[((buffer3 + TOTAL_VOICES) * 2) + 1];
					}
					// tempo
					else if(buffer2 == 3)
					{
						tick_speed = buffer3 * 14;
						data_pointer[v] += 2;
					}
					// instrument
					else if(buffer2 == 5)
					{
						configure_instrument(v,buffer3);
						data_pointer[v] += 2;
					}
					// tie
					else if(buffer2 == 6)
					{
						osc_tie_flag[v] = 1;
						data_pointer[v]++;
					}
					// channel end
					else if(buffer2 == 15)
					{
						// If we've got a previous position saved, go to it...
						if(pointer_location[v] != 0)
						{
							data_pointer[v] = pointer_location[v];
							pointer_location[v] = 0;
						}
						// ...If not, go back to the start.
						else
						{
							data_pointer[v] = (unsigned char) mmml_data[(v*2)] << 8;
							data_pointer[v] = data_pointer[v] | (unsigned char) mmml_data[(v*2)+1];
						}
					}

					/* For any command that should happen 'instantaneously' (e.g. anything
					 * that isn't a note or rest - in mmml notes can't be altered once
					 * they've started playing), we need to keep checking this loop until we
					 * find an event that requires waiting. */

					goto LOOP;
				}

				if(buffer1 == 13 || buffer1 == 14)
				{
					// octave
					if(buffer1 == 13)
						mmml_octave[v] = buffer2;
					// volume
					if(buffer1 == 14)
						mmml_volume[v] = buffer2 * (MAX_VOLUME / 8);

					data_pointer[v]++;
					goto LOOP; //see comment above previous GOTO
				}

				// note value
				if(buffer1 != 0 && buffer1 < 14)
				{
					if(osc_tie_flag[v] == 0)
					{
						// trigger the oscillators
						if (v < TOTAL_VOICES - 1)
						{
							env_type[v]   = mmml_env_type[v];
							mmml_note[v]  = buffer1;

							// no sweep
							if(swp_type[v] == 0)
							{
								osc_pitch[v]  = notes[mmml_note[v]+(mmml_octave[v]*12)];
							}
							// sweep mode 1 sets a target pitch to fall to
							else if(swp_type[v] == 1)
							{
								osc_pitch[v] = notes[mmml_note[v]+(mmml_octave[v]*12)];

								// prevent underflow
								if ((int)(mmml_note[v]+(mmml_octave[v]*12) - swp_target[v]) < 0)
									osc_target_pitch[v] = 0;
								else
									osc_target_pitch[v] = notes[mmml_note[v]+(mmml_octave[v]*12) - swp_target[v]];
							}
							// sweep mode 2 sets a target pitch to rise to
							else if(swp_type[v] == 2)
							{
								// prevent underflow
								if ((int)(mmml_note[v]+(mmml_octave[v]*12) - swp_target[v]) < 0)
									osc_pitch[v] = 0;
								else
									osc_pitch[v] = notes[mmml_note[v]+((mmml_octave[v]*12) - swp_target[v])];
								
								osc_target_pitch[v]  = notes[mmml_note[v]+(mmml_octave[v]*12)];
							}

							// compensate envelope length for level of volume
							env_length[v] = mmml_env_length[v] * (MAX_VOLUME / mmml_volume[v]);

							if (env_type[v] == 0)
								osc_volume[v] = mmml_volume[v];
							else
								osc_volume[v] = 0;

							env_tick[v] = env_length[v];

							update_wavetable(v,osc_sample[v],osc_volume[v],osc_gain[v]);
						}
						// trigger the sampler (last voice is always sampler)
						else if (v == TOTAL_VOICES - 1)
						{
							current_sample = (buffer1 - 1) % TOTAL_SAMPLER_SAMPLES;

							sample_start = sample_index[current_sample];
							sample_end   = sample_index[current_sample + 1];

							sample_accumulator = 0;
							pause = 0;
						}
					}
					//
					else if (osc_tie_flag[v] == 1)
						osc_tie_flag[v] = 0;
				}
				// rest
				else
				{
					// trigger the oscillators
					if (v < TOTAL_VOICES-1)
					{
						// avoid clicky note-offs by turning on the decay envelope
						env_type[v] = 0;
						env_length[v] = 1;
					}
					else if (v == TOTAL_VOICES-1)
						pause = 1;

					// clear tie flag here too
					osc_tie_flag[v] = 0;
				}

				// note duration value
				if(buffer2 < 8)
					// standard duration
					mmml_length[v] = 127 >> buffer2;
				else
					// dotted (1 + 1/2) duration
					mmml_length[v] = 95 >> (buffer2 & 7);

				// next element in data
				data_pointer[v]++;
			}
			// keep waiting until the note is over...
			else
				mmml_length[v]--;
		}
	}
}

void configure_instrument(unsigned char voice, unsigned char instrument_id)
{
	
	/* Here's where you can define your own custom instruments. Ideally this would be
	 * part of the mmml language, but there's no point right now as this will generally
	 * be compiled from source for each application.
     *
	 * Instrument variables available are:
	 * - Waveform
	 * - Envelope type (decay or swell)
	 * - Envelope length (in ticks)
	 * - Sweep direction (rise to target, or fall to target)
	 * - Sweep length (in ticks)
	 * - Sweep target (in semitones)
	 */
	
	switch(instrument_id)
	{
		// rising epiano
		case 0:
			mmml_env_type   [voice] = 0;
			mmml_env_length [voice] = 150;
			osc_sample      [voice] = 2;
			swp_type        [voice] = 2;
			swp_length      [voice] = 2;
			swp_target      [voice] = 5;
		break;

		// epiano swell
		case 1:
			mmml_env_type   [voice] = 1;
			mmml_env_length [voice] = 150;
			osc_sample      [voice] = 2;
			swp_type        [voice] = 0;
			swp_target      [voice] = 0;
		break;

		// epiano swell drop
		case 2:
			mmml_env_type   [voice] = 0;
			mmml_env_length [voice] = 20;
			osc_sample      [voice] = 2;
			swp_type        [voice] = 1;
			swp_length      [voice] = 100;
			swp_target      [voice] = 1;
		break;
	}
}

void update_wavetable(unsigned char voice, unsigned char sample, unsigned char volume, unsigned char gain)
{
	int output;

	for (int i = 0; i < WAVETABLE_SIZE; i++)
	{
		// read sample
		output = wavetable_data[(i + (sample * WAVETABLE_SIZE))];
		// apply gain (for distortion)
		output = (int)output * gain;
		// apply volume
		output = (output * volume) >> 5;
		// write to table
		osc_wavetable[voice][i] = output;
	}
}