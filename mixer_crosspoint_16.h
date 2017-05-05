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

#ifndef mixercrosspoint16_h_
#define mixercrosspoint16_h_

#include "Arduino.h"
#include "AudioStream.h"

class AudioMixerCrosspoint16 : public AudioStream
{
#if defined(KINETISK)
public:
	AudioMixerCrosspoint16(void) : AudioStream(16, inputQueueArray) {
		for (int i=0; i<16; i++) {
			outputGains[i] = 1.0;
			for(int j=0; j<16; j++) {
				gains[i][j] = 1.0;
				integerMultipliers[i][j] = 65536;
			}
		}
	}
	virtual void update(void);
	void gain(unsigned int bus, unsigned int channel, float gain) {
		if (bus >= 16) return;
		if (channel >= 16) return;
		if (gain > 32767.0f) gain = 32767.0f;
		else if (gain < -32767.0f) gain = -32767.0f;
		gains[bus][channel] = gain;
		integerMultipliers[bus][channel] = gains[bus][channel] * outputGains[bus] * 65536.0f;
	}
	void outputGain(unsigned int bus, float gain) {
		if (bus >= 16) return;
		if (gain > 32767.0f) gain = 32767.0f;
		else if (gain < -32767.0f) gain = -32767.0f;
		outputGains[bus] = gain;
		for (int i=0; i<16; i++) {
			integerMultipliers[bus][i] = gains[bus][i] * outputGains[bus] * 65536.0f;
		}
	}
private:
	float gains[16][16];					//For each bus n, there are x inputs -> gains[n][x]
	float outputGains[16];					//For each bus, there is an output gain
	int32_t integerMultipliers[16][16];		//Computed channel/bus gains with bus output gain mixed in
	audio_block_t *inputQueueArray[16];

#elif defined(KINETISL)
public:
	AudioMixerCrosspoint16(void) : AudioStream(16, inputQueueArray) {
		for (int i=0; i<16; i++) {
			outputGains[i] = 1.0;
			for(int j=0; j<16; j++) {
				gains[i][j] = 1.0;
				integerMultipliers[i][j] = 256
			}
		}
	}
	virtual void update(void);
	void gain(unsigned int bus,unsigned int channel, float gain) {
		if (bus >= 16) return;
		if (channel >= 16) return;
		if (gain > 127.0f) gain = 127.0f;
		else if (gain < -127.0f) gain = -127.0f;
		gains[bus][channel] = gain;
		integerMultipliers[bus][channel] = gains[bus][channel] * outputGains[bus] * 256.0f;
	}
	void outputGain(unsigned int bus, float gain) {
		if (bus >= 16) return;
		if (gain > 127.0f) gain = 127.0f;
		else if (gain < -127.0f) gain = -127.0f;
		outputGains[bus] = gain;
		calculateIntegerMultipliers(bus);
		for (int i=0; i<16; i++) {
			integerMultipliers[bus][i] = gains[bus][i] * outputGains[bus] * 256.0f;
		}
	}
private:
	float gains[16][16];				//For each bus n, there are x inputs -> gains[n][x]
	float outputGains[16];
	int16_t integerMultipliers[16][16];	//Computed channel/bus gains with bus output gain mixed in
	audio_block_t *inputQueueArray[16];
	
#endif
};

#endif
