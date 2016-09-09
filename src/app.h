#ifndef APP_H
#define APP_H

// ###########################################################################################################################################
// DESENVOLVIMENTO
#define LOG_MODBUS  pdON

// ###########################################################################################################################################
// MODEL WORKTEMP

#define MODEL   "ms10" // somente 4 chars, por causa so modbus
#define FIRMWARE_VERSION "1.0" // usar x.y s� com uma casa decimal, pois estamos usando a impress�o de temperatura
// usar 3 chars por causa do modbus
#define PROGAM_NAME "MSIP" MODEL " firmware v" FIRMWARE_VERSION " [compiled in " __DATE__ " " __TIME__ "]" CMD_TERMINATOR



// ###########################################################################################################################################
// INCLUDES


// ###########################################################################################################################################
// MODBUS
#define COM_PORT "/dev/ttyAMA0"  // Consultar /dev/ttySxxx do linux
// Opensuse com placa pci para serial = "/dev/ttyS4"
// Raspberry "/dev/ttyAMA0"
// Para descobrir qual o n� da porta UART fa�a o seguinte:
// 		Conecte o RASP com um PC via cabo serial.
//		No PC com um terminal de comnica��o serial abra uma conex�o com RASP a 115200bps 8N1
//		No RAPS liste as portas tty: ls tty*
// 		No RAPS na linha de comando envie uma mensagem para cada porta serial: echo ola /dev/ttyXXXX
//			Fa�a para para todas as portas listadas at� que receba a mensagem no terminal do PC

#define MODBUS_BAUDRATE  B57600 // N�o usar acima de 57600, pois h� erro de recep��o do raspberry.
// Deve ser algum bug de hardware do rasp porque o baudrate do rasp n�o fica indentico do ARM
// pois a comunica��o com PC a 115200 funciona bem.
// Ou a tolerancia de erro no rasp n�o � t�o grande como no PC onde o ARM tem um erro consider�vel
//	TODO Quando usar o oscilador interno do ARM refazer os testes a sabe com usando oscilador interno do ARM isso se resolve

// ###########################################################################################################################################
// CONTROLE DO SISTEMA

typedef struct {
    int stsCom; // Status de comunica��o com multimetro via modbus
    //		0: Sem comunica��o com o RH. O mesmo n�o est� conectado, ou est� desligado, ou n�o h� dispositivo neste endere�o.
    //		1: O RH recebeu uma fun��o que n�o foi implementada;
    //		2: Foi acessado a um endere�o de registrador inexistente;
    //		3: Foi tentado gravar um valor inv�lido no registrador do RH;
    //		4: Um irrecuper�vel erro ocorreu enquanto o RH estava tentando executar a a��o solicitada;
    //		5: Comunica��o estabelecida com sucesso

    int func; // Fun��o assumida do mult�metro:
    //		0: Amper�metro;
    // 		1: Volt�metro.
    int sts; // Status do sensor
    // 		0: Sinaliza que o mult�metro est� lendo pela primeira vez o sensor. Isto somente acontece no momento que o mult�metro � ligado.
    // 		1: O mult�metro j� cont�m o valor convertido;
    // 		2: Sinaliza um erro, indica que o valor est� abaixo da escala permitida pelo mult�metro;
    // 		3: Sinaliza um erro, indica que o valor est� acima da escala permitida pelo mult�metro;
    int value; // valores em miliampers ou milivolts
} tMultimeter;

typedef enum {
    cmdNONE = 0,
    cmdGET_INFO,
    cmdSET_LAMP_TIME,
    cmdSET_LAMP,
    cmdSET_SW1,
    cmdSET_SW2,
    cmdSET_SW3,
    cmdSET_CAP,
    cmdSET_MOTOR,
    cmdSET_NSAMPLES,
    cmdSET_CSAMPLES,
    cmdSET_SAMPLING,
    cmdGET_IS_SAMPLING,
    cmdGET_NSAMPLES,
    cmdGET_SAMPLES,
    cmdGET_VOLTAGE,
    cmdSET_DEFAULTCONF
} tCommand;

typedef struct {
    // flash de controle de a��o para acesso ao RH. Esses flags devem ser atualizados via rede
    unsigned exit : 1; // 1 Sinaliza para cair fora do programa
    unsigned getInfo : 1; // Sinaliza para capturar as informa��es do recurso de hardware
    unsigned setLamp : 1;
    unsigned setLampTime : 1;
    unsigned setSwitch1 : 1;
    unsigned setSwitch2 : 1;
    unsigned setSwitch3 : 1;
    unsigned setCapacitorDischarging : 1;
    unsigned setServo : 1;
    unsigned setNumberofSamples : 1;
    unsigned cleanSamples : 1;
    unsigned setSampling : 1;
    unsigned isSampling : 1;
    unsigned getNumberofSamples : 1;
    unsigned getSamples : 1;
    unsigned getVoltage : 1;
    unsigned setDefaultConfiguration : 1;

    int stsCom; // Status de comunica��o com RH via modbus
    //		0: Sem comunica��o com o RH. O mesmo n�o est� conectado, ou est� desligado, ou n�o h� dispositivo neste endere�o.
    //		1: O RH recebeu uma fun��o que n�o foi implementada;
    //		2: Foi acessado a um endere�o de registrador inexistente;
    //		3: Foi tentado gravar um valor inv�lido no registrador do RH;
    //		4: Um irrecuper�vel erro ocorreu enquanto o RH estava tentando executar a a��o solicitada;
    //		5: Comunica��o estabelecida com sucesso
    uint rhID; // Qual o ID do RH no barramento modbus
    string rhModel; // Modelo do RH
    string rhFirmware; // Firmware vers�o

    int status; // Guarda o status do experimento
    // 0: Sinaliza que o experimento ou processo de configura��o dos term�metros foi feito com sucesso;
    // 1: N�o foi poss�vel se comunicar pelo menos com um term�metro;
    // 2: Pelo menos um dos term�metros deu erro de leitura de temperatura, ou a mesma ficou fora dos limites de trabalho
    uint switchValue; // Leitura do peso em gramas da esfera
    uint nSamples; // Leitura do peso em gramas da esfera
    uint conf;
    uint servomotor;
    tMultimeter multimeter;
    uint mult_index;
    uint nsamples;
    uint sampling;
    uint lamptime;
} tControl, *pControl;

typedef enum {
    readREGS, writeREG, writeREGS
} tcmd;


// ###############################################################################
// PROTOTIPOS
int modbus_Init(void);
void modbus_SendCommand(tCommand c);
void init_control_tad();
void * modbus_Process(void * params);

#endif
