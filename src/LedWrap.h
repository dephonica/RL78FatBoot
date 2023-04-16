#pragma once

#include "LedSchemes.h"

#define GSM_DEBUG_LED           P7_bit.no0
#define GSM_PDP_STATUS_LED      P5_bit.no0
#define GSM_DATA_STATUS_LED     P5_bit.no1

#define LED_SCHEME_IDLE         0

#define LED_ON                  1
#define LED_OFF                 0

__far const uint16_t* LedCurrentScheme;
uint8_t LedSchemeStep, LedSchemeIncrement;
uint8_t LedSchemeSize;
uint8_t LedSchemeLoop;

uint8_t LedDriverStates[3] = {LED_OFF, LED_OFF, LED_OFF};
uint8_t LedFlipFlop;

UserTimerContext LedSchemeTimerContext;

void LedInit()
{
    LedCurrentScheme = LED_SCHEME_IDLE;
    LedSchemeStep = 0;
    LedSchemeIncrement = 0;
}

void LedSetScheme(uint8_t ledScheme)
{
    LedCurrentScheme = LedSchemes[ledScheme];
    LedSchemeSize = LedSchemeSizes[ledScheme];
    LedSchemeStep = 1;
    LedSchemeIncrement = 1;
    LedSchemeLoop = LedCurrentScheme[0];
    
    UserTimerSetPeriod(&LedSchemeTimerContext, 1);
    UserTimerStart(&LedSchemeTimerContext);
}

void LedTick()
{
    // LED driver
    LedFlipFlop = !LedFlipFlop;
    
    GSM_DEBUG_LED = LedDriverStates[0];
    
    if (LedFlipFlop)
    {
        if (LedDriverStates[2] == LED_OFF)
        {
            GSM_PDP_STATUS_LED = LED_OFF;
            GSM_DATA_STATUS_LED = LED_ON;
        } else
        {
            GSM_PDP_STATUS_LED = LED_OFF;
            GSM_DATA_STATUS_LED = LED_OFF;
        }
    } else
    {
        if (LedDriverStates[1] == LED_OFF)
        {
            GSM_PDP_STATUS_LED = LED_ON;
            GSM_DATA_STATUS_LED = LED_OFF;
        } else
        {
            GSM_PDP_STATUS_LED = LED_OFF;
            GSM_DATA_STATUS_LED = LED_OFF;
        }
    }
    
    // LED animation
    if (LedCurrentScheme == LED_SCHEME_IDLE)
    {
        LedDriverStates[0] = 1;
        LedDriverStates[1] = 1;
        LedDriverStates[2] = 1;
        
        return;
    }
 
    UserTimerTick(&LedSchemeTimerContext);
    
    if (UserTimerIsElapsed(&LedSchemeTimerContext))
    {
        LedDriverStates[0] = 1 - LedCurrentScheme[LedSchemeStep];
        LedSchemeStep += LedSchemeIncrement;
        LedDriverStates[1] = 1 - LedCurrentScheme[LedSchemeStep];
        LedSchemeStep += LedSchemeIncrement;
        LedDriverStates[2] = 1 - LedCurrentScheme[LedSchemeStep];
        LedSchemeStep += LedSchemeIncrement;
        
        UserTimerSetPeriod(&LedSchemeTimerContext, LedCurrentScheme[LedSchemeStep]);
        LedSchemeStep += LedSchemeIncrement;
        
        if (LedSchemeStep >= LedSchemeSize ||
            LedSchemeStep < 1)
        {
            if (LedSchemeLoop == LED_SCHEME_REPEAT_PINGPONG)
            {
                LedSchemeIncrement = -LedSchemeIncrement;
                if (LedSchemeStep < 1)
                {
                    LedSchemeStep = 2;
                } else
                {
                    LedSchemeStep = LedSchemeSize - 2;
                }
            } else if (LedSchemeLoop == LED_SCHEME_REPEAT_LOOP)
            {
                LedSchemeStep = 1;
            } else if (LedSchemeLoop == LED_SCHEME_REPEAT_ONCE)
            {
                LedCurrentScheme = LED_SCHEME_IDLE;
            }
        }
    }
}

void LedOff()
{
    LedCurrentScheme = LED_SCHEME_IDLE;
    LedTick();
    LedTick();
}

void LedAnimateScheme(uint8_t ledScheme, uint16_t timeout)
{
    UserTimerContext timeoutTimer;
    UserTimerInit(&timeoutTimer, timeout);
    UserTimerStart(&timeoutTimer);

    LedSetScheme(ledScheme);
    
    while (!UserTimerIsElapsed(&timeoutTimer))
    {
        R_WDT_Restart();
        
        UserTimerTick(&timeoutTimer);
        LedTick();
    }
    
    LedOff();
}
