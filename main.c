/*
 LEIA O README
 */

// DEFINI��ES
//=============================================================================================================
#define F_CPU 16000000
#define SECAO_HORA 0
#define SECAO_MINUTO 1
#define EXIBIR_HORARIO 0
#define ALTERAR_ALARME 1
#define ALTERAR_HORARIO 2
#define FLAG_EXIBICAO 20 // para overflow de 10 us -> exibi��o ser� atualizada a cada {FLAG_EXIBICAO} ms
#define FLAG_INCREMENTO 18 // para overflow de 10 us -> com bot�o pressionado, ser� incrementado a 
                           // cada {FLAG_INCREMENTO} ms
#define botao_apertado(y,bit)  !(y&(1<<bit)) // macro para teste bit (botao) em um registrador y
//=============================================================================================================


// INCLUS�ES
//=============================================================================================================
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h> //pois � dependencia para a lib em LCD.h
#include <stdio.h>
#include "LCD.h"
//=============================================================================================================


// PROT�TIPOS
//=============================================================================================================
ISR(TIMER0_OVF_vect);
ISR(TIMER1_OVF_vect);
ISR(PCINT0_vect);
ISR(PCINT1_vect);
void blink_cursor();
void acionar_alarme();
void desacionar_alarme();
void exibir_modo(int modo);
void direcionar_incremento_de_horario();
void atualizar_exibicao(volatile unsigned int arg_hora, volatile unsigned int arg_minuto);
void tratamento_de_horario(volatile unsigned int *arg_hora, volatile unsigned int *arg_minuto);
void incremento_e_tratamento_de_horario(volatile unsigned int *arg_hora, volatile unsigned int *arg_minuto);
//=============================================================================================================


// VARI�VEIS GLOBAIS
//=============================================================================================================
volatile unsigned int hora = 0, minuto = 0, segundo = 0;
volatile unsigned int hora_exib = 0, minuto_exib = 0;
volatile unsigned int hora_alarme = 0, minuto_alarme = 0;
volatile unsigned int hora_tmp = 0, minuto_tmp = 0;
volatile unsigned int modo = 0;
volatile unsigned int exibir_numero = 0;
volatile unsigned char cursor = SECAO_HORA;
volatile unsigned int contador_timer0 = 0;
volatile unsigned int contador_incremento = 0;
volatile char buffer[9] = "        ";
char modos[][13] = {"HORARIO     ", "ALT. ALARME ", "ALT. HORARIO"};
//=============================================================================================================


// FUN��O PRINCIPAL
//=============================================================================================================
int main(void) {    
  // DISPLAY LCD E ATUADOR DO ALARME
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
  hora = 14;
  minuto = 50;
  segundo = 0;
  hora_alarme = 13;
  minuto_alarme = 50;
    
  while (1) {
    if (hora_alarme == hora && minuto_alarme == minuto) {
      acionar_alarme();
    }
    else desacionar_alarme(); 
  }
}
//=============================================================================================================


// TRTAMENTO DE INTERRUP��ES
//=============================================================================================================
ISR(TIMER0_OVF_vect) {
  // DESCRI��O: CONTROLE DA TAXA DE EXIBI��O DO MODO E DO INCREMENTO CONT�NUO PELO USU�RIO
  TCNT0 = 100;
  contador_timer0++;
  if (botao_apertado(PINC, PC3)) {
    // ESSE BLOCO: IR� INCREMENTAR A UMA TAXA "CONTINUA" ENQUANTO O BOT�O FOR PRESSIONADO
    contador_incremento++;
    if (contador_incremento == FLAG_INCREMENTO) {
      contador_incremento = 0;
      direcionar_incremento_de_horario();
    }
  }
  if (contador_timer0 == FLAG_EXIBICAO) {
    contador_timer0 = 0; 
    exibir_modo(modo);
  }
}

ISR(TIMER1_OVF_vect) {
  // DESCRI��O: CONTROLE PARA INCREMENTO AUTOM�TICO DO HOR�RIO
  TCNT1 = 49911;    
  segundo++;
  if (segundo > 59) {
    segundo = 0;
    minuto++;
    if (minuto > 59) {
      minuto = 0;
      hora++;
      if (hora > 23) {
        hora = 0;
      }
    }    
  }  
}

ISR(PCINT0_vect) {
  // DESCRI��O: CONTROLE DO ESTADO DE QUAL MODO SER� EXIBIDO
  if (botao_apertado(PINB, PB4)) {
    modo++;
    if (modo == 3) modo = 0;
    cursor = SECAO_HORA;
  }
}

ISR(PCINT1_vect) {
  // DESCRI��O: CONTROLE DO MAPEAMETO DO CURSOR PISCANTE E DA FINALIZA��O PELO
  //            USU�RIO DO INCREMENTO DO N�MERO DA SE��O CUJO CURSOR EST� APONTANDO
  if (botao_apertado(PINC, PC2)) {
    cursor = !cursor; // seu mapeamento � entre os n�meros 0 e 1      
  }  
  if (!botao_apertado(PINC, PC3)) {
    contador_incremento = 0;
  }
}
//=============================================================================================================


// FUN��ES AUXILIARES
//=============================================================================================================
void direcionar_incremento_de_horario() {
  hora_tmp = minuto_tmp = 0; // vari�veis tempor�rias
  if (cursor == SECAO_HORA) hora_tmp++;
  else minuto_tmp++;
  if (modo == ALTERAR_ALARME) {      
    incremento_e_tratamento_de_horario(&hora_alarme, &minuto_alarme);
  }
  else if (modo == ALTERAR_HORARIO) {
    incremento_e_tratamento_de_horario(&hora, &minuto);
  }
}

void incremento_e_tratamento_de_horario(volatile unsigned int *arg_hora, volatile unsigned int *arg_minuto) {
  *arg_hora += hora_tmp;
  *arg_minuto += minuto_tmp;
  tratamento_de_horario(arg_hora, arg_minuto);
}

void tratamento_de_horario(volatile unsigned int *arg_hora, volatile unsigned int *arg_minuto) {
  if (*arg_minuto > 59) {
    *arg_minuto = 0;
  }
  if (*arg_hora > 23) *arg_hora = 0;  
}

void exibir_modo(int modo) {
  cmd_LCD(0x80, 0);
  escreve_LCD(modos[modo]);
  cmd_LCD(0xC0, 0);  
  switch (modo) {
    case EXIBIR_HORARIO:
      tratamento_de_horario(&hora, &minuto);
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
  tratamento_de_horario(&arg_hora, &arg_minuto);
  hora_exib = arg_hora;
  minuto_exib = arg_minuto;  
  if (botao_apertado(PINC, PC3)) {
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
  PORTD |= (1 << PD3);
}

void desacionar_alarme() {
  PORTD &= ~(1 << PD3);
}
//=============================================================================================================
