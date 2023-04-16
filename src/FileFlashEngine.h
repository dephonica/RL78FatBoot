#pragma once

#define STORED_UPDATE_FILE_NAME             "OTA_BIN.bin"
#define INTERMEDIATE_BUFFER_SIZE_BYTES      512
#define MIN_UPDATE_SIZE                     100

bool CheckUpdateCrc(ModemContext* modemContext, int8_t fileHandle, uint32_t fileSize)
{
    QuectelFileSeek(modemContext, fileHandle, 0, QUECTEL_FILE_SEEK_BEGIN);
    
    if (fileSize < MIN_UPDATE_SIZE ||
        fileSize > MAX_FIRMWARE_IMAGE_SIZE_BYTES)
    {
        return FALSE;
    }
    
    uint8_t crcBuffer[INTERMEDIATE_BUFFER_SIZE_BYTES];
    
    uint32_t fileBytesRemain = fileSize;
    
    uint32_t crc32 = 0xFFFFFFFF;
    
    uint32_t referenceCrc32 = 0;
    bool isReferenceCrc32Fetched = FALSE;
    
    while (fileBytesRemain > 0)
    {
        uint32_t bytesToRead = fileBytesRemain;
        if (bytesToRead > INTERMEDIATE_BUFFER_SIZE_BYTES)
        {
            bytesToRead = INTERMEDIATE_BUFFER_SIZE_BYTES;
        }
        
        if (QuectelFileRead(modemContext, fileHandle, crcBuffer, bytesToRead) < 0)
        {
            return FALSE;
        }

        fileBytesRemain -= bytesToRead;
        
        uint8_t* crcBufferPointer = crcBuffer;
        if (!isReferenceCrc32Fetched)
        {
            isReferenceCrc32Fetched = TRUE;
            
            uint32_t firmwareSizeBytes = *((uint32_t*) crcBufferPointer);
            referenceCrc32 = *((uint32_t*) crcBufferPointer + 4);
            
            if (fileSize != (firmwareSizeBytes + 8))
            {
                return FALSE;
            }
            
            crcBufferPointer += 8;
            bytesToRead -= 8;
        }
        
        crc32 = Crc32Near(crcBufferPointer, bytesToRead, crc32);
    }
    
    return crc32 == referenceCrc32;
}

bool FlashFromFile(ModemContext* modemContext, int8_t fileHandle, uint32_t fileSize, uint32_t targetAddress)
{
    QuectelFileSeek(modemContext, fileHandle, 0, QUECTEL_FILE_SEEK_BEGIN);

    if (fileSize > MAX_FIRMWARE_IMAGE_SIZE_BYTES)
    {
        return FALSE;
    }
    
    uint8_t blockBuffer[FLASH_BLOCK_SIZE_BYTES];
    
    uint32_t targetAddressBase = targetAddress;
    uint32_t firmwareBytesRemain = fileSize;
    
    uint32_t referenceCrc32 = 0;
    bool isReferenceCrc32Fetched = FALSE;
    
    while (firmwareBytesRemain > 0)
    {
        R_WDT_Restart();
        
        uint32_t bytesToCopy = firmwareBytesRemain;
        if (bytesToCopy > FLASH_BLOCK_SIZE_BYTES)
        {
            bytesToCopy = FLASH_BLOCK_SIZE_BYTES;
        } else
        {
            Memset(&blockBuffer, 0xff, FLASH_BLOCK_SIZE_BYTES);
        }

        firmwareBytesRemain -= bytesToCopy;
        
        if (QuectelFileRead(modemContext, fileHandle, blockBuffer, bytesToCopy) < 0)
        {
            return FALSE;
        }

        if (isReferenceCrc32Fetched == FALSE)
        {
            isReferenceCrc32Fetched = TRUE;
            
            uint32_t firmwareSizeBytes = *((uint32_t*) &blockBuffer);
            referenceCrc32 = *((uint32_t*) &blockBuffer[4]);
    
            if (fileSize != (firmwareSizeBytes + 8))
            {
                return FALSE;
            }
        }
        
        if (!FlashEngineWriteBlockAndVerify(targetAddress, blockBuffer))
        {
            return FALSE;
        }
        
        targetAddress += FLASH_BLOCK_SIZE_BYTES;
    }
    
    return Crc32((__far const uint8_t*) (targetAddressBase + 8), fileSize - 8, 0xFFFFFFFF) == referenceCrc32;
}

bool TryToFlashWithFile(ModemContext* modemContext, int8_t fileHandle, uint8_t firmwareOption)
{
    int32_t fileSize = QuectelFileSize(modemContext, fileHandle);
        
    if (!CheckUpdateCrc(modemContext, fileHandle, fileSize))
    {
        return FALSE;
    }

    uint32_t firmwareAddress = firmwareOption == FIRMWARE_A
        ? A_UPDATE_START_ADDRESS
        : B_UPDATE_START_ADDRESS;

    return FlashFromFile(modemContext, fileHandle, fileSize, firmwareAddress);
}

bool ProbeQuectelFileFlashing(ModemContext* modemContext, ControlBlockUnion* controlBlock, uint8_t firmwareToUpdate)
{
    int8_t fileHandle = QuectelFileOpen(modemContext, STORED_UPDATE_FILE_NAME, QUECTEL_FILE_OPEN_EXISTING);
    if (fileHandle < 0)
    {
        return FALSE;
    }

    TryToFlashWithFile(modemContext, fileHandle, firmwareToUpdate);
    
    QuectelFileClose(modemContext, fileHandle);
    
    if (firmwareToUpdate == FIRMWARE_A)
    {
        controlBlock->ControlBlock.versionOfFirmwareA++;
    } else if (firmwareToUpdate == FIRMWARE_B)
    {
        controlBlock->ControlBlock.versionOfFirmwareB++;
    }
    
    WriteControlBlock(controlBlock);
    
    return TRUE;
}
