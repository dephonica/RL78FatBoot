#pragma once

#include "fsl.h"
#include "fsl_types.h"

#include "Crc32.h"

#define FLASH_BLOCK_SIZE_BYTES             1024
#define MAX_FIRMWARE_IMAGE_SIZE_BYTES      0x25000  /* (151552) bytes */

#define PROGRAM_START_ADDRESS              0xa000
#define A_UPDATE_START_ADDRESS             0x2f000
#define B_UPDATE_START_ADDRESS             0x55000

#define FIRMWARE_0                         0
#define FIRMWARE_A                         1
#define FIRMWARE_B                         2

bool FlashEngineWriteBlockAndVerify(uint32_t address, uint8_t data[])
{
    if ((address % FLASH_BLOCK_SIZE_BYTES) != 0)
    {
        return FALSE;
    }

    R_WDT_Restart();

    uint16_t blockIndex = address / FLASH_BLOCK_SIZE_BYTES;

    uint8_t eraseResult = FSL_Erase(blockIndex);
    if (eraseResult != FSL_OK)
    {
        return FALSE;
    }
    
    const uint8_t chunkSizeBytes = 128;

    fsl_write_t fslWriteStruct;   
    
    __near uint8_t* dataPointer = data;
    uint32_t writeToAddress = address;
    
    DI();
    
    for (uint8_t i = 0; i < FLASH_BLOCK_SIZE_BYTES / chunkSizeBytes; i++)
    {
        R_WDT_Restart();
        
        fslWriteStruct.fsl_data_buffer_p_u08 = (__near fsl_u08*) dataPointer;
        fslWriteStruct.fsl_word_count_u08 = chunkSizeBytes / 4;
        fslWriteStruct.fsl_destination_address_u32 = writeToAddress;

        if (FSL_Write((__near fsl_write_t*) &fslWriteStruct) != FSL_OK)
        {
            EI();
            return FALSE;
        }
        
        writeToAddress += chunkSizeBytes;
        dataPointer += chunkSizeBytes;
    }
    
    EI();
    
    R_WDT_Restart();
    
    if (FSL_IVerify(blockIndex) != FSL_OK)
    {
        return FALSE;
    }
    
    __far const uint8_t* flashPointer = (__far const uint8_t*) address;
    __near const uint8_t* sourcePointer = (__near const uint8_t*) data;
    
    for (uint16_t i = 0; i < FLASH_BLOCK_SIZE_BYTES; i++)
    {
        if (*flashPointer != *sourcePointer)
        {
            return FALSE;
        }
    }
    
    return TRUE;
}

bool FlashEngineIsFirmwarePresent(uint8_t firmwareOption)
{
    uint32_t firmwareAddress = firmwareOption == FIRMWARE_A
        ? A_UPDATE_START_ADDRESS
        : B_UPDATE_START_ADDRESS;
        
    uint32_t firmwareSizeBytes = *((__far uint32_t*) firmwareAddress);
    uint32_t referenceCrc32 = *((__far uint32_t*) (firmwareAddress + 4));
    
    __far uint8_t* firmwarePointer = (__far uint8_t*) (firmwareAddress + 8);
    
    if (firmwareSizeBytes >= MAX_FIRMWARE_IMAGE_SIZE_BYTES)
    {
        return FALSE;
    }

    uint32_t firmwareCrc32 = Crc32(firmwarePointer, firmwareSizeBytes, 0xFFFFFFFF);
    
    return firmwareCrc32 == referenceCrc32;
}

uint32_t FlashEngineGetFirmwareCrc(uint8_t firmwareOption)
{
    uint32_t firmwareAddress = firmwareOption == FIRMWARE_A
        ? A_UPDATE_START_ADDRESS
        : B_UPDATE_START_ADDRESS;

    return *((__far uint32_t*) (firmwareAddress + 4));
}

uint32_t FlashEngineGetFirmwareSize(uint8_t firmwareOption)
{
    uint32_t firmwareAddress = firmwareOption == FIRMWARE_A
        ? A_UPDATE_START_ADDRESS
        : B_UPDATE_START_ADDRESS;
        
    return *((__far uint32_t*) firmwareAddress);
}

bool FlashEngineRestoreFirmwareFromUpdate(uint8_t firmwareOption)
{
    if (!FlashEngineIsFirmwarePresent(firmwareOption))
    {
        return FALSE;
    }
    
    uint32_t targetAddress = PROGRAM_START_ADDRESS;
    
    uint32_t firmwareAddress = firmwareOption == FIRMWARE_A
        ? A_UPDATE_START_ADDRESS
        : B_UPDATE_START_ADDRESS;
        
    uint32_t firmwareSizeBytes = *((__far uint32_t*) firmwareAddress);
    uint32_t referenceCrc32 = *((__far uint32_t*) (firmwareAddress + 4));
    __far uint8_t* firmwarePointer = (__far uint8_t*) (firmwareAddress + 8);
    
    if (firmwareSizeBytes > MAX_FIRMWARE_IMAGE_SIZE_BYTES)
    {
        return FALSE;
    }
    
    uint8_t blockBuffer[FLASH_BLOCK_SIZE_BYTES];
    
    uint32_t firmwareBytesRemain = firmwareSizeBytes;
    
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
        
        for (uint16_t i = 0; i < bytesToCopy; i++)
        {
            blockBuffer[i] = *firmwarePointer;
            firmwarePointer = (__far const uint8_t*)(((uint32_t) firmwarePointer) + 1);
        }
        
        firmwareBytesRemain -= bytesToCopy;
        
        if (!FlashEngineWriteBlockAndVerify(targetAddress, blockBuffer))
        {
            return FALSE;
        }
        
        targetAddress += FLASH_BLOCK_SIZE_BYTES;
    }
    
    return Crc32((__far const uint8_t*) PROGRAM_START_ADDRESS, firmwareSizeBytes, 0xFFFFFFFF) == referenceCrc32;
}
