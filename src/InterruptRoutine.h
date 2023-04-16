#pragma once

#include "fsl.h"

#define RAM_ALLOCATED_FOR_HANDLER_BYTES     256
#define HANDLER_SAFE_AREA_BYTES             0

extern volatile uint32_t __timerZeroTickCounter;

extern volatile uint8_t uartTxFlags;
extern volatile void (*IntCallbackUart1OnReceived)(uint8_t rxByte);
extern volatile void (*IntCallbackUart2OnReceived)(uint8_t rxByte);

#pragma section text    ram_text

#pragma interrupt isrRAM  // declared as an interrupt, but is not in the RL78 vector table
static void __far isrRAM(void)
{
    if (TMIF00)
    {
        TMIF00 = 0;
        __timerZeroTickCounter++;
    } else if (SRIF1)
    {
        volatile uint8_t rxData = RXD1 + 0;
        volatile uint8_t err_type = (uint8_t)(SSR03 & 0x0007U);
        SIR03 = (uint16_t)err_type;
        SRIF1 = 0;        
    
        if (IntCallbackUart1OnReceived != 0)
        {
            IntCallbackUart1OnReceived(rxData);
        }
    } else if (SRIF2)
    {
        volatile uint8_t rxData = RXD2 + 0;
        volatile uint8_t errType = (uint8_t)(SSR11 & 0x0007U);
        SIR11 = (uint16_t)errType;
        SRIF2 = 0;
    
        if (IntCallbackUart2OnReceived != 0)
        {
            IntCallbackUart2OnReceived(rxData);
        }
    }
}

#pragma section

volatile uint8_t ramIntHandler[RAM_ALLOCATED_FOR_HANDLER_BYTES];

void CopyIsrHandler(void)
{
    for (uint16_t i = 0; i < RAM_ALLOCATED_FOR_HANDLER_BYTES; i++)
    {
        ramIntHandler[i] = 0;
    }
    
    uint16_t codeSize = ((uint32_t) __secend("ram_text_f")) - ((uint32_t) __sectop("ram_text_f"));
    __far uint8_t *src = __sectop("ram_text_f");
    __near uint8_t *dst = (__near uint8_t *) &ramIntHandler[0] + HANDLER_SAFE_AREA_BYTES;

    for (uint16_t i = 0; i < codeSize; i++)
    {
        *dst = *src;
        src += 1;
        dst += 1;
    }
    
    dst = (__near uint8_t *) &ramIntHandler[0] + HANDLER_SAFE_AREA_BYTES;
    dst = dst + codeSize;
    
    // Add NOP instructions at the end
    for (uint8_t n = 0; n < 10; n++)
    {
        *dst++ = 0;
    }
}

fsl_descriptor_t  fslDescriptor;

bool FlashLibraryInit()
{
    fslDescriptor.fsl_flash_voltage_u08 = 0x00;
    fslDescriptor.fsl_frequency_u08 = 0x08;
    fslDescriptor.fsl_auto_status_check_u08 = 0x01;
        
    if(FSL_Init(&fslDescriptor) != FSL_OK)
    {
        return FALSE;
    }
        
    FSL_Open();
    FSL_PrepareFunctions();
    FSL_PrepareExtFunctions();
    
    return TRUE;
}

void FlashLibraryRelease()
{
    FSL_Close();
}

void SetupInterruptHandlers()
{
    DI();
    
    CopyIsrHandler();
    
    FlashLibraryInit();
    FSL_ChangeInterruptTable(((fsl_u16)((__near uint16_t *) &ramIntHandler[0])) + HANDLER_SAFE_AREA_BYTES);
    
    EI();
}

void ReleaseInterruptHandlers()
{
    DI();
    
    FSL_RestoreInterruptTable();
    FlashLibraryRelease();
}
