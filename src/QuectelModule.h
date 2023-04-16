#pragma once

#define WAIT_FOR_RDY_TIMEOUT_MS   30000

#define GSM_POWER_KEY             P2_bit.no7
#define GSM_POWER_ENABLE          P14_bit.no7  
#define PS_SYNC                   P14_bit.no6

#define GSM_RESET                 P2_bit.no6
#define GSM_WAKEUP_IN             P2_bit.no5
#define GSM_W_DISABLE             P2_bit.no4

void QuectelPowerOn()
{
    GSM_POWER_ENABLE = 1;   // GSM-P-EN (Enable input (1 enabled, 0 disabled))
    PS_SYNC = 1;            // PS-SYNC (Enable/disable power save mode (1 disabled, 0 enabled))
    GSM_POWER_KEY = 0;
    GSM_RESET = 0;
    
    DelayMs(500);           // Delay for Power Supply to stabilise
    
    GSM_POWER_KEY = 1;      // GSM power key is Enabled
    DelayMs(150);           // 150 mSec delay
    GSM_POWER_KEY = 0;      // GSM power key is Disabled
}

void QuectelPowerOff()
{
    GSM_POWER_KEY = 1;      // GSM power key is Enabled
    DelayMs(700);           // 700 mSec delay
    GSM_POWER_KEY = 0;      // GSM power key is Disabled
    
    GSM_POWER_ENABLE = 0;
    PS_SYNC     = 0;
    DelayMs(2000); 
}

bool QuectelWaitForRdy(ModemContext* modemContext)
{
    bool isReady = ModemWaitForString(modemContext, "RDY", WAIT_FOR_RDY_TIMEOUT_MS);
    if (!isReady)
    {
        return FALSE;
    }
    
    ModemPushCommandAsync(modemContext, "AT");
    return ModemWaitForString(modemContext, "AT", WAIT_FOR_RDY_TIMEOUT_MS) &&
        ModemWaitForString(modemContext, "OK", WAIT_FOR_RDY_TIMEOUT_MS);
}

bool QuectelSwitchOffEcho(ModemContext* modemContext)
{
    ModemPushCommandAsync(modemContext, "ATE0");
    return ModemWaitForString(modemContext, "ATE0", WAIT_FOR_RDY_TIMEOUT_MS) &&
        ModemWaitForString(modemContext, "OK", WAIT_FOR_RDY_TIMEOUT_MS);
}
