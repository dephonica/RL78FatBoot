#pragma once

void Memset(void* p, uint8_t value, uint16_t length)
{
    uint8_t* s = (uint8_t*) p;
    
    for (uint16_t i = 0; i < length; i++)
    {
        *s = value;
        s = s + 1;
    }
}

bool Strcmp(const char* string1, const char* string2)
{
    const char* pointer1 = string1;
    const char* pointer2 = string2;
    
    while (1)
    {
        if (*pointer1 != *pointer2)
        {
            return TRUE;
        }
        
        if (*pointer1 == 0)
        {
            break;
        }
        
        pointer1 += 1;
        pointer2 += 1;
    }
    
    return FALSE;
}

char* AppendStrings(char *target, const char* string)
{
    while (*string)
    {
        *target = *string;
        target += 1;
        string += 1;
    }
    
    return target;
}

void AppendStrings2(char *targetBuffer, const char* string1, const char* string2)
{
    char *target = AppendStrings(targetBuffer, string1);
    target = AppendStrings(target, string2);
    *target = 0;
}

void AppendStrings3(char *targetBuffer, const char* string1, const char* string2, const char* string3)
{
    char *target = AppendStrings(targetBuffer, string1);
    target = AppendStrings(target, string2);
    target = AppendStrings(target, string3);
    *target = 0;
}

void AppendStrings4(char *targetBuffer, const char* string1, const char* string2, 
                    const char* string3, const char* string4)
{
    char *target = AppendStrings(targetBuffer, string1);
    target = AppendStrings(target, string2);
    target = AppendStrings(target, string3);
    target = AppendStrings(target, string4);
    *target = 0;
}

void AppendStrings5(char *targetBuffer, const char* string1, const char* string2, 
                    const char* string3, const char* string4, const char* string5)
{
    char *target = AppendStrings(targetBuffer, string1);
    target = AppendStrings(target, string2);
    target = AppendStrings(target, string3);
    target = AppendStrings(target, string4);
    target = AppendStrings(target, string5);
    *target = 0;
}

void AppendStrings6(char *targetBuffer, const char* string1, const char* string2, 
                    const char* string3, const char* string4, const char* string5, 
                    const char* string6)
{
    char *target = AppendStrings(targetBuffer, string1);
    target = AppendStrings(target, string2);
    target = AppendStrings(target, string3);
    target = AppendStrings(target, string4);
    target = AppendStrings(target, string5);
    target = AppendStrings(target, string6);
    *target = 0;
}


bool StartsWith(const char* string1, const char* string2)
{
    const char* pointer1 = string1;
    const char* pointer2 = string2;
    
    while (1)
    {
        if (!*pointer2)
        {
            return TRUE;
        }
        
        if (*pointer1 != *pointer2)
        {
            return FALSE;
        }
        
        if (*pointer1 == 0)
        {
            break;
        }
        
        pointer1 += 1;
        pointer2 += 1;
    }
    
    return FALSE;
}

int16_t StringToInt16(const char* string)
{
    int16_t res = 0;
 
    for (uint8_t i = 0; 
            string[i] != '\0' && string[i] >= '0' && string[i] <= '9';
            ++i)
    {
        res = res * 10 + string[i] - '0';
    }
 
    return res;
}

int32_t StringToInt32(const char* string)
{
    int32_t res = 0;
 
    for (uint8_t i = 0; 
            string[i] != '\0' && string[i] >= '0' && string[i] <= '9';
            ++i)
    {
        res = res * 10 + string[i] - '0';
    }
 
    return res;
}
