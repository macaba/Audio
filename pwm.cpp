#include "Audio.h"
#include "arm_math.h"



audio_block_t * AudioOutputPWM::block_1st = NULL;
audio_block_t * AudioOutputPWM::block_2nd = NULL;
uint32_t  AudioOutputPWM::block_offset = 0;
bool AudioOutputPWM::update_responsibility = false;
uint8_t AudioOutputPWM::interrupt_count = 0;

DMAMEM uint32_t pwm_dma_buffer[AUDIO_BLOCK_SAMPLES*2];

void AudioOutputPWM::begin(void)
{
	//Serial.println("AudioPwmOutput constructor");
	block_1st = NULL;
	FTM1_SC = 0;
	FTM1_CNT = 0;
	FTM1_MOD = 543;
	FTM1_C0SC = 0x69;  // send DMA request on match
	FTM1_C1SC = 0x28;
	FTM1_SC = FTM_SC_CLKS(1) | FTM_SC_PS(0);
	CORE_PIN3_CONFIG = PORT_PCR_MUX(3) | PORT_PCR_DSE | PORT_PCR_SRE;
	CORE_PIN4_CONFIG = PORT_PCR_MUX(3) | PORT_PCR_DSE | PORT_PCR_SRE;
	FTM1_C0V = 120; // range 120 to 375
	FTM1_C1V = 0;   // range 0 to 255
	for (int i=0; i<256; i+=2) {
		pwm_dma_buffer[i] = 120; // zero must not be used
		pwm_dma_buffer[i+1] = 0;
	}
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;
	DMA_CR = 0;
	DMA_TCD3_SADDR = pwm_dma_buffer;
	DMA_TCD3_SOFF = 4;
	DMA_TCD3_ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2) | DMA_TCD_ATTR_DMOD(4);
	DMA_TCD3_NBYTES_MLNO = 8;
	DMA_TCD3_SLAST = -sizeof(pwm_dma_buffer);
	DMA_TCD3_DADDR = &FTM1_C0V;
	DMA_TCD3_DOFF = 8;
	DMA_TCD3_CITER_ELINKNO = sizeof(pwm_dma_buffer) / 8;
	DMA_TCD3_DLASTSGA = 0;
	DMA_TCD3_BITER_ELINKNO = sizeof(pwm_dma_buffer) / 8;
	DMA_TCD3_CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	DMAMUX0_CHCFG3 = DMAMUX_DISABLE;
	DMAMUX0_CHCFG3 = DMAMUX_SOURCE_FTM1_CH0 | DMAMUX_ENABLE;
	DMA_SERQ = 3;
	update_responsibility = update_setup();
	NVIC_ENABLE_IRQ(IRQ_DMA_CH3);
}

void AudioOutputPWM::update(void)
{
	audio_block_t *block;
	block = receiveReadOnly();
	if (!block) return;
	__disable_irq();
	if (block_1st == NULL) {
		block_1st = block;
		block_offset = 0;
		__enable_irq();
	} else if (block_2nd == NULL) {
		block_2nd = block;
		__enable_irq();
	} else {
		audio_block_t *tmp = block_1st;
		block_1st = block_2nd;
		block_2nd = block;
		block_offset = 0;
		__enable_irq();
		release(tmp);
	}
}

void dma_ch3_isr(void)
{
	int16_t *src;
	uint32_t *dest;
	audio_block_t *block;
	uint32_t saddr, offset;

	saddr = (uint32_t)DMA_TCD3_SADDR;
        DMA_CINT = 3;
	if (saddr < (uint32_t)pwm_dma_buffer + sizeof(pwm_dma_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = &pwm_dma_buffer[AUDIO_BLOCK_SAMPLES];
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = pwm_dma_buffer;
	}
	block = AudioOutputPWM::block_1st;
	offset = AudioOutputPWM::block_offset;

	if (block) {
		src = &block->data[offset];
		for (int i=0; i < AUDIO_BLOCK_SAMPLES/4; i++) {
			uint16_t sample = *src++ + 0x8000;
			uint32_t msb = ((sample >> 8) & 255) + 120;
			uint32_t lsb = sample & 255;
			*dest++ = msb;
			*dest++ = lsb;
			*dest++ = msb;
			*dest++ = lsb;
		}
		offset += AUDIO_BLOCK_SAMPLES/4;
		if (offset < AUDIO_BLOCK_SAMPLES) {
			AudioOutputPWM::block_offset = offset;
		} else {
			AudioOutputPWM::block_offset = 0;
			AudioStream::release(block);
			AudioOutputPWM::block_1st = AudioOutputPWM::block_2nd;
			AudioOutputPWM::block_2nd = NULL;
		}
	} else {
		// fill with silence when no data available
		for (int i=0; i < AUDIO_BLOCK_SAMPLES/4; i++) {
			*dest++ = 248;
			*dest++ = 0;
			*dest++ = 248;
			*dest++ = 0;
		}
	}
	if (AudioOutputPWM::update_responsibility) {
		if (++AudioOutputPWM::interrupt_count >= 4) {
			AudioOutputPWM::interrupt_count = 0;
			AudioStream::update_all();
		}
	}
}




// DMA target is: (registers require 32 bit writes)
//  40039010 Channel 0 Value (FTM1_C0V)
//  40039018 Channel 1 Value (FTM1_C1V)

// TCD:
//  source address = buffer address
//  source offset = 4 bytes
//  attr = no src mod, ssize = 32 bit, dest mod = 16 bytes (4), dsize = 32 bit
//  minor loop byte count = 8
//  source last adjust = -sizeof(buffer)
//  dest address = FTM1_C0V
//  dest address offset = 8
//  citer = sizeof(buffer) / 8    (no minor loop linking)
//  dest last adjust = 0          (dest modulo keeps it ready for more)
//  control:
//    throttling = 0
//    major link to same channel
//    done = 0
//    active = 0
//    majorlink = 1
//    scatter/gather = 0
//    disable request = 0
//    inthalf = 1
//    intmajor = 1
//    start = 0
//  biter = sizeof(buffer) / 8   (no minor loop linking)



