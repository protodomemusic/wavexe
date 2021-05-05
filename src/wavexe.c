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
*                You must define an output on compilation.
*                For the wave exporter, ensure that
*                WAVE_EXPORTER is uncommented. For piped
*                output, uncomment PIPE_OUTPUT.
*
*                To hear the results of the piped output,
*                you'll need to use the following commands:
*                - macOS (with sox):
*                  ./wavexe | play -t raw -e unsigned-integer
*                  -b 8 -c 1 -r 44100 -
*                - Linux
*                  ./wavexe | aplay -r 44100
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
*                - Portamento.
*                - Stereo.
*                - Link LFO to different parameters?
*                - Fancy DSP maths stuff like limiters, reverb
*                  and filters.
*
*  AUTHOR:       Blake 'PROTODOME' Troise
*  PLATFORM:     Command Line Application (Windows/MacOS/Linux)
*  DATE:         10th February 2021
************************************************************H*/

#define WAVE_EXPORTER
//#define PIPE_OUTPUT

#include "wavexe-mmml-data.h"
#include "wavexe-sample-data.h"

#define PLAY_TIME        20     // duration of recording in seconds
#define SAMPLE_RATE      44100  // cd quality audio
#define AMPLITUDE        127    // waveform high position (maximum from DC zero is 127)
#define DC_OFFSET        127    // waveform low position (127 is DC zero)
#define BITS_PER_SAMPLE  8      // 8-bit wave files require 8 bits per sample
#define AUDIO_FORMAT     1      // for PCM data
#define TOTAL_CHANNELS   1      // mono file
#define SUBCHUNK_1_SIZE  16     // dunno, what's this?
#define BYTE_RATE        SAMPLE_RATE * TOTAL_CHANNELS * BITS_PER_SAMPLE / 8
#define BLOCK_ALIGN      TOTAL_CHANNELS * BITS_PER_SAMPLE / 8
#define TOTAL_SAMPLES    PLAY_TIME * SAMPLE_RATE
#define SUBCHUNK_2_SIZE  TOTAL_SAMPLES * TOTAL_CHANNELS * BITS_PER_SAMPLE / 8
#define CHUNK_SIZE       4 + (8 + SUBCHUNK_1_SIZE) + (8 + SUBCHUNK_2_SIZE)

// why not remove all libraries?
#if !defined(WAVE_EXPORTER) && !defined(PIPE_OUTPUT) || defined(WAVE_EXPORTER)
	#include <stdio.h>
#endif

#ifdef WAVE_EXPORTER
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
#endif

// functions of convenience
void mmml();
void configure_instrument(unsigned char voice, unsigned char instrument_id);

#ifdef PIPE_OUTPUT
	// alternative putchar when without stdio.h
	void putchar(char c)
	{
		extern long write(int, const char *, unsigned long);
		(void) write(1, &c, 1);
	}
#endif

// important definitions
#define MAXLOOPS      5
#define TOTAL_VOICES  8
#define TOTAL_NOTES   72

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

// oscillator variables
unsigned int  osc_accumulator   [TOTAL_VOICES - 1];
unsigned int  osc_pitch         [TOTAL_VOICES - 1];
unsigned int  osc_target_pitch  [TOTAL_VOICES - 1];
unsigned int  osc_tie_flag      [TOTAL_VOICES - 1];
unsigned int  osc_sample        [TOTAL_VOICES - 1];
signed   char osc_volume        [TOTAL_VOICES - 1];

// lfo variables
unsigned int  lfo_accumulator   [TOTAL_VOICES - 1];
unsigned int  lfo_pitch         [TOTAL_VOICES - 1];
signed   char lfo_amplitude     [TOTAL_VOICES - 1];
signed   char lfo_intensity     [TOTAL_VOICES - 1];
signed   char lfo_waveform      [TOTAL_VOICES - 1];
signed   int  lfo_output        [TOTAL_VOICES - 1];
unsigned int  lfo_tick          [TOTAL_VOICES - 1];
unsigned int  lfo_length        [TOTAL_VOICES - 1];

// envelope variables
unsigned int  env_length        [TOTAL_VOICES - 1];
unsigned int  env_tick          [TOTAL_VOICES - 1];
unsigned char env_type          [TOTAL_VOICES - 1];

// pitch sweep variables
unsigned char swp_length        [TOTAL_VOICES - 1];
unsigned int  swp_tick          [TOTAL_VOICES - 1];
unsigned char swp_type          [TOTAL_VOICES - 1];
unsigned char swp_target        [TOTAL_VOICES - 1];

// mmml variables
unsigned char mmml_octave      [TOTAL_VOICES];
unsigned char mmml_length      [TOTAL_VOICES];
unsigned char mmml_volume      [TOTAL_VOICES];
unsigned char mmml_note        [TOTAL_VOICES];
unsigned char mmml_env_type    [TOTAL_VOICES];
unsigned int  mmml_env_length  [TOTAL_VOICES];
unsigned char loops_active     [TOTAL_VOICES];
unsigned char current_length   [TOTAL_VOICES];
unsigned int  data_pointer     [TOTAL_VOICES];
unsigned int  loop_duration    [MAXLOOPS][TOTAL_VOICES];
unsigned int  loop_point       [MAXLOOPS][TOTAL_VOICES];
unsigned int  pointer_location [TOTAL_VOICES];

// sampler variables
unsigned int  sample_slowdown_counter;
unsigned int  sample_accumulator;
unsigned int  sample_start;
unsigned int  sample_end;
unsigned char sample_current;
unsigned char sample_mute;

//===== BEGIN THE PROGRAM =====//

int main(int argc, char *argv[])
{
	#ifdef WAVE_EXPORTER
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
	#endif

	//===== WAVE SAMPLE DATA CODE =====//

	unsigned long sample_current;

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

		// mmml default values
		mmml_volume[v]     = 24;
		mmml_octave[v]     = 2;
		mmml_env_type[v]   = 0;
		mmml_env_length[v] = 20;

		data_pointer[v] = (unsigned char)mmml_data[(v*2)] << 8;
		data_pointer[v] = data_pointer[v] | (unsigned char)mmml_data[(v*2)+1];
	}

	//=====================//
	// MAIN SYNTHESIS LOOP //
	//=====================//

	for(unsigned long t = 0; t < TOTAL_SAMPLES - 1; t++)
	{
		//=============//
		// update lfos //
		//=============//

		for (unsigned char v = 0; v < TOTAL_VOICES - 1; v++) // all voices minus the last (sampler)
		{
			lfo_output[v] = wavetable_data[(unsigned char)((lfo_accumulator[v] += lfo_pitch[v]) >> 9) + (lfo_waveform[v] * 4)];
			lfo_output[v] = (lfo_output[v] * lfo_amplitude[v]) >> 5;
		}

		// lfo swell / lfo delay
		for (unsigned char v = 0; v < TOTAL_VOICES - 1; v++)
		{
			if (lfo_amplitude[v] < (lfo_intensity[v] + 1) && lfo_tick[v] == 0)
			{
				lfo_amplitude[v]++;
				lfo_tick[v] = lfo_length[v] << 6;
			}
			else if (lfo_tick[v] > 0)
				lfo_tick[v]--;
		}

		//==================//
		// generate samples //
		//==================//

		int output = 0;

		// oscillators
		for (unsigned char v = 0; v < TOTAL_VOICES - 1; v++) // all voices minus the last (sampler)
			output += ((wavetable_data[(unsigned char)((osc_accumulator[v] += (osc_pitch[v] + lfo_output[v])) >> 9) + (osc_sample[v] * 256)]) * osc_volume[v]) >> 5;

		// sampler
		int sample_pos = sample_start + (sample_accumulator++ / (mmml_octave[TOTAL_VOICES-1] + 2));

		if (sample_pos > sample_end)
			sample_mute = 1;

		if (sample_mute == 0)
			output += (float)wavetable[sample_pos] * (1.0 / ((MAX_VOLUME - 16) / (float)mmml_volume[TOTAL_VOICES-1]));

		// mix it all together
		output = 127 + (output / (TOTAL_VOICES + 1));

		// stick the data in a .wav file
		#ifdef WAVE_EXPORTER
			buffer[t] = output;
		#endif

		// on linux: ./wavexe | aplay -r 441000
		// on macos: ./wavexe | play -t raw -e unsigned-integer -b 8 -c 1 -r 44100 -
		#ifdef PIPE_OUTPUT
			putchar(output);
		#endif

		//=====================//
		// envelope management //
		//=====================//

		for (unsigned char v = 0; v < TOTAL_VOICES - 1; v++)
		{
			// decay
			if (env_type[v] == 0 && osc_volume[v] > -1 && env_tick[v] == 0)
			{
				osc_volume[v]--;
				env_tick[v] = env_length[v] << 6;
			}
			// swell
			else if (env_type[v] == 1 && osc_volume[v] < (mmml_volume[v] + 1) && env_tick[v] == 0)
			{
				osc_volume[v]++;
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

		mmml(); // where the magic happens

		//=====================//
	}

	//=================//
	// end the program //
	//=================//

	#ifdef WAVE_EXPORTER
		printf("Writing to buffer...\n");

		// write sample data to file
		fwrite(buffer, sizeof(unsigned char), TOTAL_SAMPLES-1, output_wave_file);

		printf("Successfully written!\n");
	#endif

	#if !defined(WAVE_EXPORTER) && !defined(PIPE_OUTPUT)
		printf("No build definitions, no sound.\n");
	#endif
}

void configure_instrument(unsigned char voice, unsigned char instrument_id)
{
	
	/* Here's where you can define your own custom instruments. Ideally this would be
	 * part of the mmml language, but there's no point right now as this will generally
	 * be compiled from source for each application.
     *
	 * Instrument variables available are:
	 * - Waveform number
	 * - Envelope type (decay or swell)
	 * - Envelope length (in ticks)
	 * - Sweep direction (rise to target, or fall to target)
	 * - Sweep length (in ticks)
	 * - Sweep target (in semitones)
	 * - LFO frequency (in Hz)
	 * - LFO intensity (target amplitude)
	 * - LFO waveform (taken from the wavetable data)
	 */
	
	switch(instrument_id)
	{
		// rising epiano
		case 0:
			osc_sample      [voice] = 8;

			mmml_env_type   [voice] = 0;
			mmml_env_length [voice] = 150;

			swp_type        [voice] = 2;
			swp_length      [voice] = 2;
			swp_target      [voice] = 5;

			lfo_pitch       [voice] = 13;
			lfo_intensity   [voice] = 20;
			lfo_waveform    [voice] = 0;
			lfo_length      [voice] = 200;
		break;

		// epiano swell
		case 1:
			osc_sample      [voice] = 8;

			mmml_env_type   [voice] = 1;
			mmml_env_length [voice] = 150;

			swp_type        [voice] = 0;
			swp_length      [voice] = 0;
			swp_target      [voice] = 0;

			lfo_pitch       [voice] = 0;
			lfo_intensity   [voice] = 0;
			lfo_waveform    [voice] = 0;
			lfo_length      [voice] = 0;
		break;

		// epiano swell drop
		case 2:
			osc_sample      [voice] = 8;

			mmml_env_type   [voice] = 0;
			mmml_env_length [voice] = 20;

			swp_type        [voice] = 1;
			swp_length      [voice] = 100;
			swp_target      [voice] = 1;

			lfo_pitch       [voice] = 40;
			lfo_intensity   [voice] = 10;
			lfo_waveform    [voice] = 0;
			lfo_length      [voice] = 0;
		break;
	}
}

//=================//
// music sequencer //
//=================//

void mmml()
{
	// main timer variables
	static unsigned int tick_counter = 0;
	static unsigned int tick_speed   = 0;

	// temporary data storage variables
	unsigned char  buffer1 = 0;
	unsigned char  buffer2 = 0;
	unsigned char  buffer3 = 0;
	unsigned int   buffer4 = 0;

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
							env_type[v]  = mmml_env_type[v];
							mmml_note[v] = buffer1;

							// no sweep
							if(swp_type[v] == 0)
							{
								osc_pitch[v] = notes[mmml_note[v]+(mmml_octave[v]*12)];
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

							// reset the lfo
							lfo_amplitude[v] = 0;

							// compensate envelope length for level of volume
							env_length[v] = mmml_env_length[v] * (MAX_VOLUME / mmml_volume[v]);

							if (env_type[v] == 0)
								osc_volume[v] = mmml_volume[v];
							else
								osc_volume[v] = 0;

							env_tick[v] = env_length[v];
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
						sample_mute = 1;

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