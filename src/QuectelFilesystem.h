#pragma once

#define QUECTEL_DEFAULT_TIMEOUT         5000
#define QUECTEL_WRITE_TIMEOUT           5000
#define QUECTEL_READ_TIMEOUT            5000
#define WAIT_FOR_OK_TIMEOUT_MS          5000

#define QUECTEL_FILE_OPEN_ALWAYS        0
#define QUECTEL_FILE_OPEN_CREATE        1
#define QUECTEL_FILE_OPEN_EXISTING      2

#define QUECTEL_FILE_SEEK_BEGIN         0
#define QUECTEL_FILE_SEEK_CURRENT       1
#define QUECTEL_FILE_SEEK_END           2

#define QUECTEL_FILE_SUCCESS            0
#define QUECTEL_FILE_ERR                -1
#define QUECTEL_FILE_ERR_UNEXPECTED     -2
#define QUECTEL_FILE_FAILED             -3
#define QUECTEL_FILE_BIN_FAILED         -4

uint16_t QuectelFileLastError;

void PrintCommand(const char *command)
{
    printf("Command: ");
    printf(command);
    printf("\r\n");
}

void PrintReply(const char *reply)
{
    printf("Reply: ");
    printf(reply);
    printf("\r\n");
}

int8_t HandleError(const char* reply)
{
    if (StartsWith(reply, "+CME ERROR:"))
    {
        QuectelFileLastError = StringToInt16(&reply[12]);
        return QUECTEL_FILE_ERR;
    }
    
    return QUECTEL_FILE_SUCCESS;
}

int8_t QuectelFileOpen(ModemContext* modemContext, const char* fileName, uint8_t openMode)
{
    ModemFlushBuffers(modemContext);
    
    QuectelFileLastError = QUECTEL_FILE_SUCCESS;
    
    char openCommand[128];
    char openModeString[2] = {'0' + openMode, 0};
    
    AppendStrings4(openCommand, "AT+QFOPEN=\"", fileName, "\",", openModeString);
    PrintCommand(openCommand);
    
    ModemPushCommandSync(modemContext, openCommand, QUECTEL_DEFAULT_TIMEOUT);
    
    const char *fopenReply = ModemGetLastReply(modemContext);
    PrintReply(fopenReply);

    if (StartsWith(fopenReply, "+QFOPEN:"))
    {
        uint8_t fileHandle = (uint8_t) StringToInt16(&fopenReply[9]);
        
        if (ModemWaitForString(modemContext, "OK", WAIT_FOR_OK_TIMEOUT_MS))
        {
            return fileHandle;
        }
        
        return QUECTEL_FILE_FAILED;
    }

    return HandleError(fopenReply)
        ? QUECTEL_FILE_ERR
        : QUECTEL_FILE_ERR_UNEXPECTED;
}

int8_t QuectelFileClose(ModemContext* modemContext, uint8_t fileHandle)
{
    ModemFlushBuffers(modemContext);
    
    char closeCommand[32];
    
    char fileHandleString[8];
    itoa(fileHandle, fileHandleString, 10);
    
    AppendStrings2(closeCommand, "AT+QFCLOSE=", fileHandleString);
    PrintCommand(closeCommand);
    
    ModemPushCommandSync(modemContext, closeCommand, QUECTEL_DEFAULT_TIMEOUT);

    const char *fcloseReply = ModemGetLastReply(modemContext);
    PrintReply(fcloseReply);
    
    if (StartsWith(fcloseReply, "OK"))
    {
        return QUECTEL_FILE_SUCCESS;
    }
    
    return HandleError(fcloseReply)
        ? QUECTEL_FILE_ERR
        : QUECTEL_FILE_ERR_UNEXPECTED;
}

int8_t QuectelFileSeek(ModemContext* modemContext, uint8_t fileHandle, uint32_t offset, uint8_t position)
{
    ModemFlushBuffers(modemContext);    
    
    char seekCommand[32];
    
    char fileHandleString[8], offsetString[16], positionString[2] = {'0' + position, 0};
    itoa(fileHandle, fileHandleString, 10);
    itoa(offset, offsetString, 10);
    
    AppendStrings6(seekCommand, "AT+QFSEEK=", fileHandleString, ",", offsetString, ",", positionString);
    PrintCommand(seekCommand);
    
    ModemPushCommandSync(modemContext, seekCommand, QUECTEL_DEFAULT_TIMEOUT);

    const char *fseekReply = ModemGetLastReply(modemContext);
    PrintReply(fseekReply);
    
    if (StartsWith(fseekReply, "OK"))
    {
        return QUECTEL_FILE_SUCCESS;
    }
    
    return HandleError(fseekReply)
        ? QUECTEL_FILE_ERR
        : QUECTEL_FILE_ERR_UNEXPECTED;
}

int32_t QuectelFilePosition(ModemContext* modemContext, uint8_t fileHandle)
{
    ModemFlushBuffers(modemContext);
    
    char positionCommand[32];
    
    char fileHandleString[8];
    itoa(fileHandle, fileHandleString, 10);
    
    AppendStrings2(positionCommand, "AT+QFPOSITION=", fileHandleString);
    PrintCommand(positionCommand);
    
    ModemPushCommandSync(modemContext, positionCommand, QUECTEL_DEFAULT_TIMEOUT);

    const char *fpositionReply = ModemGetLastReply(modemContext);
    PrintReply(fpositionReply);
    
    if (StartsWith(fpositionReply, "+QFPOSITION:"))
    {
        int32_t position = StringToInt32(&fpositionReply[13]);

        if (ModemWaitForString(modemContext, "OK", WAIT_FOR_OK_TIMEOUT_MS))
        {
            return position;
        }
        
        return QUECTEL_FILE_FAILED;
    }
    
    return HandleError(fpositionReply)
        ? QUECTEL_FILE_ERR
        : QUECTEL_FILE_ERR_UNEXPECTED;
}

int32_t QuectelFileSize(ModemContext* modemContext, uint8_t fileHandle)
{
    if (QuectelFileSeek(modemContext, fileHandle, 0, 2) < 0)
    {
        return QUECTEL_FILE_ERR;
    }
    
    int32_t fileSize = QuectelFilePosition(modemContext, fileHandle);
    if (fileSize < 0)
    {
        return QUECTEL_FILE_ERR;
    }
    
    if (QuectelFileSeek(modemContext, fileHandle, 0, 0) < 0)
    {
        return QUECTEL_FILE_ERR;
    }
    
    return fileSize;
}

int8_t QuectelFileWrite(ModemContext* modemContext, uint8_t fileHandle, 
    const uint8_t* data, uint32_t dataSize)
{
    ModemFlushBuffers(modemContext);
    
    char writeCommand[32];
    
    char fileHandleString[8], sizeString[16], timeoutString[8];
    itoa(fileHandle, fileHandleString, 10);
    itoa(dataSize, sizeString, 10);
    itoa(QUECTEL_WRITE_TIMEOUT / 1000, timeoutString, 10);
    
    AppendStrings6(writeCommand, "AT+QFWRITE=", fileHandleString, ",", sizeString, ",", timeoutString);
    PrintCommand(writeCommand);
    
    ModemPushCommandSync(modemContext, writeCommand, QUECTEL_DEFAULT_TIMEOUT);

    const char *fwriteReply = ModemGetLastReply(modemContext);
    PrintReply(fwriteReply);
    
    if (StartsWith(fwriteReply, "CONNECT"))
    {
        ModemPushBinaryData(modemContext, data, dataSize);
        if (ModemWaitForString(modemContext, "+QFWRITE:", QUECTEL_DEFAULT_TIMEOUT))
        {
            fwriteReply = ModemGetLastReply(modemContext);
            PrintReply(fwriteReply);
            uint32_t writeSize = StringToInt32(&fwriteReply[10]);
            
            if (ModemWaitForString(modemContext, "OK", WAIT_FOR_OK_TIMEOUT_MS))
            {
                return writeSize;
            }

            return HandleError(ModemGetLastReply(modemContext))
                ? QUECTEL_FILE_FAILED
                : QUECTEL_FILE_ERR_UNEXPECTED;
        }
    }
    
    return HandleError(fwriteReply)
        ? QUECTEL_FILE_ERR
        : QUECTEL_FILE_ERR_UNEXPECTED;
}

int8_t QuectelFileRead(ModemContext* modemContext, uint8_t fileHandle, 
    uint8_t* readBuffer, uint32_t readSize)
{
    ModemFlushBuffers(modemContext);
    
    char readCommand[32];
    
    char fileHandleString[8], sizeString[16];
    itoa(fileHandle, fileHandleString, 10);
    itoa(readSize, sizeString, 10);
    
    AppendStrings4(readCommand, "AT+QFREAD=", fileHandleString, ",", sizeString);
    PrintCommand(readCommand);
    
    ModemPushCommandSync(modemContext, readCommand, QUECTEL_DEFAULT_TIMEOUT);

    const char *freadReply = ModemGetLastReply(modemContext);
    
    if (StartsWith(freadReply, "CONNECT"))
    {
        uint32_t readLength = StringToInt32(&freadReply[8]);
        ModemSwitchToBinaryMode(modemContext, readBuffer, readLength);
        if (!ModemWaitForBinaryData(modemContext, QUECTEL_READ_TIMEOUT))
        {
            return QUECTEL_FILE_BIN_FAILED;
        }
        
        if (ModemWaitForString(modemContext, "OK", WAIT_FOR_OK_TIMEOUT_MS))
        {
            return readLength;
        }

        return HandleError(ModemGetLastReply(modemContext))
            ? QUECTEL_FILE_FAILED
            : QUECTEL_FILE_ERR_UNEXPECTED;
    }
    
    return HandleError(freadReply)
        ? QUECTEL_FILE_ERR
        : QUECTEL_FILE_ERR_UNEXPECTED;
}

int8_t QuectelFileDelete(ModemContext* modemContext, const char* fileName)
{
    char deleteCommand[128];
    
    AppendStrings3(deleteCommand, "AT+QFDEL=\"", fileName, "\"");
    PrintCommand(deleteCommand);
    
    ModemPushCommandSync(modemContext, deleteCommand, QUECTEL_DEFAULT_TIMEOUT);
    
    const char *deleteReply = ModemGetLastReply(modemContext);
    PrintReply(deleteReply);

    if (StartsWith(deleteReply, "OK"))
    {
        return QUECTEL_FILE_SUCCESS;
    }
    
    return HandleError(deleteReply)
        ? QUECTEL_FILE_ERR
        : QUECTEL_FILE_ERR_UNEXPECTED;

}
