#include "capture.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "logic_analyzer.pio.h"

// --- Digital Configuration ---
#define DIGITAL_MAX_SAMPLES  (256 * 1024) // 256 KB
#define DIGITAL_PIN_BASE     0
#define DIGITAL_PIN_COUNT    8

// --- Analog Configuration ---
// 4000 samples = 2000 per pin = 10ms of data at 200kSPS
// Adjust this based on your in-situ math / USB polling rate
#define ANALOG_BUFFER_SAMPLES 4000 
#define ANALOG_BUFFER_COUNT   3

static PIO pio = pio0;
static uint sm = 0;
static uint pio_offset;

// Digital uses 8-bit to save massive amounts of RAM
static uint8_t digital_storage[DIGITAL_MAX_SAMPLES];
static uint32_t digital_sample_count = 0;
static volatile bool digital_done_flag = false;
static int digital_dma_channel = -1;

// Analog Triple Buffer setup
static uint16_t analog_storage[ANALOG_BUFFER_COUNT][ANALOG_BUFFER_SAMPLES];
static uint32_t analog_sample_count = ANALOG_BUFFER_SAMPLES;
static int analog_dma_chan_a = -1;
static int analog_dma_chan_b = -1;

// Tracking for the triple buffer
static volatile int dma_a_buf_idx = 0;
static volatile int dma_b_buf_idx = 1;
static volatile bool analog_buf_ready[ANALOG_BUFFER_COUNT] = {false, false, false};
static int main_read_buf_idx = 0; // Handled by main loop

static void dma_irq_handler(void);

void capture_init(void)
{
    // Digital GPIO Init
    for (int i = 0; i < DIGITAL_PIN_COUNT; i++) {
        gpio_init(DIGITAL_PIN_BASE + i);
        gpio_set_dir(DIGITAL_PIN_BASE + i, GPIO_IN);
    }

    // PIO Setup
    pio_offset = pio_add_program(pio, &logic_analyzer_program);
    pio_sm_config c = logic_analyzer_program_get_default_config(pio_offset);
    sm_config_set_in_pins(&c, DIGITAL_PIN_BASE);
    sm_config_set_clkdiv(&c, 1.0f);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_NONE);
    
    // IMPORTANT: Ensure your PIO pushes 8 bits, not 32.
    // sm_config_set_in_shift(&c, true, true, 8); 

    pio_sm_init(pio, sm, pio_offset, &c);
    pio_sm_set_enabled(pio, sm, false);

    // Claim DMA Channels (1 for Digital, 2 for Analog Chaining)
    digital_dma_channel = dma_claim_unused_channel(true);
    analog_dma_chan_a   = dma_claim_unused_channel(true);
    analog_dma_chan_b   = dma_claim_unused_channel(true);

    // ADC Setup
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);

    // Setup IRQ
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

// ==========================================
// DIGITAL SNAPSHOT LOGIC
// ==========================================

void capture_digital_config(uint32_t sample_rate_hz, uint32_t sample_count)
{
    if (sample_count > DIGITAL_MAX_SAMPLES)
        sample_count = DIGITAL_MAX_SAMPLES;

    digital_sample_count = sample_count;

    float div = (float)clock_get_hz(clk_sys) / sample_rate_hz;
    pio_sm_set_clkdiv(pio, sm, div);

    dma_channel_config dc = dma_channel_get_default_config(digital_dma_channel);
    channel_config_set_transfer_data_size(&dc, DMA_SIZE_8); // 8-bit transfers
    channel_config_set_read_increment(&dc, false);
    channel_config_set_write_increment(&dc, true);
    channel_config_set_dreq(&dc, pio_get_dreq(pio, sm, false));

    dma_channel_configure(
        digital_dma_channel,
        &dc,
        digital_storage,
        &pio->rxf[sm],
        digital_sample_count,
        false
    );

    dma_channel_set_irq0_enabled(digital_dma_channel, true);
}

void capture_digital_start(void)
{
    digital_done_flag = false;
    pio_sm_clear_fifos(pio, sm);
    pio_sm_restart(pio, sm);
    dma_channel_set_write_addr(digital_dma_channel, digital_storage, false);
    dma_channel_set_trans_count(digital_dma_channel, digital_sample_count, false);
    pio_sm_set_enabled(pio, sm, true);
    dma_channel_start(digital_dma_channel);
}

bool capture_digital_done(void) { return digital_done_flag; }
uint8_t* capture_digital_get_buffer(void) { return digital_storage; }
uint32_t capture_digital_get_sample_count(void) { return digital_sample_count; }

// ==========================================
// ANALOG LIVE STREAM LOGIC (TRIPLE BUFFER)
// ==========================================

void capture_analog_config(uint32_t sample_rate_hz)
{
    // ADC Round Robin for 2 pins (Channel 0 and 1)
    adc_fifo_setup(true, true, 1, false, false);
    adc_set_round_robin((1 << 0) | (1 << 1));

    float div = (float)clock_get_hz(clk_adc) / sample_rate_hz;
    adc_set_clkdiv(div);

    // Config DMA A
    dma_channel_config dc_a = dma_channel_get_default_config(analog_dma_chan_a);
    channel_config_set_transfer_data_size(&dc_a, DMA_SIZE_16);
    channel_config_set_read_increment(&dc_a, false);
    channel_config_set_write_increment(&dc_a, true);
    channel_config_set_dreq(&dc_a, DREQ_ADC);
    channel_config_set_chain_to(&dc_a, analog_dma_chan_b); // Chain to B

    dma_channel_configure(
        analog_dma_chan_a, &dc_a,
        analog_storage[0], // Start at buffer 0
        &adc_hw->fifo,
        ANALOG_BUFFER_SAMPLES,
        false
    );

    // Config DMA B
    dma_channel_config dc_b = dma_channel_get_default_config(analog_dma_chan_b);
    channel_config_set_transfer_data_size(&dc_b, DMA_SIZE_16);
    channel_config_set_read_increment(&dc_b, false);
    channel_config_set_write_increment(&dc_b, true);
    channel_config_set_dreq(&dc_b, DREQ_ADC);
    channel_config_set_chain_to(&dc_b, analog_dma_chan_a); // Chain back to A

    dma_channel_configure(
        analog_dma_chan_b, &dc_b,
        analog_storage[1], // Start at buffer 1
        &adc_hw->fifo,
        ANALOG_BUFFER_SAMPLES,
        false
    );

    dma_channel_set_irq0_enabled(analog_dma_chan_a, true);
    dma_channel_set_irq0_enabled(analog_dma_chan_b, true);
}

void capture_analog_start(void)
{
    // Reset state
    dma_a_buf_idx = 0;
    dma_b_buf_idx = 1;
    main_read_buf_idx = 0;
    
    for(int i=0; i<ANALOG_BUFFER_COUNT; i++) {
        analog_buf_ready[i] = false;
    }

    adc_fifo_drain();
    adc_run(true);

    // Start DMA A (which will auto-chain to B when done)
    dma_channel_start(analog_dma_chan_a);
}

// Main loop calls this to get the next ready buffer for math/USB
uint16_t* capture_analog_get_ready_buffer(void)
{
    if (analog_buf_ready[main_read_buf_idx]) {
        return analog_storage[main_read_buf_idx];
    }
    return NULL; // None ready yet
}

// Main loop calls this when finished sending data over USB
void capture_analog_release_buffer(void)
{
    analog_buf_ready[main_read_buf_idx] = false;
    main_read_buf_idx = (main_read_buf_idx + 1) % ANALOG_BUFFER_COUNT;
}

// ==========================================
// INTERRUPT HANDLER
// ==========================================

static void dma_irq_handler(void)
{
    uint32_t status = dma_hw->ints0;

    // --- DIGITAL DONE ---
    if (status & (1u << digital_dma_channel)) {
        dma_hw->ints0 = 1u << digital_dma_channel;
        pio_sm_set_enabled(pio, sm, false);
        digital_done_flag = true;
    }

    // --- ANALOG DMA A DONE ---
    if (status & (1u << analog_dma_chan_a)) {
        dma_hw->ints0 = 1u << analog_dma_chan_a;
        
        // Mark the buffer DMA A just finished as ready
        analog_buf_ready[dma_a_buf_idx] = true;
        
        // Update DMA A to write to the next available buffer in the rotation
        dma_a_buf_idx = (dma_b_buf_idx + 1) % ANALOG_BUFFER_COUNT;
        dma_channel_set_write_addr(analog_dma_chan_a, analog_storage[dma_a_buf_idx], false);
    }

    // --- ANALOG DMA B DONE ---
    if (status & (1u << analog_dma_chan_b)) {
        dma_hw->ints0 = 1u << analog_dma_chan_b;
        
        // Mark the buffer DMA B just finished as ready
        analog_buf_ready[dma_b_buf_idx] = true;
        
        // Update DMA B to write to the next available buffer in the rotation
        dma_b_buf_idx = (dma_a_buf_idx + 1) % ANALOG_BUFFER_COUNT;
        dma_channel_set_write_addr(analog_dma_chan_b, analog_storage[dma_b_buf_idx], false);
    }
}
