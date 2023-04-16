#pragma once

#include "StringHelpers.h"

#define INT_MODEM_BUFFER_SIZE       128
#define AT_REPLY_BUFFER_SIZE        256

#define MODEM_MODE_AT               1
#define MODEM_MODE_BINARY           2

typedef struct __ModemContext
{
    uint8_t uartIndex;
    volatile uint8_t incomePointerLow, incomePointerHigh;
    volatile uint8_t incomeBuffer[INT_MODEM_BUFFER_SIZE];

    volatile uint8_t atReplyPointer;
    volatile uint8_t atReplyBuffer[AT_REPLY_BUFFER_SIZE];

    volatile uint32_t atCommandCount;
    
    volatile uint8_t modemMode;
    
    uint8_t* binaryReadBuffer;
    uint32_t binaryReadLength;
    uint32_t binaryReadPosition;
    
    volatile void (*onAtCommandReply)(const char *replyString);
} ModemContext;

uint8_t ModemGetIncomeBufferDataSize(volatile ModemContext* modemContext)
{
    if (modemContext->incomePointerHigh >= modemContext->incomePointerLow)
    {
        return modemContext->incomePointerHigh - modemContext->incomePointerLow;
    }
    
    return INT_MODEM_BUFFER_SIZE - modemContext->incomePointerLow + modemContext->incomePointerHigh;
}

void OnByteFromModem(void* uartContextPointer)
{
    UartContext* uartContext = (UartContext*) uartContextPointer;
    volatile ModemContext* modemContext = (ModemContext*) uartContext->userData;
    
    if (modemContext)
    {
        if (modemContext->modemMode == MODEM_MODE_AT)
        {
            // Handle interrupt in AT mode
            if (ModemGetIncomeBufferDataSize(modemContext) < INT_MODEM_BUFFER_SIZE)
            {
                modemContext->incomeBuffer[modemContext->incomePointerHigh] = uartContext->rxByte;
                modemContext->incomePointerHigh++;
            
                if (modemContext->incomePointerHigh >= INT_MODEM_BUFFER_SIZE)
                {
                    modemContext->incomePointerHigh = 0;
                }
            }
        } else
        {
            // Handle interrupt in BINARY mode
            modemContext->binaryReadBuffer[modemContext->binaryReadPosition] = uartContext->rxByte;
            modemContext->binaryReadPosition++;
        
            if (modemContext->binaryReadPosition >= modemContext->binaryReadLength)
            {
                modemContext->modemMode = MODEM_MODE_AT;
            }
        }
    }
}

void ModemAttachToUart(volatile ModemContext* modemContext, uint8_t uartIndex)
{
    Memset((void*) modemContext, 0, sizeof(ModemContext));
    
    modemContext->uartIndex = uartIndex;
    modemContext->incomePointerLow = 0;
    modemContext->incomePointerHigh = 0;
    modemContext->modemMode = MODEM_MODE_AT;
    
    UartListen(uartIndex, &OnByteFromModem, (void*) modemContext);
}

void ModemDetachFromUart(volatile ModemContext* modemContext)
{
    UartStopListening(modemContext->uartIndex);
}

void ModemSetAtReplyCallback(volatile ModemContext* modemContext, 
                             volatile void (*onAtCommandReply)(const char *replyString))
{
    DI();
    
    modemContext->onAtCommandReply = onAtCommandReply;
    
    EI();
}

void ModemTickAtMode(volatile ModemContext* modemContext)
{
    DI();
    
    while (modemContext->incomePointerLow != modemContext->incomePointerHigh)
    {
        uint8_t rxByte = modemContext->incomeBuffer[modemContext->incomePointerLow];
        
        modemContext->incomePointerLow++;
        if (modemContext->incomePointerLow >= INT_MODEM_BUFFER_SIZE)
        {
            modemContext->incomePointerLow = 0;
        }
        
        if (rxByte == 0x0d)
        {
            continue;
        }
        
        if (rxByte == 0x0a)
        {
            if (modemContext->atReplyPointer > 0)
            {
                EI();
                
                modemContext->atReplyBuffer[modemContext->atReplyPointer] = 0;
                modemContext->atCommandCount++;

                if (modemContext->onAtCommandReply)
                {
                    modemContext->onAtCommandReply((const char*)modemContext->atReplyBuffer);
                }
                
                modemContext->atReplyPointer = 0;
                
                break;
            }
            
            modemContext->atReplyPointer = 0;
        } else
        {
            if (modemContext->atReplyPointer < AT_REPLY_BUFFER_SIZE)
            {
                modemContext->atReplyBuffer[modemContext->atReplyPointer] = rxByte;
                modemContext->atReplyPointer++;
            }
        }
    }
    
    EI();    
}

void ModemTick(volatile ModemContext* modemContext)
{
    if (modemContext->modemMode != MODEM_MODE_AT)
    {
        return;
    }
    
    ModemTickAtMode(modemContext);
}

void ModemFlushBuffers(volatile ModemContext* modemContext)
{
    DI();
    
    modemContext->incomePointerLow = 0;
    modemContext->incomePointerHigh = 0;
    modemContext->atReplyPointer = 0;
    modemContext->binaryReadLength = 0;
    modemContext->binaryReadPosition = 0;
    modemContext->modemMode = MODEM_MODE_AT;
    
    EI();
}

const char* ModemGetLastReply(volatile ModemContext* modemContext)
{
    return (const char*) modemContext->atReplyBuffer;
}

void ModemPushBinaryData(volatile ModemContext* modemContext, const uint8_t* data, uint32_t dataSize)
{
    UartSendBuffer(modemContext->uartIndex, data, dataSize);
}

void ModemPushCommandAsync(volatile ModemContext* modemContext, const char* atCommand)
{
    UartSendString(modemContext->uartIndex, atCommand);
    UartSendByte(modemContext->uartIndex, '\r');
    UartSendByte(modemContext->uartIndex, '\n');
}

bool ModemPushCommandSync(volatile ModemContext* modemContext, const char* atCommand, uint16_t timeoutMs)
{
    ModemPushCommandAsync(modemContext, atCommand);

    void* oldOnAtCommandReply = (void*) modemContext->onAtCommandReply;
    modemContext->onAtCommandReply = 0;
    
    uint32_t waitForAtCommandCount = modemContext->atCommandCount + 1;
    
    UserTimerContext timeoutTimer;
    UserTimerInit(&timeoutTimer, timeoutMs);
    UserTimerStart(&timeoutTimer);
    
    bool isTimeout = FALSE;
    
    while (1)
    {
        R_WDT_Restart();
        
        ModemTick(modemContext);
        UserTimerTick(&timeoutTimer);
        
        if (modemContext->atCommandCount >= waitForAtCommandCount)
        {
            break;
        }
        
        if (UserTimerIsElapsed(&timeoutTimer))
        {
            isTimeout = TRUE;
            break;
        }
    }
    
    modemContext->onAtCommandReply = (volatile void(*)(const char*)) oldOnAtCommandReply;
    
    return !isTimeout;
}

bool ModemWaitForString(volatile ModemContext* modemContext, const char* waitForString, uint16_t timeoutMs)
{
    void* oldOnAtCommandReply = (void*) modemContext->onAtCommandReply;
    modemContext->onAtCommandReply = 0;
    
    UserTimerContext timeoutTimer;
    UserTimerInit(&timeoutTimer, timeoutMs);
    UserTimerStart(&timeoutTimer);
    
    bool isTimeout = FALSE;

    uint32_t currentAtCommandCount = modemContext->atCommandCount;
    
    while (1)
    {
        R_WDT_Restart();
        
        ModemTick(modemContext);
        UserTimerTick(&timeoutTimer);
        
        if (modemContext->atCommandCount > currentAtCommandCount)
        {
            currentAtCommandCount = modemContext->atCommandCount;
            
            if (StartsWith((char*) &modemContext->atReplyBuffer[0], waitForString))
            {
                break;
            }
        }
        
        if (UserTimerIsElapsed(&timeoutTimer))
        {
            isTimeout = TRUE;
            break;
        }
    }
    
    modemContext->onAtCommandReply = (volatile void(*)(const char*)) oldOnAtCommandReply;
    
    return !isTimeout;
}

void ModemSwitchToBinaryMode(volatile ModemContext* modemContext, 
    uint8_t* readBuffer, uint32_t readLength)
{
    DI();
    
    modemContext->modemMode = MODEM_MODE_BINARY;
    modemContext->binaryReadBuffer = readBuffer;
    modemContext->binaryReadLength = readLength;
    modemContext->binaryReadPosition = 0;
    
    uint8_t* bufferPointer = readBuffer;
    
    while (modemContext->incomePointerLow != modemContext->incomePointerHigh)
    {
        *bufferPointer = modemContext->incomeBuffer[modemContext->incomePointerLow];
        
        modemContext->binaryReadPosition++;
        
        modemContext->incomePointerLow++;
        if (modemContext->incomePointerLow >= INT_MODEM_BUFFER_SIZE)
        {
            modemContext->incomePointerLow = 0;
        }
    }
    
    EI();
}

bool ModemWaitForBinaryData(volatile ModemContext* modemContext, uint16_t timeoutMs)
{
    UserTimerContext timeoutTimer;
    UserTimerInit(&timeoutTimer, timeoutMs);
    UserTimerStart(&timeoutTimer);
    
    bool isTimeout = FALSE;

    while (1)
    {
        R_WDT_Restart();
        
        ModemTick(modemContext);
        UserTimerTick(&timeoutTimer);
        
        if (modemContext->modemMode == MODEM_MODE_AT)
        {
            break;
        }
        
        if (UserTimerIsElapsed(&timeoutTimer))
        {
            isTimeout = TRUE;
            break;
        }
    }
    
    return !isTimeout;
}
