#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "bsp/board.h"
#include "tusb.h"

#define CAPTURE_CHANNEL 0
#define CAPTURE_DEPTH 32
#define LOG_DEPTH 2048
#define BUFFER_MIN 0    // Set to the lowest expected buffer value
#define BUFFER_MAX 4095 // Set to the highest expected buffer value
#define MIDI_MIN 0      // Lowest MIDI note number
#define MIDI_MAX 127    // Highest MIDI note number

const uint32_t LED_PIN = 25;
const uint32_t fs = 10000;
const uint32_t BENCHMARK_PIN = 2;
static bool ping = true;

/* Ping Pong buffers */
uint16_t cap0[CAPTURE_DEPTH] = {0};
uint16_t cap1[CAPTURE_DEPTH] = {0};
// Processing buffer
uint16_t buffer[CAPTURE_DEPTH] = {0};
uint16_t last_buffer[CAPTURE_DEPTH] = {0};
uint8_t last_note = 0;

int32_t log0[LOG_DEPTH] = {0};
int32_t log1[LOG_DEPTH] = {0};
int32_t *read_logger = log0;
uint32_t log_index = 0;
static bool log_ping = true;
void core1_main();

//--------------------------------------------------------------------+
// MIDI Stack callback (not needed right now)
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
}

void tud_suspend_cb(bool remote_wakeup_en)
{
}

void tud_resume_cb(void)
{
}

// Scaling function to map buffer values to MIDI note numbers
uint8_t buffer_to_midi_note(int32_t value)
{
    // Constrain value to BUFFER_MIN and BUFFER_MAX
    if (value < BUFFER_MIN)
        value = BUFFER_MIN;
    if (value > BUFFER_MAX)
        value = BUFFER_MAX;

    // Map the value from BUFFER_MIN–BUFFER_MAX to MIDI_MIN–MIDI_MAX
    return (uint8_t)(((value - BUFFER_MIN) * (MIDI_MAX - MIDI_MIN)) / (BUFFER_MAX - BUFFER_MIN)) + MIDI_MIN;
}

int main()
{
    uint16_t *dma_buffer = cap1;
    int32_t *write_logger = log1;

    stdio_init_all();
    board_init();
    tusb_init();


    /*
     * GPIO Setup
     */
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(BENCHMARK_PIN);
    gpio_set_dir(BENCHMARK_PIN, GPIO_OUT);
    gpio_put(BENCHMARK_PIN, 0);

    /*
     * ADC Setup
     */
    adc_gpio_init(26 + CAPTURE_CHANNEL);
    adc_gpio_init(26 + 1);
    adc_init();

    adc_select_input(CAPTURE_CHANNEL);
    adc_fifo_setup(
        true,  // Write each completed conversion to the sample FIFO
        true,  // Enable DMA data request (DREQ)
        1,     // DREQ (and IRQ) asserted when at least 1 sample present
        false, // We won't see the ERR bit because of 8 bit reads; disable.
        false  // No need to shift since we want 16 bits
    );
    adc_set_clkdiv(4800); /* Scale down to 10khz */

    sleep_ms(10);

    /*
     * DMA Setup
     */
    // Set up the DMA to start transferring data as soon as it appears in FIFO
    uint dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);

    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);

    // Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&cfg, DREQ_ADC);

    dma_channel_configure(dma_chan, &cfg,
                          cap0,          // dst
                          &adc_hw->fifo, // src
                          CAPTURE_DEPTH, // transfer count
                          true           // start immediately
    );

    adc_run(true);

    multicore_launch_core1(core1_main);

    for (;;)
    {
        dma_channel_wait_for_finish_blocking(dma_chan);

        if (ping)
        {
            dma_channel_configure(dma_chan, &cfg, cap1, &adc_hw->fifo, CAPTURE_DEPTH, true);
            dma_buffer = cap0;
        }
        else
        {
            dma_channel_configure(dma_chan, &cfg, cap0, &adc_hw->fifo, CAPTURE_DEPTH, true);
            dma_buffer = cap1;
        }
        ping = !ping;

        uint8_t const cable_num = 0; // MIDI jack associated with USB endpoint
        uint8_t const channel = 0;   // MIDI channel 1

        // Process the buffer data
        for (size_t i = 0; i < CAPTURE_DEPTH; i++)
        {
            last_buffer[i] = buffer[i];
            buffer[i] = ((int32_t)(dma_buffer[i]) - (2048 - 60)) << 5; // Adjust for resistor variance
            write_logger[log_index + i] = buffer[i];
        }

        // Calculate the average and map to a MIDI note
        int sum = 0;
        for (int i = 0; i < CAPTURE_DEPTH; i++)
            sum += buffer[i];

        uint8_t midi_note = buffer_to_midi_note((int32_t)(sum / CAPTURE_DEPTH));

        // Handle MIDI note on/off messages
        if (midi_note != last_note)
        {
            // Send Note Off for the previous note if it's different
            if (last_note != 0)
            {
                uint8_t note_off[3] = {0x80 | channel, last_note, 0};
                tud_midi_stream_write(cable_num, note_off, 3);
            }

            // Send Note On for the new note
            uint8_t note_on[3] = {0x90 | channel, midi_note, 127};
            tud_midi_stream_write(cable_num, note_on, 3);

            last_note = midi_note; // Update last note
        }

        // GPIO feedback
        if (midi_note == last_note)
        {
            gpio_put(LED_PIN, 1);
            gpio_put(BENCHMARK_PIN, 1);
        }
        else
        {
            gpio_put(LED_PIN, 0);
            gpio_put(BENCHMARK_PIN, 0);
        }

        // Update logging index
        log_index += CAPTURE_DEPTH;
        if (log_index >= LOG_DEPTH)
        {
            log_index = 0; // Reset log index if exceeding LOG_DEPTH
            log_ping = !log_ping;
            write_logger = log_ping ? log0 : log1;
        }
    }

    return 0;
}

/* Core 1 currently only used for dumping data in the
 * debug build */
void core1_main()
{
    //static int32_t *last_read_logger = log1;
    while (true)
    {
        if (tud_cdc_connected())
        {
            // if (last_read_logger != read_logger)
            // {
            //     last_read_logger = read_logger;
            //     for (size_t i = 0; i < LOG_DEPTH; i++)
            //     {
            //         char buffer[32];
            //         snprintf(buffer, sizeof(buffer), "%d\n", read_logger[i]);
            //         tud_cdc_write_str(buffer);
            //     }
            // }
            tud_cdc_write_str("\n");
            tud_cdc_write_flush();
        }
        tud_task();
        sleep_ms(100); // Wait until USB CDC is connected
    }
   
}
