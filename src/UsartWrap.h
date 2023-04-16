#pragma once

#include "StringHelpers.h"

extern volatile void (*IntCallbackUart1OnReceived)(uint8_t rxByte);
extern volatile void (*IntCallbackUart2OnReceived)(uint8_t rxByte);

typedef struct _UartContext
{
    uint8_t uartIndex;
    uint8_t rxByte;
    uint32_t bytesReceived;
    void (*callbackOnReceived)(void* uartContext);
    void *userData;
} UartContext;

UartContext _uart1Context, _uart2Context;

void UartInit()
{
    Memset(&_uart1Context, 0, sizeof(UartContext));
    Memset(&_uart2Context, 0, sizeof(UartContext));
}

void UartSendByte(uint8_t uartIndex, int8_t charValue)
{
    switch (uartIndex)
    {
        case 1:
            R_UART1_Send_Byte(charValue);
            break;
        case 2:
            R_UART2_Send_Byte(charValue);
            break;
    }
}

void UartSendString(uint8_t uartIndex, const char stringValue[])
{
    switch (uartIndex)
    {
        case 1:
            R_UART1_Send_String(stringValue);
            break;
        case 2:
            R_UART2_Send_String(stringValue);
            break;
    }
}

void UartSendBuffer(uint8_t uartIndex, const uint8_t buffer[], uint32_t bufferLength)
{
    switch (uartIndex)
    {
        case 1:
            R_UART1_Send_Buffer(buffer, bufferLength);
            break;
        case 2:
            R_UART2_Send_Buffer(buffer, bufferLength);
            break;
    }
}

volatile void _IntUart1HandleChar(uint8_t charValue)
{
    _uart1Context.rxByte = charValue;
    _uart1Context.bytesReceived++;
    
    if (_uart1Context.callbackOnReceived)
    {
        _uart1Context.callbackOnReceived(&_uart1Context);
    }
}

volatile void _IntUart2HandleChar(uint8_t charValue)
{
    _uart2Context.rxByte = charValue;
    _uart2Context.bytesReceived++;
    
    if (_uart2Context.callbackOnReceived)
    {
        _uart2Context.callbackOnReceived(&_uart2Context);
    }
}

void UartListen(uint8_t uartIndex, 
    void (*IntCallbackHandleChar)(void* uartContext),
    void* userData)
{
    DI();
    
    IntCallbackUart1OnReceived = &_IntUart1HandleChar;
    IntCallbackUart2OnReceived = &_IntUart2HandleChar;
    
    _uart1Context.uartIndex = 1;
    _uart2Context.uartIndex = 2;
    
    switch (uartIndex)
    {
        case 1:
            _uart1Context.callbackOnReceived = IntCallbackHandleChar;
            _uart1Context.userData = userData;
            break;
        case 2:
            _uart2Context.callbackOnReceived = IntCallbackHandleChar;
            _uart2Context.userData = userData;
            break;
    }
    
    EI();
}

void UartStopListening(uint8_t uartIndex)
{
    DI();

    switch (uartIndex)
    {
        case 1:
            _uart1Context.callbackOnReceived = 0;
            _uart1Context.userData = 0;
            break;
        case 2:
            _uart2Context.callbackOnReceived = 0;
            _uart2Context.userData = 0;
            break;
    }
    
    EI();
}
