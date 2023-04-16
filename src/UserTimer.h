#pragma once

extern volatile uint32_t __timerZeroTickCounter;

typedef struct __UserTimerContext
{
  bool enabled_, elapsed_;
  uint32_t period_;
  uint32_t nextTimerTick_;

  void (*CallbackWithTimestamp_)(uint32_t timestamp);
} UserTimerContext;

void UserTimerInit(UserTimerContext* timerContext, uint32_t period)
{
    timerContext->period_ = period;
    timerContext->enabled_ = FALSE;
    timerContext->elapsed_ = FALSE;
    
    timerContext->CallbackWithTimestamp_ = 0;
}

void UserTimerStart(UserTimerContext* timerContext)
{
    timerContext->nextTimerTick_ = __timerZeroTickCounter + timerContext->period_;
    timerContext->enabled_ = TRUE;
    timerContext->elapsed_ = FALSE;
}

void UserTimerStop(UserTimerContext* timerContext)
{
    timerContext->enabled_ = FALSE;
}

void UserTimerTick(UserTimerContext* timerContext)
{
    if (!timerContext->enabled_)
    {
        return;
    }
    
    if (__timerZeroTickCounter >= timerContext->nextTimerTick_)
    {
      if (timerContext->CallbackWithTimestamp_)
      {
        timerContext->CallbackWithTimestamp_(timerContext->nextTimerTick_);
      }
      
      timerContext->nextTimerTick_ += timerContext->period_;
      timerContext->elapsed_ = TRUE;
    }
}

void UserTimerSetPeriod(UserTimerContext* timerContext, uint32_t period)
{
    timerContext->nextTimerTick_ -= timerContext->period_;
    timerContext->period_ = period;
    timerContext->nextTimerTick_ += timerContext->period_;
}
  
bool UserTimerIsElapsed(UserTimerContext* timerContext)
{
    if (timerContext->elapsed_)
    {
      timerContext->elapsed_ = FALSE;
      return TRUE;
    }
    
    return FALSE;
}
  
bool UserTimerIsEnabled(UserTimerContext* timerContext)
{
    return timerContext->enabled_;
}
  
void UserTimerSetCallback(UserTimerContext* timerContext, void (*callbackWithTimestamp)(uint32_t timestamp))
{
    timerContext->CallbackWithTimestamp_ = callbackWithTimestamp;
}
