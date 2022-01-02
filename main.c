#define F_CPU 16000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h> //pois é dependencia para a lib LCD
#include <stdio.h>
#include "LCD.h"

// PROTÓTIPO DE MACROS PARA INTERRUPÇÕES
ISR(TIMER1_OVF_vect);

// MACROS PARA TRATAMENTO DE BITS NOS REGISTRADORES
#define test_bit(y, bit)    (y & (1 << bit))

// VARIÁVEIS GLOBAIS
volatile unsigned int hora = 0;
volatile unsigned int minuto = 0;
volatile unsigned int segundo = 0;
volatile char buffer[9] = "        ";


int main(void) {    
  // DISPLAY LCD
  DDRD = 0xFF;
  PORTD = 0x00;
  
  // PUSH BUTTONS
  DDRC = 0b00000000;
  PORTC = 0b00001100; // habilitando pull up dos push buttons
  
  // INTERRUPÇÕES  
  TCCR1B = 0b00000101; // prescaler 1024
  TIMSK1 = 0b00000001; // habilitando interrupção do timer por overflow
  TCNT1 = 49911; // contagem para overflow em 1 seg.
  sei(); // habilitando chave geral de interrupções
  
  // LCD    
  inic_LCD_4bits();
  
  while (1) {    
  }
}

ISR(TIMER1_OVF_vect) {
  TCNT1 = 49911;
  sprintf(buffer, "%.2d:%.2d:%.2d", hora, minuto, segundo);
  cmd_LCD(0x80, 0);
  escreve_LCD(buffer);
  segundo++;
  if (segundo == 60) {
    segundo = 0;
    minuto++;
    if (minuto == 60) {
      minuto = 0;
      hora++;
      if (hora == 60) {
        hora = 0;
      }
    }    
  }  
}