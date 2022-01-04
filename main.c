#define F_CPU 16000000
#define SECAO_HORA 0
#define SECAO_MINUTO 1
#define EXIBIR_HORARIO 0
#define ALTERAR_ALARME 1
#define ALTERAR_HORARIO 2
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h> //pois � dependencia para a lib LCD
#include <stdio.h>
#include "LCD.h"

/*
 DEFINI��ES:
 * H� 3 MODOS/STATUS: EXIBI��O, AJUSTE DE HOR�RIO, AJUSTE DE ALARME
 * PB4 -> ALTERA O MODO
 * PC2 -> NOS AJUSTES: ALTERA O "CURSOR" ENTRE HORA E MINUTO
 * PC4 -> NOS AJUSTES: INCREMENTA A VARI�VEL EM QUE EST� APONTADO O "CURSOR"
 */

/*
 TAREFAS:
 * - IMPLEMENTAR O ALARME
 */

// MACROS PARA TRATAMENTO DE BITS NOS REGISTRADORES
#define test_bit(y, bit)    (y & (1 << bit))
#define set_bit(y, bit)     (y |= (1 << bit))
#define clear_bit(y, bit)   (y &= ~(1 << bit))

// PROT�TIPOS
ISR(TIMER0_OVF_vect);
ISR(TIMER1_OVF_vect);
ISR(PCINT0_vect);
ISR(PCINT1_vect);
void exibir_modo(int modo);
void blink_cursor();
void acionar_alarme();
void desacionar_alarme();
void tratamento_de_horario(volatile unsigned int *arg_hora, volatile unsigned int *arg_minuto);
void atualizar_exibicao(volatile unsigned int arg_hora, volatile unsigned int arg_minuto);

// VARI�VEIS GLOBAIS
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
  DDRD = 0b11111000;
  PORTD = 0x00;
  
  // PUSH BUTTONS
  DDRC = 0x00;
  PORTC = 0b00001100; // habilitando pull up dos push buttons
  DDRB = 0x00;
  PORTB = 0b00010000;
  
  // INTERRUP��ES 
  PCICR = 0b00000011; // habilitando interrup��o do PORTB e PORTC
  PCMSK0 = 0b00010000; // habilitando interrup��o do pino PB4
  PCMSK1 = 0b00001100;
  TCCR0B = 0b00000101; // prescaler 1024 do timer 0
  TIMSK0 = 0b00000001; // habilitando interrup��o do timer 0 por overflow
  TCCR1B = 0b00000101;  
  TIMSK1 = 0b00000001;
  TCNT0 = 100; // contagem para overflow em 10 us.
  TCNT1 = 49911; // contagem para overflow em 1 s.
  sei(); // habilitando chave geral de interrup��es
  
  // LCD    
  inic_LCD_4bits();
  hora = 23;
  hora_alarme = 23;
  minuto_alarme = 55;
  minuto = 55;
  
  while (1) {
    if (hora_alarme == hora && minuto_alarme == minuto) {
      PORTD |= (1 << 3);
    }
    else PORTD &= ~(1 << 3);
  }
}

ISR(TIMER0_OVF_vect) {
  // DESCRI��O: CONTROLE DA TAXA DE EXIBI��O DO MODO -> A CADA 0,5 SEGUNDOS
  TCNT0 = 100;
  contador_timer0++;
  if (contador_timer0 == 50) {
    contador_timer0 = 0; 
    exibir_modo(modo);
  }
}

ISR(TIMER1_OVF_vect) {
  // DESCRI��O: CONTROLE PARA INCREMENTO DO HOR�RIO
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
  // DESCRI��O: CONTROLE DO ESTADO DE QUAL MODO SER� EXIBIDO
  if (!test_bit(PINB, PB4)) {
    modo++;
    if (modo == 3) modo = 0;
    cursor = SECAO_HORA;
  }
}

ISR(PCINT1_vect) {
  // DESCRI��O: CONTROLE DO MAPEAMETO DO CURSOR PISCANTE E DO INCREMENTO DO
  //            N�MERO DA SE��O CUJO CURSOR EST� APONTANDO
  if (!test_bit(PINC, PC2)) {
    cursor = !cursor; // seu mapeamento � entre os n�meros 0 e 1      
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

void tratamento_de_horario(volatile unsigned int *arg_hora, volatile unsigned int *arg_minuto) {
  *arg_hora += hora_tmp;
  *arg_minuto += minuto_tmp;
  if (*arg_minuto == 60) {
    *arg_minuto = 0;
    *arg_hora = *arg_hora + 1;
  }
  if (*arg_hora == 24) *arg_hora = 0;  
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

void atualizar_exibicao(volatile unsigned int arg_hora, volatile unsigned int arg_minuto) {
  hora_exib = arg_hora;
  minuto_exib = arg_minuto;
  if (!test_bit(PINC, PC3)) {
    sprintf(buffer, "%.2d:%.2d:%.2d", hora_exib, minuto_exib, segundo); 
  }
  else blink_cursor();
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

void acionar_alarme() {
  set_bit(PIND, PD3);
}
void desacionar_alarme() {
  clear_bit(PIND, PD3);
}