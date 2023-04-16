#pragma once

#ifndef R_WDT_Restart
    void R_WDT_Restart(void)
    {
        //WDTE = 0xACU;   /* restart watchdog timer */
    }
#endif

void reverse(char str[], int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end)
    {
        char temp = *(str+start);
        *(str+start) = *(str+end);
        *(str+end) = temp;
        
        start++;
        end--;
    }
}
 
char* itoa(int32_t num, char* str, int base)
{
    int i = 0;
    bool isNegative = FALSE;
 
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
 
    if (num < 0 && base == 10)
    {
        isNegative = TRUE;
        num = -num;
    }
 
    while (num != 0)
    {
        int32_t rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num / base;
    }
 
    if (isNegative)
    {
        str[i++] = '-';
    }
 
    str[i] = '\0';
 
    reverse(str, i);
 
    return str;
}

void printf(const char* string)
{
    R_UART2_Send_String(string);
}

void printfn(int number)
{
    char numBuff[16];
    itoa(number, numBuff, 10);
    
    R_UART2_Send_String(numBuff);
}
