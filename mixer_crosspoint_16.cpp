/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Modified by Macaba
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "mixer_crosspoint_16.h"
#include "utility/dspinst.h"

#if defined(KINETISK)
#define MULTI_UNITYGAIN 65536

static void applyGain(int16_t *data, int32_t mult)
{
	uint32_t *p = (uint32_t *)data;
	const uint32_t *end = (uint32_t *)(data + AUDIO_BLOCK_SAMPLES);

	do {
		uint32_t tmp32 = *p; // read 2 samples from *data
		int32_t val1 = signed_multiply_32x16b(mult, tmp32);
		int32_t val2 = signed_multiply_32x16t(mult, tmp32);
		val1 = signed_saturate_rshift(val1, 16, 0);
		val2 = signed_saturate_rshift(val2, 16, 0);
		*p++ = pack_16b_16b(val2, val1);
	} while (p < end);
}

static void applyGainThenAdd(int16_t *data, const int16_t *in, int32_t mult)
{
	uint32_t *dst = (uint32_t *)data;
	const uint32_t *src = (uint32_t *)in;
	const uint32_t *end = (uint32_t *)(data + AUDIO_BLOCK_SAMPLES);

	if (mult == MULTI_UNITYGAIN) {
		do {
			uint32_t tmp32 = *dst;
			*dst++ = signed_add_16_and_16(tmp32, *src++);
			tmp32 = *dst;
			*dst++ = signed_add_16_and_16(tmp32, *src++);
		} while (dst < end);
	} else {
		do {
			uint32_t tmp32 = *src++; // read 2 samples from *data
			int32_t val1 = signed_multiply_32x16b(mult, tmp32);
			int32_t val2 = signed_multiply_32x16t(mult, tmp32);
			val1 = signed_saturate_rshift(val1, 16, 0);
			val2 = signed_saturate_rshift(val2, 16, 0);
			tmp32 = pack_16b_16b(val2, val1);
			uint32_t tmp32b = *dst;
			*dst++ = signed_add_16_and_16(tmp32, tmp32b);
		} while (dst < end);
	}
}

#elif defined(KINETISL)
#define MULTI_UNITYGAIN 256

static void applyGain(int16_t *data, int32_t mult)
{
	const int16_t *end = data + AUDIO_BLOCK_SAMPLES;

	do {
		int32_t val = *data * mult;
		*data++ = signed_saturate_rshift(val, 16, 0);
	} while (data < end);
}

static void applyGainThenAdd(int16_t *dst, const int16_t *src, int32_t mult)
{
	const int16_t *end = dst + AUDIO_BLOCK_SAMPLES;

	if (mult == MULTI_UNITYGAIN) {
		do {
			int32_t val = *dst + *src++;
			*dst++ = signed_saturate_rshift(val, 16, 0);
		} while (dst < end);
	} else {
		do {
			int32_t val = *dst + ((*src++ * mult) >> 8); // overflow possible??
			*dst++ = signed_saturate_rshift(val, 16, 0);
		} while (dst < end);
	}
}

#endif

void AudioMixerCrosspoint16::update(void)
{
	audio_block_t *in[16], *out=NULL;
	unsigned int channel;
	unsigned int bus;
	unsigned int sample;
	
	//First get audio_blocks to all the input channels as readonly. 
	//Readonly because we'll be using them multiple times so don't want to modify contents.
	for (channel=0; channel < 16; channel++) {
		in[channel] = receiveReadOnly(channel);
	}

	//Now crosspoint mix.
	for(bus=0; bus < 16; bus++) {
		out = allocate();		//get an audio_block that we'll sum the other channels to
		for(sample=0; sample < AUDIO_BLOCK_SAMPLES; sample++){
			out->data[sample] = 0;
		}
		//Using explicit loop instead of memset reduced CPU usage from 53% to 48.50%
		//memset(out->data, 0, AUDIO_BLOCK_SAMPLES * sizeof(int16_t)); 
		if(out){
			for (channel=0; channel < 16; channel++) {
				if (in[channel]) {
					applyGainThenAdd(out->data, in[channel]->data, integerMultipliers[bus][channel]);	
				}
			}
			transmit(out, bus);
			release(out);
		}
	}
	
	//Now release all the input audio_blocks
	for (channel=0; channel < 16; channel++) {
		if(in[channel])
			release(in[channel]);
	}
}


