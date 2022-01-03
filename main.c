#define F_CPU 16000000
#define SECAO_HORA 0
#define SECAO_MINUTO 1
#define EXIBIR_HORARIO 0
#define ALTERAR_ALARME 1
#define ALTERAR_HORARIO 2
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

/*
 TAREFAS:
 * - IMPLEMENTAR O ALARME
 */

// MACROS PARA TRATAMENTO DE BITS NOS REGISTRADORES
#define test_bit(y, bit)    (y & (1 << bit))

// PROTÓTIPOS
ISR(TIMER0_OVF_vect);
ISR(TIMER1_OVF_vect);
ISR(PCINT0_vect);
ISR(PCINT1_vect);
void exibir_modo(int modo);
void blink_cursor();
void tratamento_de_horario(volatile unsigned int *hora, volatile unsigned int *minuto);
void atualizar_exibicao(volatile unsigned int hora, volatile unsigned int minuto);

// VARIÁVEIS GLOBAIS
volatile unsigned int hora = 0, minuto = 0, segundo = 0;
volatile unsigned int hora_exib = 0, minuto_exib = 0;
volatile unsigned int hora_alarme = 0, minuto_alarme = 0;
volatile unsigned int hora_tmp, minuto_tmp;
volatile char buffer[9] = "        ";
volatile unsigned int modo = 0;
volatile unsigned int exibir_numero = 0;
volatile unsigned char cursor = SECAO_HORA;
volatile unsigned int contador_timer0 = 0;
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
  PCICR = 0b00000011; // habilitando interrupção do PORTB e PORTC
  PCMSK0 = 0b00010000; // habilitando interrupção do pino PB4
  PCMSK1 = 0b00001100;
  TCCR0B = 0b00000101; // prescaler 1024 do timer 0
  TIMSK0 = 0b00000001; // habilitando interrupção do timer 0 por overflow
  TCCR1B = 0b00000101;  
  TIMSK1 = 0b00000001;
  TCNT0 = 100; // contagem para overflow em 10 us.
  TCNT1 = 49911; // contagem para overflow em 1 s.
  sei(); // habilitando chave geral de interrupções
  
  // LCD    
  inic_LCD_4bits();
  hora = 23;
  hora_alarme = 23;
  minuto_alarme = 55;
  minuto = 55;
  
  while (1) {
  }
}

ISR(TIMER0_OVF_vect) {
  // DESCRIÇÃO: CONTROLE DA TAXA DE EXIBIÇÃO DO MODO -> A CADA 0,5 SEGUNDOS
  TCNT0 = 100;
  contador_timer0++;
  if (contador_timer0 == 50) {
    contador_timer0 = 0; 
    exibir_modo(modo);
  }
}

ISR(TIMER1_OVF_vect) {
  // DESCRIÇÃO: CONTROLE PARA INCREMENTO DO HORÁRIO
  TCNT1 = 49911;    
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
  // DESCRIÇÃO: CONTROLE DO ESTADO DE QUAL MODO SERÁ EXIBIDO
  if (!test_bit(PINB, PB4)) {
    modo++;
    if (modo == 3) modo = 0;
    cursor = SECAO_HORA;
  }
}

ISR(PCINT1_vect) {
  // DESCRIÇÃO: CONTROLE DO MAPEAMETO DO CURSOR PISCANTE E DO INCREMENTO DO
  //            NÚMERO DA SEÇÃO CUJO CURSOR ESTÁ APONTANDO
  if (!test_bit(PINC, PC2)) {
    cursor = !cursor; // seu mapeamento é entre os números 0 e 1      
  }
  if (!test_bit(PINC, PC3)) {
    hora_tmp = minuto_tmp = 0;
    if (cursor == SECAO_HORA) hora_tmp++;
    else minuto_tmp++;
    if (modo == ALTERAR_ALARME) {      
      tratamento_de_horario(&hora_alarme, &minuto_alarme);
    }
    else if (modo == ALTERAR_HORARIO) {
      tratamento_de_horario(&hora, &minuto);
    }
  }
}

void tratamento_de_horario(volatile unsigned int *hora, volatile unsigned int *minuto) {
  *hora += hora_tmp;
  *minuto += minuto_tmp;
  if (*minuto == 60) {
    *minuto = 0;
    *hora = *hora + 1;
  }
  if (*hora == 24) *hora = 0;  
}

void exibir_modo(int modo) {
  cmd_LCD(0x80, 0);
  escreve_LCD(modos[modo]);
  cmd_LCD(0xC0, 0);  
  switch (modo) {
    case EXIBIR_HORARIO:
      sprintf(buffer, "%.2d:%.2d:%.2d", hora, minuto, segundo);
      break;
    case ALTERAR_ALARME:
      atualizar_exibicao(hora_alarme, minuto_alarme);
      break;
    case ALTERAR_HORARIO:
      atualizar_exibicao(hora, minuto);      
      break;
  }
  escreve_LCD(buffer);
}

void atualizar_exibicao(volatile unsigned int hora, volatile unsigned int minuto) {
  hora_exib = hora;
  minuto_exib = minuto;
  blink_cursor();
}

void blink_cursor() {
  if (!exibir_numero) {
    if (cursor == SECAO_HORA) {
      sprintf(buffer, "  :%.2d:%.2d", minuto_exib, segundo);
    }
    else if (cursor == SECAO_MINUTO) {
      sprintf(buffer, "%.2d:  :%.2d", hora_exib, segundo);
    }
  }
  else {              
    sprintf(buffer, "%.2d:%.2d:%.2d", hora_exib, minuto_exib, segundo);                               
  }
  exibir_numero = !exibir_numero;      
}
