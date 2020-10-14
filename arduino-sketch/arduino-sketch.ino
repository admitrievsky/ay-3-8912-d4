#include "defines.h"
#include "music_data.h"
#include "ay_channel.h"
#include "music_state.h"

Channel channel_a, channel_b, channel_c;
MusicState state;

ISR(TIMER2_OVF_vect) {
    static uint8_t tick = 0;
  // will be called 31373 times per second
    OCR2B = (uint8_t)(channel_a.render() + channel_b.render() + channel_c.render()) + 128;

    digitalWrite(LED_BUILTIN, channel_a.get_volume() > (tick & 0x0f));
    tick ++;
}

void setup() {
  pinMode(3, OUTPUT); // Will use OC2B, digital pin #3, PD3

  TCCR2A = 0;
  TCCR2B = 0;
  TCCR2A |= (1 << WGM00); // set phase correct PWM mode
  TCCR2A |= (1 << COM2B1); // clear OC2B on compare match when up-counting. Set OC2B on compare match when down-counting.
  TCCR2B |= (1 << CS00); // set pwm frequency to 31373 Hz

  TIMSK2 |= (1 << TOIE2); // enable overflow interrupts

  Channel::init_tone_period_table();

  MusicData music_data = get_music_data_1();
  state.set_data(&music_data);

  Serial.begin(9600);
}

void loop() {
  static uint32_t last_managed = 0;
  if ( (millis() - last_managed) > 20 ) {
    last_managed = millis();
    state.next(channel_a, channel_b, channel_c);

    if (channel_c.get_is_noise_enabled())
      UDR0 = 0; // it's not necessary to wait until transmission finishes, we only wanna flash the led
  }
}
