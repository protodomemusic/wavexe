/**************************************************************
*  FILENAME:     wavexe.c
*  DESCRIPTION:  Executable Wavetable Synth
*
*  NOTES:        A simple wavetable synth which outputs
*                directly into a 16-bit stereo WAV file.
*                Sequencing powered by:
*                https://github.com/protodomemusic/mmml
*                Reverb powered by:
*                https://github.com/protodomemusic/cverb
*
*                The wave building code was adapted from:
*                https://stackoverflow.com/questions/23030980/
*                creating-a-mono-wav-file-using-c - thanks
*                Safayet Ahmed!
*
*                TO DO:
*                - Migrate to -1.0 - 1.0 signal levels.
*                - MIDI -> MMML conversion.
*                - Sort out the weird mixing/balancing to work
*                  better w/ variable voices and ensure no
*                  clipping.
*                - Make volumes ('v' commands) more dynamic.
*                - Portamento.
*                - Link LFO to different parameters?
*                - Needs a limiter, or at least normalisation.
*                - Reverb on every track? Delay? Dither?
*                - Define custom instruments in MMML.
*
*                BUGS:
*                - Weird 'notch' in waveform when switching
*                  between pitches.
*                - Number of voices is broken for some numbers.
*                  Selecting only 6 or 8 works right now.
*
*  AUTHOR:       Blake 'PROTODOME' Troise
*  PLATFORM:     Command Line Application (MacOS)
*  DATE:         Original: 10th February 2021
*                Remaster: 14th February 2022
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "freeverb.c"
#include "wavexe-mmml-data.h"
#include "wavexe-sample-data.h"

#define PLAY_TIME        80     // duration of recording in seconds
#define SAMPLE_RATE      44100  // cd quality audio
#define BITS_PER_SAMPLE  16     // 16-bit wave files require 16 bits per sample
#define AUDIO_FORMAT     1      // for PCM data
#define TOTAL_CHANNELS   2      // mono file
#define SUBCHUNK_1_SIZE  16     // dunno, what's this?
#define BYTE_RATE        SAMPLE_RATE * TOTAL_CHANNELS * BITS_PER_SAMPLE / 8
#define BLOCK_ALIGN      TOTAL_CHANNELS * BITS_PER_SAMPLE / 8
#define TOTAL_SAMPLES    PLAY_TIME * SAMPLE_RATE
#define SUBCHUNK_2_SIZE  TOTAL_SAMPLES * TOTAL_CHANNELS * BITS_PER_SAMPLE / 8
#define CHUNK_SIZE       4 + (8 + SUBCHUNK_1_SIZE) + (8 + SUBCHUNK_2_SIZE)

// holds the 44 byte header information
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
int16_t buffer_int   [TOTAL_SAMPLES];
float   buffer_float [TOTAL_SAMPLES];

// important definitions
#define MAXLOOPS      5
#define TOTAL_VOICES  6
#define TOTAL_NOTES   72
#define OSC_GAIN      20
#define SMP_GAIN      30
#define MOD_SPEED     200

const uint16_t notes[TOTAL_NOTES] =
{
//  c    c#   d    d#   e    f    f#   g    g#   a    a#   b	
	131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247,
	262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
	523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988,
	1046,1109,1175,1244,1318,1397,1480,1568,1661,1760,1865,1976,
	2093,2217,2349,2489,2637,2794,2960,3136,3322,3520,3729,3951,
	4186,4435,4699,4978,5274,5588,5920,6272,6645,7040,7459,7902,
};

// misc variables
uint8_t  mod_counter = MOD_SPEED;

// oscillator variables
uint32_t osc_accumulator   [TOTAL_VOICES - 1];
uint16_t osc_pitch         [TOTAL_VOICES - 1];
uint16_t osc_target_pitch  [TOTAL_VOICES - 1];
uint16_t osc_tie_flag      [TOTAL_VOICES - 1];

// wavetable variables
float    osc_volume        [TOTAL_VOICES - 1];
uint8_t  osc_mix           [TOTAL_VOICES - 1];
uint8_t  osc_sample_1      [TOTAL_VOICES - 1];
uint8_t  osc_sample_2      [TOTAL_VOICES - 1];
int16_t  osc_wavetable     [TOTAL_VOICES - 1][WAVETABLE_SIZE];
uint8_t  osc_instrument    [TOTAL_VOICES - 1];

// lfo variables
uint32_t lfo_accumulator   [TOTAL_VOICES - 1];
uint16_t lfo_pitch         [TOTAL_VOICES - 1];
int8_t   lfo_amplitude     [TOTAL_VOICES - 1];
int8_t   lfo_intensity     [TOTAL_VOICES - 1];
int8_t   lfo_waveform      [TOTAL_VOICES - 1];
int16_t  lfo_output        [TOTAL_VOICES - 1];
uint16_t lfo_tick          [TOTAL_VOICES - 1];
uint16_t lfo_length        [TOTAL_VOICES - 1];

// volume envelope variables
uint16_t env_vol_length    [TOTAL_VOICES - 1];
uint16_t env_vol_tick      [TOTAL_VOICES - 1];
uint8_t  env_vol_type      [TOTAL_VOICES - 1];

// waveform envelope variables
uint16_t env_wav_length    [TOTAL_VOICES - 1];
uint16_t env_wav_tick      [TOTAL_VOICES - 1];
uint8_t  env_wav_type      [TOTAL_VOICES - 1];

// pitch sweep variables
uint8_t  swp_length        [TOTAL_VOICES - 1];
uint16_t swp_tick          [TOTAL_VOICES - 1];
uint8_t  swp_type          [TOTAL_VOICES - 1];
uint8_t  swp_target        [TOTAL_VOICES - 1];

// mmml variables
float    mmml_volume       [TOTAL_VOICES];
uint16_t mmml_octave       [TOTAL_VOICES];
uint16_t mmml_length       [TOTAL_VOICES];
uint16_t mmml_note         [TOTAL_VOICES];
int8_t   mmml_transpose    [TOTAL_VOICES];
uint8_t  loops_active      [TOTAL_VOICES];
uint8_t  current_length    [TOTAL_VOICES];
uint16_t data_pointer      [TOTAL_VOICES];
uint16_t loop_duration     [MAXLOOPS][TOTAL_VOICES];
uint16_t loop_point        [MAXLOOPS][TOTAL_VOICES];
uint16_t pointer_location  [TOTAL_VOICES];

// sampler variables
uint16_t sample_slowdown_counter;
uint16_t sample_accumulator;
uint16_t sample_start;
uint16_t sample_end;
uint8_t  sample_current;
uint8_t  sample_mute;

// functions of convenience
void mmml();

// stealing the arduino map function - it's very handy indeed!
float map(float x, float in_min, float in_max, float out_min, float out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/* Here's where you can define your own custom instruments. Ideally this would be
 * part of the mmml language, but there's no point right now as this will generally
 * be compiled from source for each application.
 *
 * Instrument variables available are:
 * - Waveform number #1 (starting waveform)
 * - Waveform number #2 (destination waveform)
 * - Waveform blend duration (how long it takes to fade between the wavecycles)
 * - Envelope type (decay or swell)
 * - Envelope length (in ticks)
 * - Sweep direction (rise to target, or fall to target)
 * - Sweep length (in ticks)
 * - Sweep target (in semitones)
 * - LFO frequency (in Hz)
 * - LFO intensity (target amplitude)
 * - LFO waveform (taken from the wavetable data)
 * - LFO length (how long it takes to reach full amplitude)
 */

#define TOTAL_INSTRUMENTS     18
#define INSTRUMENT_PARAMETERS 12

uint8_t instrument_bank [TOTAL_INSTRUMENTS][INSTRUMENT_PARAMETERS] =
{
//------------------------------------------------------------------//
//   waveform     /**/  volume  /**/  sweep   /**/  lfo (vibrato)   //
//------------------------------------------------------------------//
// 0: synth clav
	{  4,3,10,    /**/  0,120,  /**/  2,1,1,   /**/  12,10,0,35   },
// 1: rhodes
	{  9,8,20,    /**/  0,120,  /**/  0,0,0,   /**/  0,0,0,0      },
// 2: soft square
	{  7,6,100,   /**/  0,120,  /**/  0,0,0,   /**/  11,1,2,156   },
// 3: glass
	{  11,0,10,   /**/  0,80,   /**/  0,0,0,   /**/  12,1,2,56    },
// 4: plucked string
	{  12,8,10,   /**/  0,200,  /**/  0,0,0,   /**/  0,0,0,0      },
// 5: thin rectangle
	{  5,4,100,   /**/  0,100,  /**/  2,4,1,   /**/  14,1,0,100   },
// 6: soft epiano
	{  8,0,200,   /**/  0,200,  /**/  0,0,0,   /**/  14,1,2,100   },
// 7: bassoon-ish
	{  12,10,100, /**/  0,200,  /**/  0,0,0,   /**/  0,0,0,0      },
// 8: popcorn
	{  2,0,3,     /**/  0,30,   /**/  2,1,1,   /**/  0,0,0,0      },
// 9: harpsichord piano hybrid
	{  10,3,10,   /**/  0,30,   /**/  0,0,0,   /**/  0,0,0,0      },
//10: tom/kick
	{  12,0,1,    /**/  0,5,    /**/  1,2,24,  /**/  0,0,0,0      },
//11: drop sfx #1 - sounds good at o5
	{  12,10,1,   /**/  0,80,   /**/  1,15,72, /**/  60,126,12,0  },
//12: drop sfx #2 - sounds good at o5
	{  3,8,200,   /**/  0,50,   /**/  1,10,72, /**/  200,100,12,0 },
//13: rise sfx #1 - sounds good at o5 or o6
	{  10,11,10,  /**/  1,100,  /**/  2,20,48, /**/  200,100,12,0 },
//14: rise sfx #2 - sounds good at o6
	{  3,9,10,    /**/  1,100,  /**/  2,15,48, /**/  100,100,5,0  },
//15: 5th saw2sine
	{  14,13,200, /**/  0,150,  /**/  2,1,1,   /**/  0,0,0,0      },
//16: test sine
	{  0,0,255,   /**/  0,255,  /**/  0,0,0,   /**/  0,0,0,0      },
//17: reverse piano
	{  9,8,20,    /**/  1,120,  /**/  0,0,0,   /**/  14,10,0,35   },
//------------------------------------------------------------------//
};

void configure_instrument(uint8_t voice, uint8_t instrument_id)
{
	// waveform
	osc_sample_1   [voice] = instrument_bank[instrument_id][0];
	osc_sample_2   [voice] = instrument_bank[instrument_id][1];
	env_wav_length [voice] = instrument_bank[instrument_id][2] * 2; // scale for int8_t to int16
	env_wav_tick   [voice] = 1;
	// volume
	env_vol_type   [voice] = instrument_bank[instrument_id][3];
	env_vol_length [voice] = instrument_bank[instrument_id][4] * 4;
	env_vol_tick   [voice] = 1;
	// sweep
	swp_type       [voice] = instrument_bank[instrument_id][5];
	swp_length     [voice] = instrument_bank[instrument_id][6];
	swp_target     [voice] = instrument_bank[instrument_id][7];
	swp_tick       [voice] = 1;
	// lfo
	lfo_pitch      [voice] = instrument_bank[instrument_id][8];
	lfo_intensity  [voice] = instrument_bank[instrument_id][9];
	lfo_waveform   [voice] = instrument_bank[instrument_id][10];
	lfo_length     [voice] = instrument_bank[instrument_id][11] * 10; // scale for int8 to int16
	lfo_tick       [voice] = 1; // prevent lfo update glitch on lfo_tick zero
}

void update_wavetable(uint8_t voice, int8_t sample_1, int8_t sample_2, float volume, uint8_t mix_percentage)
{
	int16_t output;

	for (int16_t i = 0; i < WAVETABLE_SIZE; i++)
	{
		// read sample 1
		output = wavetable_data[(i + (sample_1 * WAVETABLE_SIZE))];
		// merge with sample 2
		output = map(mix_percentage, 0, 100, output, wavetable_data[(i + (sample_2 * WAVETABLE_SIZE))]);
		// apply volume
		if (volume > 0)
			output = (output * OSC_GAIN) * volume;
		else
			output = 0;
		// write to table
		osc_wavetable[voice][i] = output;
	}
}

//===== BEGIN THE PROGRAM =====//

int main()
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

	printf("Starting synthesis...\n");
	printf("Total samples: %u\n", TOTAL_SAMPLES);

	//===== WAVE SAMPLE DATA CODE =====//

	uint32_t sample_current;

	// initial variables
	for (uint8_t v = 0; v < TOTAL_VOICES; v++)
	{
		// mmml default values
		mmml_volume    [v] = 0.5;
		mmml_octave    [v] = 2;
		mmml_transpose [v] = 3;

		data_pointer[v] = (uint8_t)mmml_data[v * 2] << 8;
		data_pointer[v] = data_pointer[v] | (uint8_t)mmml_data[(v*2)+1];

		update_wavetable(v, osc_sample_1[v], osc_sample_2[v], osc_volume[v], osc_mix[v]);
		configure_instrument(v, 0);
	}

	//=====================//
	// MAIN SYNTHESIS LOOP //
	//=====================//

	for(uint32_t t = 0; t < TOTAL_SAMPLES - 1; t+=2)
	{
		//==================//
		// generate samples //
		//==================//

		int16_t output = 0;

		//~~~~ oscillators ~~~~//
		for (uint8_t v = 0; v < TOTAL_VOICES - 1; v++) // all voices minus the last (sampler)
			output += osc_wavetable[v][(uint8_t)((osc_accumulator[v] += (osc_pitch[v] + lfo_output[v])) >> 9)];
		//~~~~~~~~~~~~~~~~~~~~~//

		//<><><> sampler <><><>//
		int16_t sample_pos = sample_start + (sample_accumulator++ / (mmml_octave[TOTAL_VOICES - 1] + 2));

		// silence sample at end
		if (sample_pos > sample_end)
			sample_mute = 1;

		// play sample and adjust volume
		if (sample_mute == 0)
			output += (float)((wavetable[sample_pos] * SMP_GAIN) * mmml_volume[TOTAL_VOICES - 1]);
		//<><><><><><><><><><>//

		// stick the data in a .wav file
		buffer_int[t]   = output;
		buffer_int[t+1] = output;

		//=============//
		// update lfos //
		//=============//

		for (uint8_t v = 0; v < TOTAL_VOICES - 1; v++) // all voices minus the last (sampler)
		{
			lfo_output[v] = wavetable_data[(uint8_t)((lfo_accumulator[v] += lfo_pitch[v]) >> 9) + (lfo_waveform[v] * 4)];
			lfo_output[v] = (lfo_output[v] * lfo_amplitude[v]) >> 5;
		}

		// lfo swell / lfo delay
		for (uint8_t v = 0; v < TOTAL_VOICES - 1; v++)
		{
			if (lfo_amplitude[v] < (lfo_intensity[v] + 1) && lfo_tick[v] == 0)
			{
				lfo_amplitude[v]++;
				lfo_tick[v] = lfo_length[v] << 6;
			}
			else if (lfo_tick[v] > 0)
				lfo_tick[v]--;
		}

		//=====================//
		// envelope management //
		//=====================//

		if (mod_counter-- == 0)
		{
			for (uint8_t v = 0; v < TOTAL_VOICES - 1; v++)
			{
				//==== amplitude envelopes ====//

				if (env_vol_tick[v] <= env_vol_length[v])
				{
					if (env_vol_type[v] == 0)
					{
						osc_volume[v] = map(env_vol_tick[v], 0, env_vol_length[v], mmml_volume[v], 0);
						update_wavetable(v, osc_sample_1[v], osc_sample_2[v], osc_volume[v], osc_mix[v]);
						env_vol_tick[v]++;
					}
					else if (env_vol_type[v] == 1)
					{
						osc_volume[v] = map(env_vol_tick[v], 0, env_vol_length[v], 0, mmml_volume[v]);
						update_wavetable(v, osc_sample_1[v], osc_sample_2[v], osc_volume[v], osc_mix[v]);
						env_vol_tick[v]++;
					}
				}

				//==== waveform envelopes ====//

				if (env_wav_tick[v] <= env_wav_length[v])
				{
					osc_mix[v] = map(env_wav_tick[v], 0, env_wav_length[v], 0, 100);
					update_wavetable(v, osc_sample_1[v], osc_sample_2[v], osc_volume[v], osc_mix[v]);
					env_wav_tick[v]++;
				}
			}

			mod_counter = MOD_SPEED;
		}

		//==== pitch envelopes ====//

		// sweep mode 0 used for no sweep
		// happens a bit faster than the other envelopes

		for (uint8_t v = 0; v < TOTAL_VOICES - 1; v++)
		{
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

		mmml(); // where the magic happens

		//=====================//
	}

	//=================//
	// end the program //
	//=================//

	// printf("Applying reverb...\n");

	// convert int to float (to send to fv_process)
	for (uint32_t i = 0; i < TOTAL_SAMPLES; i++)
		buffer_float[i] = (float)buffer_int[i];

	// process reverb
	reverb_process(buffer_float,TOTAL_SAMPLES);

	// convert float back to int
	for (uint32_t i = 0; i < TOTAL_SAMPLES; i++)
		buffer_int[i] = (int16_t)buffer_float[i];

	printf("Writing to buffer...\n");

	// write sample data to file
	fwrite(buffer_int, sizeof(uint16_t), TOTAL_SAMPLES-1, output_wave_file);

	printf("Successfully written!\n");
}

//=================//
// music sequencer //
//=================//

void mmml()
{
	// main timer variables
	static uint16_t tick_counter = 0;
	static uint16_t tick_speed   = 700; // default tempo

	// temporary data storage variables
	uint8_t  buffer1 = 0;
	uint8_t  buffer2 = 0;
	uint8_t  buffer3 = 0;
	uint16_t buffer4 = 0;

	if (tick_counter-- == 0)
	{
		// Variable tempo, sets the fastest / smallest possible clock event.
		tick_counter = tick_speed;

		for (uint8_t v = 0; v < TOTAL_VOICES; v++)
		{
			// If the note ended, start processing the next byte of data.
			if (mmml_length[v] == 0)
			{
				LOOP:

				// Temporary storage of data for quick processing.
				// first nibble of data
				buffer1 = ((uint8_t)mmml_data[(data_pointer[v])] >> 4);
				// second nibble of data
				buffer2 = (uint8_t)mmml_data[data_pointer[v]] & 0x0F;

				// function command
				if (buffer1 == 15)
				{
					// Another buffer for commands that require an additional byte.
					buffer3 = (uint8_t)mmml_data[data_pointer[v] + 1];

					// loop start
					if (buffer2 == 0)
					{
						loops_active[v]++;
						loop_point[loops_active[v] - 1][v] = data_pointer[v] + 2;
						loop_duration[loops_active[v] - 1][v] = buffer3 - 1;
						data_pointer[v]+= 2;
					}
					// loop end
					else if (buffer2 == 1)
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
					else if (buffer2 == 2)
					{
						pointer_location[v] = data_pointer[v] + 2;
						
						data_pointer[v] = (uint8_t)mmml_data[(buffer3 + TOTAL_VOICES) * 2] << 8;
						data_pointer[v] = data_pointer[v] | (uint8_t)mmml_data[((buffer3 + TOTAL_VOICES) * 2) + 1];
					}
					// tempo
					else if (buffer2 == 3)
					{
						tick_speed = buffer3 * 14;
						data_pointer[v] += 2;
					}
					// transpose
					else if (buffer2 == 4)
					{
						mmml_transpose[v] = buffer3;
						data_pointer[v] += 2;
					}
					// instrument
					else if (buffer2 == 5)
					{
						osc_instrument[v] = buffer3;
						data_pointer[v] += 2;
					}
					// tie
					else if (buffer2 == 6)
					{
						osc_tie_flag[v] = 1;
						data_pointer[v]++;
					}
					// channel end
					else if (buffer2 == 15)
					{
						// If we've got a previous position saved, go to it...
						if (pointer_location[v] != 0)
						{
							data_pointer[v] = pointer_location[v];
							pointer_location[v] = 0;
						}
						// ...If not, go back to the start.
						else
						{
							data_pointer[v] = (uint8_t) mmml_data[(v*2)] << 8;
							data_pointer[v] = data_pointer[v] | (uint8_t) mmml_data[(v*2)+1];
						}
					}

					/* For any command that should happen 'instantaneously' (e.g. anything
					 * that isn't a note or rest - in mmml notes can't be altered once
					 * they've started playing), we need to keep checking this loop until we
					 * find an event that requires waiting. */

					goto LOOP;
				}

				if (buffer1 == 13 || buffer1 == 14)
				{
					// octave
					if(buffer1 == 13)
						mmml_octave[v] = buffer2;
					// volume
					if(buffer1 == 14)
						mmml_volume[v] = buffer2 / 8.0;

					data_pointer[v]++;
					goto LOOP; //see comment above previous GOTO
				}

				// note value
				if (buffer1 != 0 && buffer1 < 14)
				{
					if (osc_tie_flag[v] == 0)
					{
						// trigger the oscillators
						if (v < TOTAL_VOICES - 1)
						{
							// reset the lfo
							lfo_amplitude[v] = 0;

							configure_instrument(v,osc_instrument[v]);

							mmml_note[v] = buffer1;

							// no sweep
							if (swp_type[v] == 0)
							{
								osc_pitch[v] = notes[(mmml_note[v]+(mmml_octave[v]*12)) + mmml_transpose[v]];
							}
							// sweep mode 1 sets a target pitch to fall to
							else if (swp_type[v] == 1)
							{
								osc_pitch[v] = notes[(mmml_note[v]+(mmml_octave[v]*12)) + mmml_transpose[v]];

								// prevent underflow
								if ((int16_t)((mmml_note[v]+(mmml_octave[v]*12) - swp_target[v]) + mmml_transpose[v]) < 0)
									osc_target_pitch[v] = 0;
								else
									osc_target_pitch[v] = notes[mmml_note[v]+(mmml_octave[v]*12) - swp_target[v]];
							}
							// sweep mode 2 sets a target pitch to rise to
							else if (swp_type[v] == 2)
							{
								// prevent underflow
								if ((int16_t)((mmml_note[v]+(mmml_octave[v]*12) - swp_target[v])) + mmml_transpose[v] < 0)
									osc_pitch[v] = 0;
								else
									osc_pitch[v] = notes[(mmml_note[v]+((mmml_octave[v]*12) - swp_target[v])) + mmml_transpose[v]];
								
								osc_target_pitch[v] = notes[(mmml_note[v]+(mmml_octave[v]*12))  + mmml_transpose[v]];
							}

							// reset the waveform mix
							osc_mix[v] = 0;

							if (env_vol_type[v] == 0)
								osc_volume[v] = mmml_volume[v];
							else
								osc_volume[v] = 0.0;

							env_vol_tick[v] = 0;

							update_wavetable(v, osc_sample_1[v], osc_sample_2[v], osc_volume[v], osc_mix[v]);
						}
						// trigger the sampler (last voice is always sampler)
						else if (v == TOTAL_VOICES - 1)
						{
							sample_current = (buffer1 - 1) % TOTAL_SAMPLER_SAMPLES;

							sample_start = sample_index[sample_current];
							sample_end   = sample_index[sample_current + 1];

							sample_accumulator = 0;
							sample_mute = 0;
						}
					}
					else if (osc_tie_flag[v] == 1)
						osc_tie_flag[v] = 0;
				}
				// rest
				else
				{
					// trigger the oscillators
					if (v < TOTAL_VOICES-1)
					{
						configure_instrument(v,osc_instrument[v]);

						// avoid clicky note-offs by turning on the decay envelope
						env_vol_type[v] = 0;
						env_vol_length[v] = 1;
					}
					else if (v == TOTAL_VOICES-1)
						sample_mute = 1;

					// clear tie flag here too
					osc_tie_flag[v] = 0;
				}

				// note duration value
				if (buffer2 < 8)
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