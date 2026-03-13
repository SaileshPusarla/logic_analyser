#include "capture.h"
#include "logic_analyzer.pio.h"
#include "hardware/clocks.h" // <--- Fixes clock_get_hz and clk_sys
#include "hardware/irq.h"

static uint32_t capture_storage[CAPTURE_BUFFER_SIZE_BYTES / 4];
static int dma_chan;
static PIO _pio;
static uint _sm;
static volatile bool transfer_complete = false;

void __isr dma_handler() {
    dma_hw->ints0 = 1u << dma_chan;
    transfer_complete = true;
}

void capture_init(PIO pio, uint sm, float freq_hz) {
    _pio = pio;
    _sm = sm;

    uint offset = pio_add_program(pio, &logic_analyzer_program);
    pio_sm_config c = logic_analyzer_program_get_default_config(offset);
    
    sm_config_set_in_pins(&c, CAPTURE_PIN_BASE);
    for(int i = 0; i < CAPTURE_PIN_COUNT; i++) {
        pio_gpio_init(pio, CAPTURE_PIN_BASE + i);
    }
    
    sm_config_set_in_shift(&c, true, true, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);

    // clock_get_hz(clk_sys) now works with hardware/clocks.h included
    float div = (float)clock_get_hz(clk_sys) / freq_hz;
    sm_config_set_clkdiv(&c, div);

    pio_sm_init(pio, sm, offset, &c);

    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config dc = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&dc, false);
    channel_config_set_write_increment(&dc, true);
    channel_config_set_transfer_data_size(&dc, DMA_SIZE_32);
    channel_config_set_dreq(&dc, pio_get_dreq(pio, sm, false));

    dma_channel_configure(
        dma_chan, &dc,
        capture_storage,
        &pio->rxf[sm],
        CAPTURE_BUFFER_SIZE_BYTES / 4,
        false
    );

    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

void capture_start() {
    transfer_complete = false;
    dma_channel_set_trans_count(dma_chan, CAPTURE_BUFFER_SIZE_BYTES / 4, true);
    pio_sm_set_enabled(_pio, _sm, true);
}

// Added for stream.c compatibility
void capture_wait() {
    while (!transfer_complete) {
        tight_loop_contents();
    }
}

bool capture_is_done() {
    return transfer_complete;
}

// Returns the struct expected by stream.c
capture_buffer_t capture_get_buffer() {
    capture_buffer_t buf;
    buf.data = (uint8_t*)capture_storage;
    buf.length = CAPTURE_BUFFER_SIZE_BYTES;
    return buf;
}
