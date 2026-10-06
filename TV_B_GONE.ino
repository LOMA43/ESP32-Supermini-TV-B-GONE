#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

// --- TV-B-Gone Database Macros ---
#define NUM_ELEM(x) (sizeof(x) / sizeof((x)[0]))
#define freq_to_timerval(x) (8000000 / ((x) * 2))

struct IrCode {
  uint8_t freq;
  uint8_t num_pairs;
  uint8_t bitcompression;
  const uint16_t *timer_val;
  const uint8_t *codes;
};

// Ensure WORLD_IR_CODES.h is in the same folder as this sketch
#include "WORLD_IR_CODES.h" 

// --- Pin Definitions (ESP32-C3 SuperMini) ---
const uint16_t IR_LED_PIN = 10;  
const uint16_t TRIGGER_PIN = 8;  

IRsend irsend(IR_LED_PIN);
uint16_t rawData[300]; 

bool runEU = true;  
bool runNA = true; 

void setup() {
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  irsend.begin();
}

void loop() {
  // Fire and Forget: Press once to trigger
  if (digitalRead(TRIGGER_PIN) == LOW) {
    
    delay(50); // Debounce
    while (digitalRead(TRIGGER_PIN) == LOW) { delay(10); } // Wait for release

    if (runEU) fireCodes(EUpowerCodes, num_EUcodes);
    if (runNA) fireCodes(NApowerCodes, num_NAcodes);
  }
  delay(100);
}

void fireCodes(const IrCode* const* powerCodes, uint8_t numCodes) {
  for (uint8_t i = 0; i < numCodes; i++) {
    // Sequence Cancellation: Press again to stop early
    if (digitalRead(TRIGGER_PIN) == LOW) {
      delay(250); 
      return; 
    }

    const IrCode *code_ptr = (const IrCode *)powerCodes[i];
    uint8_t freq = code_ptr->freq;
    uint8_t num_pairs = code_ptr->num_pairs;
    uint8_t bitcompression = code_ptr->bitcompression;
    const uint16_t *time_ptr = (const uint16_t *)code_ptr->timer_val;

    uint8_t bit_index = 0;
    uint8_t byte_index = 0;
    
    for (uint16_t k = 0; k < num_pairs * 2; k++) {
      uint16_t timer_index = 0;
      for (uint8_t b = 0; b < bitcompression; b++) {
        uint8_t bit = (code_ptr->codes[byte_index] >> bit_index) & 1;
        timer_index |= (bit << b);
        bit_index++;
        if (bit_index == 8) {
          bit_index = 0;
          byte_index++;
        }
      }
      rawData[k] = time_ptr[timer_index] * 10;
    }

    if (freq != 0) {
      uint16_t actual_freq = 8000000 / (freq * 2); 
      irsend.sendRaw(rawData, num_pairs * 2, actual_freq);
    } else {
      irsend.sendRaw(rawData, num_pairs * 2, 38000); 
    }
    
    delay(250); 
  }
}