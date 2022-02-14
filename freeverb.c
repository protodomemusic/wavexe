/*******************************************************************************
 * 
 * Public domain C implementation of the original freeverb, with the addition of
 * support for sample rates other than 44.1khz. Written by rxi, found at:
 * https://gist.github.com/rxi/e5488c6660154329ddfc4a7a7d2997f8
 *
 * Classic freeverb C++ version written by Jezar at Dreampoint, June 2000
 *
 * Edits here are alterations to rxi's C version to make it as portable as
 * possible (some languages don't allow passing of structs into functions, nor
 * arrays in structs).
 * 
 *******************************************************************************/

#define NUMCOMBS       8
#define NUMALLPASSES   4
#define MUTED          0.0
#define FIXEDGAIN      0.015
#define SCALEWET       3
#define SCALEDRY       2
#define SCALEDAMP      0.4
#define SCALEROOM      0.28
#define STEREOSPREAD   23
#define OFFSETROOM     0.7
#define INITIALROOM    0.5
#define INITIALDAMP    0.5
#define INITIALWET     (1.0 / SCALEWET)
#define INITIALDRY     1.0
#define INITIALWIDTH   1.0
#define INITIALMODE    0.0
#define INITIALSR      44100.0
#define FREEZEMODE     0.5
#define TOTAL_CHANNELS 2

float comb_feedback    [TOTAL_CHANNELS][NUMCOMBS];
float comb_filterstore [TOTAL_CHANNELS][NUMCOMBS];
float comb_damp1       [TOTAL_CHANNELS][NUMCOMBS];
float comb_damp2       [TOTAL_CHANNELS][NUMCOMBS];
float comb_buf         [TOTAL_CHANNELS][NUMCOMBS][4096];
int   comb_bufsize     [TOTAL_CHANNELS][NUMCOMBS];
int   comb_bufidx      [TOTAL_CHANNELS][NUMCOMBS];

float allpass_feedback [TOTAL_CHANNELS][NUMALLPASSES];
float allpass_buf      [TOTAL_CHANNELS][NUMALLPASSES][2048];
int   allpass_bufsize  [TOTAL_CHANNELS][NUMALLPASSES];
int   allpass_bufidx   [TOTAL_CHANNELS][NUMALLPASSES];

// reverb context variables
float verb_mode      = 0;
float verb_gain      = 0;
float verb_roomsize  = 0;
float verb_roomsize1 = 0;
float verb_damp      = 0;
float verb_damp1     = 0;
float verb_wet       = 0;
float verb_wet1      = 0;
float verb_wet2      = 0;
float verb_dry       = 0;
float verb_width     = 0;

static inline float allpass_process(int chn, int obj, float input)
{
	float bufout = allpass_buf[chn][obj][allpass_bufidx[chn][obj]];

	float output = -input + bufout;
	allpass_buf[chn][obj][allpass_bufidx[chn][obj]] = input + bufout * allpass_feedback[chn][obj];

	if (++allpass_bufidx[chn][obj] >= allpass_bufsize[chn][obj])
		allpass_bufidx[chn][obj] = 0;

	return output;
}

static inline float comb_process(int chn, int obj, float input)
{
	float output = comb_buf[chn][obj][comb_bufidx[chn][obj]];

	comb_filterstore[chn][obj] = output * comb_damp2[chn][obj] + comb_filterstore[chn][obj] * comb_damp1[chn][obj];

	comb_buf[chn][obj][comb_bufidx[chn][obj]] = input + comb_filterstore[chn][obj] * comb_feedback[chn][obj];

	if (++comb_bufidx[chn][obj] >= comb_bufsize[chn][obj])
		comb_bufidx[chn][obj] = 0;

	return output;
}

static inline void comb_set_damp(int chn, int obj, float n)
{
	comb_damp1[chn][obj] = n;
	comb_damp2[chn][obj] = 1.0 - n;
}

static void reverb_update()
{
	verb_wet1 = verb_wet * (verb_width * 0.5 + 0.5);
	verb_wet2 = verb_wet * ((1 - verb_width) * 0.5);

	if (verb_mode >= FREEZEMODE)
	{
		verb_roomsize1 = 1;
		verb_damp1 = 0;
		verb_gain = MUTED;

	}
	else
	{
		verb_roomsize1 = verb_roomsize;
		verb_damp1 = verb_damp;
		verb_gain = FIXEDGAIN;
	}

	for (int i = 0; i < NUMCOMBS; i++)
	{
		comb_feedback[0][i] = verb_roomsize1;
		comb_feedback[1][i] = verb_roomsize1;
		comb_set_damp(0, i, verb_damp1);
		comb_set_damp(1, i, verb_damp1);
	}
}

void reverb_process(float *buf, int n)
{
	/*===== INITIALISE PARAMETERS =====*/

	for (int i = 0; i < NUMALLPASSES; i++)
	{
		allpass_feedback[0][i] = 0.5;
		allpass_feedback[1][i] = 0.5;
	}

	//------- set sample rate -------//

	const int combs[]     = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
	const int allpasses[] = { 556, 441, 341, 225 };

	double multiplier = INITIALSR / INITIALSR;

	// init comb buffers
	for (int i = 0; i < NUMCOMBS; i++)
	{
		comb_bufsize[0][i] = combs[i] * multiplier;
		comb_bufsize[1][i] = (combs[i] + STEREOSPREAD) * multiplier;
	}

	// init allpass buffers
	for (int i = 0; i < NUMALLPASSES; i++)
	{
		allpass_bufsize[0][i] = allpasses[i] * multiplier;
		allpass_bufsize[1][i] = (allpasses[i] + STEREOSPREAD) * multiplier;
	}

	//------- initialise parameters -------//

	verb_wet      = INITIALWET  * SCALEWET;
	verb_roomsize = INITIALROOM * SCALEROOM + OFFSETROOM;
	verb_dry      = INITIALDRY  * SCALEDRY;
	verb_damp     = INITIALDAMP * SCALEDAMP;
	verb_width    = INITIALWIDTH;

	reverb_update();

	/*===== PROCESS REVERB =====*/

	for (int i = 0; i < n; i += 2)
	{
		float outl  = 0;
		float outr  = 0;
		float input = (buf[i] + buf[i + 1]) * verb_gain;

		/* accumulate comb filters in parallel */
		for (int i = 0; i < NUMCOMBS; i++)
		{
			outl += comb_process(0, i, input);
			outr += comb_process(1, i, input);
		}

		/* feed through allpasses in series */
		for (int i = 0; i < NUMALLPASSES; i++)
		{
			outl = allpass_process(0, i, outl);
			outr = allpass_process(1, i, outr);
		}

		/* replace buffer with output */
		buf[i  ] = outl * verb_wet1 + outr * verb_wet2 + buf[i  ] * verb_dry;
		buf[i+1] = outr * verb_wet1 + outl * verb_wet2 + buf[i+1] * verb_dry;
	}
}