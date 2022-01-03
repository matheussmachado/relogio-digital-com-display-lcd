#define F_CPU 16000000
#define SECAO_HORA 0
#define SECAO_MINUTO 1
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h> //pois é dependencia para a lib LCD
#include <stdio.h>
#include "LCD.h"

/*
 DEFINIÇÕES:
 * HÁ 3 MODOS/STATUS: EXIBIÇÃO, AJUSTE DE HORÁRIO, AJUSTE DE ALARME
 * PB4 -> ALTERA O MODO
 * PC2 -> NOS AJUSTES: ALTERA O "CURSOR" ENTRE HORA E MINUTO
 * PC4 -> NOS AJUSTES: INCREMENTA A VARIÁVEL EM QUE ESTÁ APONTADO O "CURSOR"
 */

// MACROS PARA TRATAMENTO DE BITS NOS REGISTRADORES
#define test_bit(y, bit)    (y & (1 << bit))

// PROTÓTIPOS
ISR(TIMER1_OVF_vect);
ISR(PCINT0_vect);
void executar_modo(int modo);

// VARIÁVEIS GLOBAIS
volatile unsigned int hora = 0;
volatile unsigned int minuto = 0;
volatile unsigned int segundo = 0;
volatile char buffer[9] = "        ";
volatile unsigned int modo = 0;
volatile unsigned int exibir_numero = 0;
volatile unsigned int cursor = SECAO_HORA;
char modos[][13] = {"HORARIO     ", "ALT. ALARME ", "ALT. HORARIO"};

int main(void) {    
  // DISPLAY LCD
  DDRD = 0xFF;
  PORTD = 0x00;
  
  // PUSH BUTTONS
  DDRC = 0x00;
  PORTC = 0b00001100; // habilitando pull up dos push buttons
  DDRB = 0x00;
  PORTB = 0b00010000;
  
  // INTERRUPÇÕES 
  PCICR = 0b00000001; // habilitando interrupção do PORTB
  PCMSK0 = 0b00010000; // habilitando interrupção do pino PB4
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
  executar_modo(modo);
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

ISR(PCINT0_vect) {
  if (!test_bit(PINB, PB4)) {
    modo++;
    if (modo == 3) modo = 0;
    
  }
}

void executar_modo(int modo) {
  cmd_LCD(0x80, 0);
  escreve_LCD(modos[modo]);
  cmd_LCD(0xC0, 0);
  switch (modo) {
    case 0:
      sprintf(buffer, "%.2d:%.2d:%.2d", hora, minuto, segundo);
      break;
    case 1:
      if (!exibir_numero) {
        if (cursor == SECAO_HORA) {
          sprintf(buffer, "  :%.2d:%.2d", minuto, segundo);
        }
        else if (cursor == SECAO_MINUTO) {
          sprintf(buffer, "%.2d:  :%.2d", hora, minuto, segundo);
        }
      }
      else {              
        sprintf(buffer, "%.2d:%.2d:%.2d", hora, minuto, segundo);                               
      }
      exibir_numero = !exibir_numero;
      break;
  } 
  escreve_LCD(buffer);
}
