//=================//
//  simple filter  //
//=================//

#define FL_SAMPLE_RATE 44100

void filter_process(float *input_buffer, uint32_t input_length, uint8_t filter_type, float wet_mix, float cutoff_frequency)
{
	// the classic filter special ingredients
	float RC    = 1.0 / (cutoff_frequency * 2 * 3.14);
	float dt    = 1.0 / FL_SAMPLE_RATE;
	float alpha = dt  / ( RC + dt);

	float *filter_buffer = (float*)malloc(input_length * sizeof(float));

	filter_buffer[0] = input_buffer[0];
	filter_buffer[1] = input_buffer[1];
	
	// lowpass filter
	if (filter_type == 0)
	{
		for(uint32_t i = 2; i < input_length; i += 2)
		{
			filter_buffer[i]   = filter_buffer[i-2] + (alpha * (input_buffer[i] - filter_buffer[i-2]));
			filter_buffer[i+1] = filter_buffer[i-1] + (alpha * (input_buffer[i+1] - filter_buffer[i-1]));
		}
	}
	// hipass filter
	else if (filter_type == 1)
	{
		for(uint32_t i = 2; i < input_length; i += 2)
		{
			filter_buffer[i]   = alpha * (filter_buffer[i-2] + input_buffer[i] - input_buffer[i-2]);
			filter_buffer[i+1] = alpha * (filter_buffer[i-1] + input_buffer[i+1] - input_buffer[i-1]);
		}
	}

	for(uint32_t i = 0; i < input_length; i ++)
		input_buffer[i] = (input_buffer[i] * (1.0 - wet_mix)) + filter_buffer[i] * wet_mix;
}