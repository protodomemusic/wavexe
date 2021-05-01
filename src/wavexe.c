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
*                - support ties (&)
*                - fix triggering issues with 'swell'
*                - weird 'notch' in waveform when switching between pitches
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
#define TOTAL_VOICES     5
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
		mmml_octave[v]     = 3;
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

		//=======================//
		// oscillator management //
		//=======================//

		int output = 0;

		for (unsigned char v = 0; v < TOTAL_VOICES; v++)
			output += osc_wavetable[v][(unsigned char)((osc_accumulator[v] += osc_pitch[v]) >> 9)];

		/* output */
		buffer[t] = 127 + (output / TOTAL_VOICES);

		//=====================//
		// envelope management //
		//=====================//

		for (unsigned char v = 0; v < TOTAL_VOICES; v++)
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
			else if (env_tick > 0)
				env_tick[v]--;

			//==================//
			// sweep management //
			//==================//

			// fall
			if (swp_type[v] == 0 && osc_pitch[v] > notes[0] && swp_tick[v] == 0)
			{
				osc_pitch[v]--;
				swp_tick[v] = swp_length[v] << 2;
			}
			// rise
			if (swp_type[v] == 1 && osc_pitch[v] < notes[TOTAL_NOTES-1] && swp_tick[v] == 0)
			{
				osc_pitch[v]++;
				swp_tick[v] = swp_length[v] << 2;
			}
			else if (swp_tick > 0)
				swp_tick[v]--;
		}

		mmml();

		//=====================//
	}

	printf("Writing to buffer...\n");

	// write sample data to file
	fwrite(buffer, sizeof(unsigned char), TOTAL_SAMPLES, output_wave_file);

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
						tick_speed = buffer3 << 4;
						data_pointer[v] += 2;
					}
					// instrument
					else if(buffer2 == 5)
					{
						configure_instrument(v,buffer3);
						data_pointer[v] += 2;
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
						mmml_volume[v] = buffer2 * 4;

					data_pointer[v]++;
					goto LOOP; //see comment above previous GOTO
				}

				// note value
				if(buffer1 != 0 && buffer1 < 14)
				{
					env_type[v]   = mmml_env_type[v];
					mmml_note[v]  = buffer1;
					osc_pitch[v]  = notes[mmml_note[v]+(mmml_octave[v]*12)];

					// compensate envelope length for level of volume
					env_length[v] = mmml_env_length[v] * (MAX_VOLUME / mmml_volume[v]);

					if (env_type[v] == 0)
						osc_volume[v] = mmml_volume[v];
					else
						osc_volume[v] = 0;

					env_tick[v] = env_length[v];
					update_wavetable(v,osc_sample[v],osc_volume[v],osc_gain[v]);
				}
				// rest
				else
				{
					// avoid clicky note-offs by turning on the decay envelope
					env_type[v] = 0;
					env_length[v] = 2;
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
	 * - Sweep direction (rise or fall)
	 * - Sweep length (in ticks)
	 */
	
	switch(instrument_id)
	{
		// sine
		case 0:
			mmml_env_type   [voice] = 1;
			mmml_env_length [voice] = 0;
			osc_sample      [voice] = 0;
		break;

		// epiano
		case 1:
			mmml_env_type   [voice] = 0;
			mmml_env_length [voice] = 40;
			osc_sample      [voice] = 2;
		break;

		// saw swell
		case 2:
			mmml_env_type   [voice] = 1;
			mmml_env_length [voice] = 80;
			osc_sample      [voice] = 1;
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