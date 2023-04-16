#pragma once

#define LED_SCHEME_REPEAT_ONCE      0
#define LED_SCHEME_REPEAT_LOOP      1
#define LED_SCHEME_REPEAT_PINGPONG  2

uint16_t LedSchemeInit[] = {LED_SCHEME_REPEAT_ONCE,
                                 1,0,0,100, 0,0,0,100, 1,0,0,100, 0,0,0,100,
                                 0,1,0,100, 0,0,0,100, 0,1,0,100, 0,0,0,100,
                                 0,0,1,100, 0,0,0,100, 0,0,1,100, 0,0,0,100,
                                 1,1,1,100, 0,0,0,100, 1,1,1,100, 0,0,0,100};

uint16_t LedSchemeInitWait[] = {LED_SCHEME_REPEAT_LOOP,
                                 1,1,1,70, 0,0,0,70};
                                 
uint16_t LedSchemeError[] = {LED_SCHEME_REPEAT_LOOP,
                                 1,0,1,100, 0,1,0,50, 0,0,0,50, 0,1,0,50, 0,0,0,50};

uint16_t LedSchemeSwap[] = {LED_SCHEME_REPEAT_LOOP,
                                 1,0,1,500, 0,1,0,500};
                                 
#define LED_SCHEME_INIT         0
#define LED_SCHEME_INITWAIT     1
#define LED_SCHEME_ERROR        2
#define LED_SCHEME_SWAP         3

__far uint16_t* LedSchemes[] = {LedSchemeInit, LedSchemeInitWait, LedSchemeError, LedSchemeSwap};
uint16_t LedSchemeSizes[] = {sizeof(LedSchemeInit), sizeof(LedSchemeInitWait),
                                    sizeof(LedSchemeError), sizeof(LedSchemeSwap)};
                                    