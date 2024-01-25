/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/audio_i2s.h"
#include "audio_i2s.pio.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"


CU_REGISTER_DEBUG_PINS(audio_timing)

// ---- select at most one ---
//CU_SELECT_DEBUG_PINS(audio_timing)


#if PICO_AUDIO_I2S_MONO_OUTPUT
#define i2s_dma_configure_size DMA_SIZE_16
#else
#define i2s_dma_configure_size DMA_SIZE_32
#endif

#define audio_pio __CONCAT(pio, PICO_AUDIO_I2S_PIO)
#define GPIO_FUNC_PIOx __CONCAT(GPIO_FUNC_PIO, PICO_AUDIO_I2S_PIO)
#define DREQ_PIOx_TX0 __CONCAT(__CONCAT(DREQ_PIO, PICO_AUDIO_I2S_PIO), _TX0)
#define DREQ_PIOx_RX0 __CONCAT(__CONCAT(DREQ_PIO, PICO_AUDIO_I2S_PIO), _RX0)

struct {
    uint32_t freq;
    uint8_t pio_sm;
    uint8_t dma_channel;
} shared_state;

struct {
    uint32_t freq;
    uint8_t pio_sm;
    uint8_t dma_channel;
} shared_state2;

bufring_t *bufring2;

void audioi2sconstuff(bufring_t *bufring1) {
bufring2=bufring1;
}

uint32_t buflends[8192];
void audioi2sconstuff2() {
    uint32_t divider = (270000000 * 2 / 48000) - ((bufring2->len - 16)/2);
    buflends[bufring2->index1] = divider;
    pio_sm_set_clkdiv_int_frac(audio_pio, shared_state.pio_sm, divider >> 8u, divider & 0xffu);
}

bufring_t *bufring4;

void audioi2sconstuff3(bufring_t *bufring3) {
bufring4=bufring3;
}

static void __isr __time_critical_func(audio_i2s_dma_irq_handler)();

const audio_format_t *audio_i2s_setup(const audio_format_t *intended_audio_format,
                                               const audio_i2s_config_t *config) {
    uint func = GPIO_FUNC_PIOx;
    gpio_set_function(config->data_pin, func);
    gpio_set_function(config->clock_pin_base, func);
    gpio_set_function(config->clock_pin_base + 1, func);

    uint8_t sm = shared_state.pio_sm = config->pio_sm;
    pio_sm_claim(audio_pio, sm);

    uint offset = pio_add_program(audio_pio, &audio_i2s_program);

    audio_i2s_program_init(audio_pio, sm, offset, config->data_pin, config->clock_pin_base);

    __mem_fence_release();
    uint8_t dma_channel = config->dma_channel;
    dma_channel_claim(dma_channel);

    shared_state.dma_channel = dma_channel;

    dma_channel_config dma_config = dma_channel_get_default_config(dma_channel);

    channel_config_set_dreq(&dma_config,
                            DREQ_PIOx_TX0 + sm
    );
    channel_config_set_transfer_data_size(&dma_config, i2s_dma_configure_size);
    dma_channel_configure(dma_channel,
                          &dma_config,
                          &audio_pio->txf[sm],  // dest
                          NULL, // src
                          0, // count
                          false // trigger
    );

    irq_add_shared_handler(DMA_IRQ_0 + PICO_AUDIO_I2S_DMA_IRQ, audio_i2s_dma_irq_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    dma_irqn_set_channel_enabled(PICO_AUDIO_I2S_DMA_IRQ, dma_channel, 1);
    return intended_audio_format;
}

const audio_format_t *audio_i2s_in_setup(const audio_format_t *intended_audio_format,
                                               const audio_i2s_config_t *config) {
    uint func = GPIO_FUNC_PIOx;
    gpio_set_function(config->data_pin, func);
    gpio_set_function(config->clock_pin_base, func);
    gpio_set_function(config->clock_pin_base + 1, func);

    uint8_t sm = shared_state2.pio_sm = config->pio_sm;
    pio_sm_claim(audio_pio, sm);

    uint offset = pio_add_program(audio_pio, &audio_i2s_program);

    audio_i2s_in_program_init(audio_pio, sm, offset, config->data_pin, config->clock_pin_base);

    __mem_fence_release();
    uint8_t dma_channel = config->dma_channel;
    dma_channel_claim(dma_channel);

    shared_state2.dma_channel = dma_channel;

    dma_channel_config dma_config = dma_channel_get_default_config(dma_channel);

    channel_config_set_dreq(&dma_config,
                            DREQ_PIOx_RX0 + sm
    );
    channel_config_set_transfer_data_size(&dma_config, i2s_dma_configure_size);
    dma_channel_configure(dma_channel,
                          &dma_config,
                          NULL,  // dest
                          &audio_pio->rxf[sm], // src
                          0, // count
                          false // trigger
    );

    return intended_audio_format;
}

static void update_pio_frequency(uint32_t sample_freq) {
    uint32_t system_clock_frequency = clock_get_hz(clk_sys);
    assert(system_clock_frequency < 0x40000000);
    uint32_t divider = system_clock_frequency * 2 / sample_freq; // avoid arithmetic overflow
    assert(divider < 0x1000000);
    pio_sm_set_clkdiv_int_frac(audio_pio, shared_state.pio_sm, divider >> 8u, divider & 0xffu);
    shared_state.freq = sample_freq;
}

static inline void audio_start_dma_transfer() {
while (bufring2->corelock == 1) {tight_loop_contents();}
bufring2->corelock = 2;
    if (bufring2->len < 2) {
bufring2->corelock = 0;
        // just play some silence
/*
irq_set_enabled(DMA_IRQ_0 + PICO_AUDIO_I2S_DMA_IRQ, false);
pio_sm_set_enabled(audio_pio, shared_state.pio_sm, false);
while (bufring2->len < 2) {tight_loop_contents();}
irq_set_enabled(DMA_IRQ_0 + PICO_AUDIO_I2S_DMA_IRQ, true);
pio_sm_set_enabled(audio_pio, shared_state.pio_sm, true);
*/
        static uint32_t zero;
        dma_channel_config c = dma_get_channel_config(shared_state.dma_channel);
        channel_config_set_read_increment(&c, false);
        dma_channel_set_config(shared_state.dma_channel, &c, false);
        dma_channel_transfer_from_buffer_now(shared_state.dma_channel, &zero, 2);
        return;
    }
    dma_channel_config c = dma_get_channel_config(shared_state.dma_channel);
    channel_config_set_read_increment(&c, true);
    dma_channel_set_config(shared_state.dma_channel, &c, false);
    dma_channel_transfer_from_buffer_now(shared_state.dma_channel, bufring2->buf+bufring2->index1, 2);
    buflends[bufring2->index1+1] = bufring2->len;
    bufring2->len = bufring2->len - 2;
    bufring2->index1 = (bufring2->index1 + 2) % (1024-32);
bufring2->corelock = 0;
    return;
}

static inline void audio_in_start_dma_transfer() {
//    while (bufring4.len > 1024-32-2) {tight_loop_contents();}
while (bufring4->corelock == 2) {tight_loop_contents();}
bufring4->corelock = 1;
    dma_channel_config c = dma_get_channel_config(shared_state2.dma_channel);
    channel_config_set_write_increment(&c, true);
    dma_channel_set_config(shared_state2.dma_channel, &c, false);
    dma_channel_transfer_to_buffer_now(shared_state2.dma_channel, bufring4->buf+bufring4->index1, 2);
    buflends[bufring4->index1+1] = bufring4->len;
    bufring4->len = bufring4->len + 2;
    bufring4->index = (bufring4->index + 2) % (1024-32);
bufring4->corelock = 0;
    return;
}

// irq handler for DMA
void __isr __time_critical_func(audio_i2s_dma_irq_handler)() {
#if PICO_AUDIO_I2S_NOOP
    assert(false);
#else
    uint dma_channel = shared_state.dma_channel;
    if (dma_irqn_get_channel_status(PICO_AUDIO_I2S_DMA_IRQ, dma_channel)) {
        dma_irqn_acknowledge_channel(PICO_AUDIO_I2S_DMA_IRQ, dma_channel);
        audio_start_dma_transfer();
    }
#endif
}

static bool audio_enabled;

void audio_i2s_set_enabled(bool enabled) {
    if (enabled != audio_enabled) {
#ifndef NDEBUG
        if (enabled)
        {
            puts("Enabling PIO I2S audio\n");
            printf("(on core %d\n", get_core_num());
    update_pio_frequency(48000);
        }
#endif
        irq_set_enabled(DMA_IRQ_0 + PICO_AUDIO_I2S_DMA_IRQ, enabled);

        if (enabled) {
            audio_start_dma_transfer();
        }

        pio_sm_set_enabled(audio_pio, shared_state.pio_sm, enabled);

        audio_enabled = enabled;
    }
}
