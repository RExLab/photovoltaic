#include "timer/timer.h"
#include "_config_cpu_.h"
#include "uart/uart.h"
#include "modbus/modbus_master.h"
#include <nan.h>
#include "app.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <v8.h>
#include <string.h>
#include <nan.h>
#include <iostream>
#include <errno.h>
#include <stdlib.h>

#define TIMEOUT 100;

using namespace v8;
pthread_t id_thread;
static int waitResponse = pdFALSE; // Testas com inicio pdFalse
static tCommand cmd;
static u16 regs[120]; // registrador de trabalho para troca de dados com os multimetros
tControl control;

int modbus_Init(void) {
    //fprintf(flog, "Abrindo UART %s"CMD_TERMINATOR, COM_PORT);
#if (LOG_MODBUS == pdON)
    printf("Abrindo UART %s"CMD_TERMINATOR, COM_PORT);
#endif
    if (uart_Init(COM_PORT, MODBUS_BAUDRATE) == pdFAIL) {
        //fprintf(flog, "Erro ao abrir a porta UART"CMD_TERMINATOR);
        //fprintf(flog, "    Verifique se essa porta n�o esteja sendo usada por outro programa,"CMD_TERMINATOR);
        //fprintf(flog, "    ou se o usu�rio tem permiss�o para usar essa porta: chmod a+rw %s"CMD_TERMINATOR, COM_PORT);
#if (LOG_MODBUS == pdON)
        printf("Erro ao abrir a porta UART"CMD_TERMINATOR);
        printf("    Verifique se essa porta n�o esteja sendo usada por outro programa,"CMD_TERMINATOR);
        printf("    ou se o usu�rio tem permiss�o para usar essa porta: chmod a+rw %s"CMD_TERMINATOR, COM_PORT);
#endif
        return pdFAIL;
    }

    //fprintf(flog, "Port UART %s aberto com sucesso"CMD_TERMINATOR, COM_PORT);
#if (LOG_MODBUS == pdON)
    printf("Port UART %s aberto com sucesso a %d bps"CMD_TERMINATOR, COM_PORT, MODBUS_BAUDRATE);
#endif
    modbus_MasterInit(uart_SendBuffer, uart_GetChar, uart_ClearBufferRx);
    modbus_MasterAppendTime(now, 3000);

    return pdPASS;
}

// para grava��o os valores dos registradores devem estar preenchidos
// ap�s a leitura os registradores estar�o preenchidos

// Envia um comando para o escravo, nesta mesma fun��o � feito o procedimento de erro de envio
// Uma vez enviado o comando com sucesso caprturar resposta no modbus_Process
//	BUSY: Ficar na espera da resposta
//  ERROR: Notificar  o erro e tomar procedimento cab�veis
//  OK para escrita: Nada, pois os valores dos registradores foram salvos no escravo com sucesso
//	OK para Leitura: Capturar os valores dos registradores lidos do escravo
// Parametros
// c: Tipo de comando para ser enviado ao escravo
// funcResponse: Ponteiro da fun��o para processar a resposta da comunica��o

void modbus_SendCommand(tCommand c) {
    if (waitResponse) return; // pdFAIL;

    cmd = c;
    // APONTA QUAIS REGISTRADORES A ACESSAR NO DISPOSITIVO
    // -----------------------------------------------------------------------------------------------------------------

    // Comando para ler os registradores: modelo e vers�o firmware
    tcmd typeCMD = readREGS;
    uint addrInit = 0;
    uint nRegs = 3;
    u16 value = 0;

    if (cmd == cmdSET_LAMP_TIME) {
        typeCMD = writeREG;
        addrInit = 0x10;
        nRegs = 1;
        value = control.lamptime;

    } else if (cmd == cmdSET_LAMP) {
        typeCMD = writeREG;
        addrInit = 0x11;
        nRegs = 1;
        value = (control.switchValue >> 0)& 1;

    } else if (cmd == cmdSET_SW1) {
        typeCMD = writeREG;
        addrInit = 0x12;
        nRegs = 1;
        value = (control.switchValue >> 1)& 1;

    } else if (cmd == cmdSET_SW2) {
        typeCMD = writeREG;
        addrInit = 0x13;
        nRegs = 1;
        value = (control.switchValue >> 2)& 1;

    } else if (cmd == cmdSET_SW3) {
        typeCMD = writeREG;
        addrInit = 0x14;
        nRegs = 1;
        value = (control.switchValue >> 3)& 1;

    } else if (cmd == cmdSET_CAP) {
        typeCMD = writeREG;
        addrInit = 0x15;
        nRegs = 1;
        value = control.switchValue;

    } else if (cmd == cmdSET_MOTOR) {
        typeCMD = writeREG;
        addrInit = 0x16;
        nRegs = 1;
        value = control.servomotor;

    } else if (cmd == cmdSET_NSAMPLES) {
        typeCMD = writeREG;
        addrInit = 0x20;
        nRegs = 1;
        value = control.nsamples;
    } else if (cmd == cmdSET_CSAMPLES) {
        typeCMD = writeREG;
        addrInit = 0x21;
        nRegs = 1;
        value = 0;
    } else if (cmd == cmdSET_SAMPLING) {
        typeCMD = writeREG;
        addrInit = 0x22;
        nRegs = 1;
        value = control.sampling;
    } else if (cmd == cmdGET_IS_SAMPLING) {
        addrInit = 0x23;
        nRegs = 1;
    } else if (cmd == cmdGET_NSAMPLES) {
        addrInit = 0x24;
        nRegs = 1;
    } else if (cmd == cmdGET_VOLTAGE) {
        addrInit = 0x25;
        nRegs = 2;
    } else if (cmd == cmdGET_SAMPLES) {
        addrInit = 0x400 + control.mult_index;
        nRegs = 1;
    } else if (cmd == cmdSET_DEFAULTCONF) {
        typeCMD = writeREG;
        addrInit = 0x30;
        nRegs = 1;
        value = control.conf;
    }

    // ENVIA O COMANDO AO DISPOSITIVO ESCRAVO
    // -----------------------------------------------------------------------------------------------------------------
    int ret;
    if (typeCMD == writeREG) {
#if (LOG_MODBUS == pdON)
        printf("modbus WriteReg [cmd %d] [slave %d] [reg 0x%x] [value 0x%x]"CMD_TERMINATOR, cmd, control.rhID, addrInit, value);
#endif
        ret = modbus_MasterWriteRegister(control.rhID, addrInit, value);
    } else if (typeCMD == writeREGS) {
#if (LOG_MODBUS == pdON)
        printf("modbus WriteRegs [cmd %d] [slave %d] [reg 0x%x] [len %d]"CMD_TERMINATOR, cmd, control.rhID, addrInit, nRegs);
#endif
        ret = modbus_MasterWriteRegisters(control.rhID, addrInit, nRegs, regs);
    } else {
#if (LOG_MODBUS == pdON)
        printf("modbus ReadRegs [cmd %d] [slave %d] [reg 0x%x] [len %d]"CMD_TERMINATOR, cmd, control.rhID, addrInit, nRegs);
#endif
        ret = modbus_MasterReadRegisters(control.rhID, addrInit, nRegs, regs);
    }

    // se foi enviado com sucesso ficaremos na espera da resposta do recurso de hardware
    if (ret == pdPASS) {
        waitResponse = pdTRUE;
    }
#if (LOG_MODBUS == pdON)
        //else fprintf(flog, "modbus err[%d] send querie"CMD_TERMINATOR, modbus_MasterReadStatus());
    else {
        printf("modbus err[%d] SEND querie"CMD_TERMINATOR, modbus_MasterReadStatus());
        //waitResponse = pdTRUE;
    }
#endif

    return;
}



// processo do modbus.
//	Neste processo gerencia os envios de comandos para o recurso de hardware e fica no aguardo de sua resposta
//	Atualiza as variaveis do sistema de acordo com a resposta do recurso de hardware.

void * modbus_Process(void * params) {

    int first = 0;

    while (control.exit == 0) {

        if (first) {
            usleep(1000);
        }
        first = 1;
        modbus_MasterProcess();

        // Gerenciador de envio de comandos
        // se n�o estamos esperando a resposta do SendCommand vamos analisar o pr�ximo comando a ser enviado
        if (!waitResponse) {
            if (control.setLamp) {
                modbus_SendCommand(cmdSET_LAMP);

            } else if (control.setLampTime) {
                modbus_SendCommand(cmdSET_LAMP_TIME);

            } else if (control.setSwitch1) {
                modbus_SendCommand(cmdSET_SW1);

            } else if (control.setSwitch2) {
                modbus_SendCommand(cmdSET_SW2);

            } else if (control.setSwitch3) {
                modbus_SendCommand(cmdSET_SW3);

            } else if (control.setServo) {
                modbus_SendCommand(cmdSET_MOTOR);

            } else if (control.setNumberofSamples) {
                modbus_SendCommand(cmdSET_NSAMPLES);

            } else if (control.setSampling) {
                modbus_SendCommand(cmdSET_SAMPLING);

            } else if (control.isSampling) {
                modbus_SendCommand(cmdGET_IS_SAMPLING);

            } else if (control.cleanSamples) {
                modbus_SendCommand(cmdSET_CSAMPLES);

            } else if (control.setCapacitorDischarging) {
                modbus_SendCommand(cmdSET_CAP);

            } else if (control.getNumberofSamples) {
                modbus_SendCommand(cmdGET_NSAMPLES);

            } else if (control.getSamples) {
                modbus_SendCommand(cmdGET_SAMPLES);

            } else if (control.getVoltage) {
                modbus_SendCommand(cmdGET_VOLTAGE);
            }

            continue;
        }

        int ret = modbus_MasterReadStatus();
        //	BUSY: Ficar na espera da resposta
        //  ERROR: Notificar  o erro e tomar procedimento cab�veis
        //  OK para escrita: Nada, pois os valores dos registradores foram salvos no escravo com sucesso
        //	OK para Leitura: Capturar os valores dos registradores lidos do escravo

        // se ainda est� ocupado n�o faz nada

        if (ret == errMODBUS_BUSY) {
            continue;
        }
        waitResponse = pdFALSE;
        if (ret < 0) {
            control.stsCom = modbus_MasterReadException();
#if (LOG_MODBUS == pdON)
            printf("modbus err[%d] WAIT response "CMD_TERMINATOR, ret);
            printf("status: %i\n", control.stsCom);
#endif

            continue;

        }

        control.stsCom = 5; // sinaliza que a conex�o foi feita com sucesso


        // ATUALIZA VARS QUANDO A COMUNICA��O FOI FEITA COM SUCESSO
        // -----------------------------------------------------------------------------------------------------------------
        if (cmd == cmdGET_INFO) {
            // Comando para ler os registradores: modelo e vers�o firmware do RH
#if (LOG_MODBUS == pdON)
            printf("model %c%c%c%c"CMD_TERMINATOR, (regs[0] & 0xff), (regs[0] >> 8), (regs[1] & 0xff), (regs[1] >> 8));
            printf("firware %c.%c"CMD_TERMINATOR, (regs[2] & 0xff), (regs[2] >> 8));
#endif
            control.rhModel[0] = (regs[0] & 0xff);
            control.rhModel[1] = (regs[0] >> 8);
            control.rhModel[2] = (regs[1] & 0xff);
            control.rhModel[3] = (regs[1] >> 8);
            control.rhModel[4] = 0;
            control.rhFirmware[0] = (regs[2] & 0xff);
            control.rhFirmware[1] = (regs[2] >> 8);
            control.rhFirmware[2] = 0;

            control.getInfo = 0;

        } else if (cmd == cmdSET_LAMP_TIME) {
            control.setLampTime = 0;

        } else if (cmd == cmdSET_LAMP) {
            control.setLamp = 0;

        } else if (cmd == cmdSET_SW1) {
            control.setSwitch1 = 0;

        } else if (cmd == cmdSET_SW2) {
            control.setSwitch2 = 0;

        } else if (cmd == cmdSET_SW3) {
            control.setSwitch3 = 0;

        } else if (cmd == cmdSET_CAP) {
            control.setCapacitorDischarging = 0;

        } else if (cmd == cmdSET_MOTOR) {
            control.setServo = 0;

        } else if (cmd == cmdSET_NSAMPLES) {
            control.setNumberofSamples = 0;

        } else if (cmd == cmdSET_CSAMPLES) {
            control.cleanSamples = 0;

        } else if (cmd == cmdSET_SAMPLING) {
            control.setSampling = 0;

        } else if (cmd == cmdGET_IS_SAMPLING) {
            control.isSampling = 0;
#if (LOG_MODBUS == pdON)
            printf("Status da amostragem: %i"CMD_TERMINATOR, control.sampling);
#endif
            control.sampling = regs[0];

        } else if (cmd == cmdGET_NSAMPLES) {
            control.getNumberofSamples = 0;
            control.nSamples = regs[0];
#if (LOG_MODBUS == pdON)
            printf("Numero de amostras: %i"CMD_TERMINATOR, control.nSamples);
#endif

        } else if (cmd == cmdGET_VOLTAGE) {
            control.getVoltage = 0;
            control.multimeter.stsCom = regs[0] & 0xff;
            control.multimeter.func = (regs[1] & 0x10) >> 4;
            control.multimeter.sts = regs[1] & 0xf;
            control.multimeter.value = (regs[2]) | (regs[3] << 16);
#if (LOG_MODBUS == pdON)
            printf("MULTIMETER stsCom 0x%x func %d sts 0x%x value %d"CMD_TERMINATOR,
                    control.multimeter.stsCom,
                    control.multimeter.func,
                    control.multimeter.sts,
                    control.multimeter.value
                    );
#endif

        } else if (cmd == cmdGET_SAMPLES) {
            control.getSamples = 0;
            //control.multimeter

        } else if (cmd == cmdSET_DEFAULTCONF) {
            control.setDefaultConfiguration = 0;

        }
    }
    printf("Fechando programa"CMD_TERMINATOR);

    return NULL;
}

void init_control_tad() {
    control.rhID = 1; // Sinaliza que o endere�o do RH no modbus � 1
    control.getInfo = 0; // sinaliza para pegar as informa��es do RH
    control.exit = 0;
    control.setLamp = 0;
    control.setSwitch1 = 0;
    control.setSwitch2 = 0;
    control.setSwitch3 = 0;
    control.setCapacitorDischarging = 0;
    control.setServo = 0;
    control.setNumberofSamples = 0;
    control.cleanSamples = 0;
    control.setSampling = 0;
    control.isSampling = 0;
    control.getNumberofSamples = 0;
    control.getSamples = 0;
    control.getVoltage = 0;
    control.setDefaultConfiguration = 0;
    memset(control.rhModel, '\0', __STRINGSIZE__);
    memset(control.rhFirmware, '\0', __STRINGSIZE__);
    return;
}

NAN_METHOD(Run) {
    NanScope();
    init_control_tad();

    int rthr = pthread_create(&id_thread, NULL, modbus_Process, (void *) 0); // fica funcionando at� que receba um comando via WEB para sair		
    if (rthr) {
        printf("Unable to create thread void * modbus_Process");

    }
    printf("Thread is running\n");

    NanReturnValue(NanNew(1));
}

NAN_METHOD(Setup) {
    NanScope();
    int i = 0;

    FILE *flog = fopen("msip.log", "w");

    if (modbus_Init() == pdFAIL) {
        fprintf(flog, "Erro ao abrir a porta UART"CMD_TERMINATOR);
        fprintf(flog, "Verifique se essa porta n�o esteja sendo usada por outro programa,"CMD_TERMINATOR);
        fprintf(flog, "ou se o usu�rio tem permiss�o para usar essa porta: chmod a+rw %s"CMD_TERMINATOR, COM_PORT);
        NanReturnValue(NanNew("0"));

    }

    fclose(flog);

    NanReturnValue(NanNew(1));
}

NAN_METHOD(Exit) {
    NanScope();
    init_control_tad();
    control.exit = 1;
    uart_Close();
    NanReturnValue(NanNew(1));
}

NAN_METHOD(Switch) {
    NanScope();
    if (args.Length() < 2) {
        NanThrowTypeError("Wrong number of arguments");
        NanReturnUndefined();
    }
    if (!args[0]->IsNumber() && !args[1]->IsNumber()) {
        NanThrowTypeError("Wrong type of arguments, it should be integer");
        NanReturnUndefined();
    }

    switch ((int) args[0]->NumberValue()) {
        case 0:
            control.setLamp = 1;
            control.switchValue = control.switchValue | ((int) args[1]->NumberValue() & 1) << 0;
            break;
        case 1:
            control.setSwitch1 = 1;
            control.switchValue = control.switchValue | ((int) args[1]->NumberValue() & 1) << 1;

            break;
        case 2:
            control.setSwitch2 = 1;
            control.switchValue = control.switchValue | ((int) args[1]->NumberValue() & 1) << 2;
            break;
        case 3:
            control.setSwitch3 = 1;
            control.switchValue = control.switchValue | ((int) args[1]->NumberValue() & 1) << 3;
            break;
        default:
            NanThrowRangeError("Argument 0 is out of valid range");
    }

    NanReturnValue(NanNew(1));
}

NAN_METHOD(Motor) {
    NanScope();
    if (args.Length() < 1) {
        NanThrowTypeError("Wrong number of arguments");
        NanReturnUndefined();
    }
    if (!args[0]->IsNumber()) {
        NanThrowTypeError("Wrong type of arguments, it should be integer");
        NanReturnUndefined();
    }

    if (args[0]->NumberValue() >= 1000 && args[0]->NumberValue() <= 2500) {
        control.servomotor = args[0]->NumberValue();
        control.setServo = 1;
        int timeout = TIMEOUT;
        while (control.setServo) {
            usleep(10000);
            timeout--;
            if (timeout < 0) {
                control.setServo = 0;
                NanThrowError("In function setServo: Modbus Timeout");
            }
        }
    } else {
        NanThrowRangeError("Argument 0 is out of valid range");
    }

    NanReturnValue(NanNew(1));
}

NAN_METHOD(LampTime) {
    NanScope();
    if (args.Length() < 1) {
        NanThrowTypeError("Wrong number of arguments");
        NanReturnUndefined();
    }
    if (!args[0]->IsNumber()) {
        NanThrowTypeError("Wrong type of arguments, it should be integer");
        NanReturnUndefined();
    }

    control.lamptime = args[0]->NumberValue();
    control.setLampTime = 1;
    
    int timeout = TIMEOUT;
    while (control.setLampTime) {
        usleep(10000);
        timeout--;
        if (timeout < 0) {
            control.setLampTime = 0;
            NanThrowError("In function LampTime: Modbus Timeout");
        }
    }

    NanReturnValue(NanNew(1));
}

NAN_METHOD(Sampling) {
    NanScope();
    int timeout = TIMEOUT;
    control.setSampling = 1;

    while (control.setSampling) {
        usleep(10000);
        timeout--;
        if (timeout < 0) {
            control.setSampling = 0;
            NanThrowError("In function Sampling: Modbus Timeout");
        }
    }
    NanReturnValue(NanNew(1));
}

NAN_METHOD(IsSampling) {
    NanScope();
    control.isSampling = 1;
    int timeout = TIMEOUT;

    while (control.isSampling) {
        usleep(10000);
        timeout--;
        if (timeout < 0) {
            control.isSampling = 0;
            NanThrowError("In function IsSampling: Modbus Timeout");
        }
    }

    NanReturnValue(NanNew(control.sampling));
}

NAN_METHOD(CleanSamples) {
    NanScope();
    control.cleanSamples = 1;
    int timeout = TIMEOUT;

    while (control.cleanSamples) {
        usleep(10000);
        timeout--;
        if (timeout < 0) {
            control.cleanSamples = 0;
            NanThrowError("In function cleanSamples: Modbus Timeout");
        }
    }

    NanReturnValue(NanNew(1));
}

NAN_METHOD(GetVoltage) {
    NanScope();
    int i;
    std::string buffer = "{";
    std::string buffer_amp = "", buffer_volt = "";
    char value[10];
    buffer_amp = std::string("\"amperemeter\":[");
    buffer_volt = std::string("\"voltmeter\":[");
    /*
    for(i =0; i < nMULTIMETER_GEREN ; i++){
            if(control.multimeter[i].sts)
                            sprintf( value, "%i", control.multimeter[i].value);
            else	
                            sprintf( value, "%i", 0);
			
		
            if(control.multimeter[i].func){
                    if(i == 0)
                            buffer_amp = buffer_amp + std::string(value) ;
                    else
                            buffer_amp = buffer_amp +  std::string(",") + std::string(value) ;
			
			
            }else{
                    if(i == nMULTIMETER_GEREN-1)
                            buffer_volt = buffer_volt +  std::string(value) ;
                    else
                            buffer_volt = buffer_volt +  std::string(value) + std::string(",");
			
			
            }				
    } */
    buffer_amp = buffer_amp + std::string("]");
    buffer_volt = buffer_volt + std::string("]");

    ;

    NanReturnValue(NanNew("{" + buffer_amp + "," + buffer_volt + "}"));

}

NAN_METHOD(GetSamples) {
    NanScope();
    int i;
    std::string buffer = "{";
    std::string buffer_amp = "", buffer_volt = "";
    char value[10];
    buffer_amp = std::string("\"amperemeter\":[");
    buffer_volt = std::string("\"voltmeter\":[");
    /*
    for(i =0; i < nMULTIMETER_GEREN ; i++){
            if(control.multimeter[i].sts)
                            sprintf( value, "%i", control.multimeter[i].value);
            else	
                            sprintf( value, "%i", 0);
			
		
            if(control.multimeter[i].func){
                    if(i == 0)
                            buffer_amp = buffer_amp + std::string(value) ;
                    else
                            buffer_amp = buffer_amp +  std::string(",") + std::string(value) ;
			
			
            }else{
                    if(i == nMULTIMETER_GEREN-1)
                            buffer_volt = buffer_volt +  std::string(value) ;
                    else
                            buffer_volt = buffer_volt +  std::string(value) + std::string(",");
			
			
            }				
    } */
    buffer_amp = buffer_amp + std::string("]");
    buffer_volt = buffer_volt + std::string("]");

    NanReturnValue(NanNew("{" + buffer_amp + "," + buffer_volt + "}"));

}

NAN_METHOD(NumberofSamples) {
    NanScope();
    if (args.Length() < 1) {
        control.getNumberofSamples = 1;
        NanReturnValue(NanNew(control.nSamples));

    } else {

        if (!args[0]->IsNumber()) {
            NanThrowTypeError("Wrong type of arguments, it should be integer");
            NanReturnUndefined();
        }
        if (args[0]->NumberValue() >= 10 && args[0]->NumberValue() <= 600) {
            control.nSamples = args[0]->IsNumber();
            control.setNumberofSamples = 1;

            int timeout = TIMEOUT;
            while (control.setNumberofSamples) {
                usleep(10000);
                timeout--;
                if (timeout < 0) {
                    control.setNumberofSamples = 0;
                    NanThrowError("In function NumberofSamples: Modbus Timeout");
                }
            }
        } else {
            NanThrowTypeError("Argument 0 is out of valid range");
        }
    }

    NanReturnValue(NanNew(control.nSamples));
}

void Init(Handle<Object> exports) { // cria fun��es para serem invocadas na aplica��o em nodejs
    exports->Set(NanNew("run"), NanNew<FunctionTemplate>(Run)->GetFunction());
    exports->Set(NanNew("setup"), NanNew<FunctionTemplate>(Setup)->GetFunction());
    exports->Set(NanNew("exit"), NanNew<FunctionTemplate>(Exit)->GetFunction());
    exports->Set(NanNew("motor"), NanNew<FunctionTemplate>(Motor)->GetFunction());
    exports->Set(NanNew("switch"), NanNew<FunctionTemplate>(Switch)->GetFunction());
    exports->Set(NanNew("lampTime"), NanNew<FunctionTemplate>(LampTime)->GetFunction());
    exports->Set(NanNew("sampling"), NanNew<FunctionTemplate>(Sampling)->GetFunction());
    exports->Set(NanNew("isSampling"), NanNew<FunctionTemplate>(IsSampling)->GetFunction());
    exports->Set(NanNew("numberOfSamples"), NanNew<FunctionTemplate>(NumberofSamples)->GetFunction());
    exports->Set(NanNew("getSample"), NanNew<FunctionTemplate>(GetVoltage)->GetFunction());
    exports->Set(NanNew("getSamples"), NanNew<FunctionTemplate>(GetSamples)->GetFunction());
    exports->Set(NanNew("cleanSamples"), NanNew<FunctionTemplate>(CleanSamples)->GetFunction());


}


NODE_MODULE(panel, Init)
