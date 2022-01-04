# LÓGICA DE RELÓGIO DIGITAL COM DISPLAY LCD

## Sobre o projeto
O projeto consiste em desenvolver a lógica de um relógio digital que possa ser visualizado através de um display LCD 16x2 e interagido através de botões (push buttons). O desenvolvimento do código foi escrito em C e baseado no microcontrolador  ATmega328p, utilizando das bibliotecas AVR.

## Por que?
O presente projeto foi desenvolvido com o intuito de aprendizado.

## Funcionalidades
O relógio atualmente possui 4 funcionalidades:

### **Exibição e ajuste do horário**

Alterando horário de 14:50 para 16:02
![horario](https://user-images.githubusercontent.com/63216146/148115320-fe7f9542-993f-43de-8e9a-851b62df164e.gif)

### Ajuste e acionamento de alarme
Alterando o alarme de 13:50 para 15:00.

*- O horário inicia ao final de 14:59*
*- Após acionado o alarme (às 15:00) o gif é cortado para próximo ao instante em que este alarme será desacionado (às 15:01)*
*- O atuador do alarme está sendo emulado pelo acionamento do LED*
![alarme](https://user-images.githubusercontent.com/63216146/148115807-63934717-0a63-4a4c-9dbb-d2e588bf1564.gif)



*As experimentações e testes visuais foram realizados através do simulador SimulIde.*


## Como executar esse projeto
O projeto em si consiste basicamente do arquivo principal `main.c`, que está toda implementação do relógio, e do arquivo `LCD.h` que é a biblioteca usada para lidar com o display LCD.  Basta inserir os arquivos nas pastas devidas em sua aplicação que está sendo desenvolvida em sua ferramenta de desenvolvimento.



