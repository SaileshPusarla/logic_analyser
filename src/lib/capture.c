#include "capture.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"

#include "logic_analyzer.pio.h"

#define SAMPLE_PIN_BASE 2
#define SAMPLE_PIN_COUNT 8

static uint32_t buffer32[CAPTURE_BUFFER_SIZE / 4];

static PIO pio = pio0;
static uint sm = 0;

static int dma_chan;

static trigger_config_t trigger;

bool capture_init(trigger_config_t trig)
{
    trigger = trig;

    uint offset = pio_add_program(pio, &logic_analyzer_program);

    pio_sm_config c = logic_analyzer_program_get_default_config(offset);

    sm_config_set_in_pins(&c, SAMPLE_PIN_BASE);

    float clkdiv = (float)clock_get_hz(clk_sys) / CAPTURE_SAMPLE_RATE;
    sm_config_set_clkdiv(&c, clkdiv);

    pio_sm_init(pio, sm, offset, &c);

    for (int i = 0; i < SAMPLE_PIN_COUNT; i++)
    {
        uint pin = SAMPLE_PIN_BASE + i;

        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);

        pio_gpio_init(pio, pin);
    }

    pio_sm_set_consecutive_pindirs(
        pio,
        sm,
        SAMPLE_PIN_BASE,
        SAMPLE_PIN_COUNT,
        false
    );

    dma_chan = dma_claim_unused_channel(true);

    dma_channel_config cfg =
        dma_channel_get_default_config(dma_chan);

    channel_config_set_transfer_data_size(
        &cfg,
        DMA_SIZE_32
    );

    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);

    channel_config_set_dreq(
        &cfg,
        pio_get_dreq(pio, sm, false)
    );

    dma_channel_configure(
        dma_chan,
        &cfg,
        buffer32,
        &pio->rxf[sm],
        CAPTURE_BUFFER_SIZE / 4,
        false
    );

    return true;
}

bool capture_start(void)
{
    /* reset write address and transfer count */
    dma_channel_set_write_addr(dma_chan, buffer32, false);

    dma_channel_set_trans_count(
        dma_chan,
        CAPTURE_BUFFER_SIZE / 4,
        false
    );

    dma_channel_start(dma_chan);

    pio_sm_set_enabled(pio, sm, true);

    return true;
}

bool capture_wait(void)
{
    dma_channel_wait_for_finish_blocking(dma_chan);
    pio_sm_set_enabled(pio, sm, false);
    return true;
}

capture_buffer_t capture_get_buffer(void)
{
    capture_buffer_t out;

    out.data = (uint8_t*)buffer32;
    out.length = CAPTURE_BUFFER_SIZE;
    out.sample_rate = CAPTURE_SAMPLE_RATE;
    out.channels = CAPTURE_CHANNELS;

    return out;
}
