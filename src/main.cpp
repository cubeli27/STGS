#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>
#include <avr/sleep.h>

volatile uint16_t ontime = 0; 
volatile uint16_t proximity = 0;

void init() {
    DDRB |= (1 << PB1); // (ON SWITCH NPN)
    DDRB |= (1 << PB0); // (IR LED)
    DDRB &= ~(1 << PB2); // Interupt pin
    ADCSRA &= ~(1 << ADEN); // Disable ADC

}

void enter_power_down()
{  
  sei();
  MCUCR |= (1 << SM1);      // Enabling powerdown sleep mode, SM[1:0] bits are written to 10
  MCUCR |= (1 << SE);     //Enabling sleep enable bit SLEEP ENABLE
  sleep_cpu();       //__asm__ __volatile__ ( "sleep" "\n\t" :: ); //Sleep instruction in assembly code
}

void enter_idle()
{
  sei();
  MCUCR &= ~(1 << SM1); // Enabling idle sleep mode, SM[1:0] bits are written to 00
  MCUCR |= (1 << SE);     //Enabling sleep enable bit SLEEP ENABLE
  sleep_cpu();           //sleep_mode(); macro might cause race conditions
}

int rxir()
{ // ADC2 for IR reciever
  ADMUX  |= (1 << MUX1) ; //ADC2(PB4) single ended input
  ADCSRA |= (1 << ADEN); // Enable ADC
  ADCSRA |= (1 << ADSC); // Start conversion
  while (ADCSRA & (1 << ADSC)); // Wait for conversion to finish
  return ADC; // Return ADC value 
}
int duration()
{   // ADC3 for potentiometer
  ADMUX &= ~(1 << REFS1) | (1 << REFS0); // Vcc a Aref
  ADMUX = (1 << MUX0) | (1 << MUX1); // Use ADC3 as input
  ADCSRA |= (1 << ADEN); // Enable ADC
  ADCSRA |= (1 << ADSC); // Start conversion
  while (ADCSRA & (1 << ADSC)); // Wait for conversion to finish
  ADMUX = 0x00; // Clears all bits in ADMUX, effectively disabling specific channel selection
  return ADC; // Return ADC value
}

int main(void)
{
  init(); // Initialize the pins
  ontime = duration(); // done only once during startup
  
  while(1){
    PORTB |= (1 << PB0); // IR Led ON
    _delay_ms(100);
    proximity = rxir();
    if(proximity > 300){
      PORTB |= (1<<PB1);      //Turn ON SWITCH
      PORTB &= ~(1 << PB0);   //Turn OFF IR
      for(uint16_t i=0;i < ontime;i++){ // Duration  //TODO Program Select
        _delay_ms(20);
      }
    }
    PORTB &= ~(1 << PB0);   //Turn OFF IR 
    PORTB &= ~(1 << PB1);   //Turn OFF SWITCH
    _delay_ms(10);
    if (PORTB &(1 << PB2)){ //conditions = external interupt not available 
      //enter_idle();        // TODO
      //_delay_ms(5000);
    }
    else if(~PORTB &(1 << PB2)){ //PB2 is HIGH or LIGHT = external interrupt available  
      GIMSK |= (1 << INT0); // Enable INT0 external interupt
      enter_power_down();        // configuration for power down sleep mode
      GIMSK &= ~(1 << INT0); //disable INT0 otherwise interupts rxir(); i think
    }
  }
  return 0;
}

ISR (INT0_vect)        // Interrupt service routine (WAKE UP)
{ // 50k home, 2k2 lab, ~10k hallway = photoresistor pullup @ PB2=INT0=PB2,  6k8 final (false positive priority)
  cli();                // dissable global interrupts, good practice?
  MCUCR &= ~(1 << SE);     //Clear sleep bit as per datasheet SLEEP DISABLE
  uint8_t timeoutCounter = 0; // Initialize the counter
  while (~PINB & (1 << PB2) && timeoutCounter<100) // Wait for button release
  {
    _delay_ms(1); // Small delay to control the loop speed
    timeoutCounter++; 
  } 
}
