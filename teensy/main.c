#include <ADC.h>
#include <MIDI.h>
#include <Arduino.h>

#define CAPTURE_CHANNEL 0
#define CAPTURE_DEPTH 32
#define LOG_DEPTH 2048
#define BUFFER_MIN 0    // Set to the lowest expected buffer value
#define BUFFER_MAX 4095 // Set to the highest expected buffer value
#define MIDI_MIN 0      // Lowest MIDI note number
#define MIDI_MAX 127    // Highest MIDI note number

const uint32_t LED_PIN = 13; // Teensy onboard LED
const uint32_t ADC_GPIO_START_PIN = A0; // Analog input pin
const uint32_t BENCHMARK_PIN = 2;       // Example GPIO pin

static bool ping = true;
uint16_t buffer[CAPTURE_DEPTH] = {0};
uint16_t last_buffer[CAPTURE_DEPTH] = {0};
uint8_t last_note = 0;

int32_t log0[LOG_DEPTH] = {0};
int32_t log1[LOG_DEPTH] = {0};
int32_t *read_logger = log0;
uint32_t log_index = 0;
static bool log_ping = true;

/* Teensy ADC setup */
ADC *adc = new ADC(); // ADC object
MIDI_CREATE_DEFAULT_INSTANCE(); // MIDI object

#define RA_WINDOW_SIZE 128
int32_t ra_buffer[RA_WINDOW_SIZE] = {0};
int32_t ra_sum = 0;
size_t ra_index = 0;
size_t ra_count = 0;

int32_t rolling_average(int32_t new_value) {
    if (ra_count >= RA_WINDOW_SIZE) {
        ra_sum -= ra_buffer[ra_index];
    } else {
        ra_count++;
    }
    ra_buffer[ra_index] = new_value;
    ra_sum += new_value;
    ra_index = (ra_index + 1) % RA_WINDOW_SIZE;
    return ra_sum / ra_count;
}

uint8_t buffer_to_midi_note(int32_t value) {
    if (value < BUFFER_MIN) value = BUFFER_MIN;
    if (value > BUFFER_MAX) value = BUFFER_MAX;
    return (uint8_t)(((value - BUFFER_MIN) * (MIDI_MAX - MIDI_MIN)) / (BUFFER_MAX - BUFFER_MIN)) + MIDI_MIN;
}

void setup() {
    Serial.begin(9600);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BENCHMARK_PIN, OUTPUT);
    
    adc->adc0->setAveraging(16); // Averaging to smooth readings
    adc->adc0->setResolution(12); // 12-bit resolution
    adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::HIGH_SPEED);
    adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED);
}

void loop() {
    int32_t smoothed_sum = 0;

    for (size_t i = 0; i < CAPTURE_DEPTH; i++) {
        int32_t raw_value = adc->analogRead(ADC_GPIO_START_PIN); // Read analog input
        int32_t adjusted_value = ((int32_t)(raw_value) - (2048 - 60)) << 5;
        buffer[i] = rolling_average(adjusted_value);
        smoothed_sum += buffer[i];
    }

    uint8_t midi_note = buffer_to_midi_note(smoothed_sum / CAPTURE_DEPTH);

    if (midi_note != last_note) {
        if (last_note != 0) {
            MIDI.sendNoteOff(last_note, 0, 1);
        }
        MIDI.sendNoteOn(midi_note, 127, 1);
        last_note = midi_note;
    }

    if (midi_note == last_note) {
        digitalWrite(LED_PIN, HIGH);
        digitalWrite(BENCHMARK_PIN, HIGH);
    } else {
        digitalWrite(LED_PIN, LOW);
        digitalWrite(BENCHMARK_PIN, LOW);
    }

    delay(10); // Adjust delay as needed
}
