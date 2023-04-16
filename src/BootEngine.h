#pragma once

#include "Debug.h"
#include "UsartWrap.h"
#include "UserTimer.h"
#include "LedWrap.h"
#include "ModemFrontend.h"
#include "QuectelModule.h"
#include "QuectelFilesystem.h"
#include "FlashEngine.h"
#include "UpdateManager.h"
#include "FileFlashEngine.h"
#include "UartFlashEngine.h"

ModemContext modemContext;

#define STARTUP_TIMEOUT_MS             2000
#define FIRMWARE_SWAP_TIMEOUT_MS       10000
#define POWER_KEY                      P7_bit.no1
#define KEY_PRESSED                    1

bool DetectFirmwareSwapMode()
{
    LedAnimateScheme(LED_SCHEME_INIT, STARTUP_TIMEOUT_MS);
    
    UserTimerContext swapTimeout;
    UserTimerInit(&swapTimeout, FIRMWARE_SWAP_TIMEOUT_MS);
    UserTimerStart(&swapTimeout);
    
    LedSetScheme(LED_SCHEME_INITWAIT);

    bool makeSwap = FALSE;
    while (POWER_KEY == KEY_PRESSED)
    {
        R_WDT_Restart();
        
        LedTick();
        UserTimerTick(&swapTimeout);
        
        if (UserTimerIsElapsed(&swapTimeout))
        {
            makeSwap = TRUE;
            break;
        }
    }
    
    LedOff();
    
    return makeSwap;
}

ControlBlockUnion controlBlock;

void BootEngineMainLoop()
{
    LedInit();
    
    // Probe control block
    ProbeControlBlock(&controlBlock);
    
    if (DetectFirmwareSwapMode())
    {
        UpdateManagerSwapFirmware(&controlBlock);
        LedAnimateScheme(LED_SCHEME_SWAP, 5000);
    }
    
    // Initialize communication hardware
    UartInit();
    ModemAttachToUart(&modemContext, 1);

    // Initialize Quectel EC25
    QuectelPowerOn();
    bool isReady = QuectelWaitForRdy(&modemContext);
    printf(isReady ? "Modem is ready\r\n" : "Modem is not ready\r\n");

    bool isEchoOff = QuectelSwitchOffEcho(&modemContext);
    printf(isEchoOff ? "AT Echo switched off\r\n" : "AT Echo is not switched off\r\n");

    // Handle firmware updates
    uint8_t firmwareToUpdate = GetFirmwareOptionToUpdate(&controlBlock);
    
    // Probe flashing options
    if (!ProbeUartFlashing(&controlBlock, firmwareToUpdate))
    {
        ProbeQuectelFileFlashing(&modemContext, &controlBlock, firmwareToUpdate);
    }
    
    // Handle available updates
    UpdateManagerRun(&controlBlock);
    
    // Release UART
    ModemDetachFromUart(&modemContext);
    
    QuectelPowerOff();    
}
