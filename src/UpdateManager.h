#pragma once

#define CONTROL_BLOCK_ADDRESS              0x7A000
#define MAX_FLASH_RETRIES                  3

// TODO: Move control block to the Data Flash
typedef union __ControlBlockUnion
{
    struct __ControlBlock
    {
        uint32_t controlBlockCrc;

        uint8_t activeFirmwareSelector;
        uint16_t activeFirmwareVersion;
        uint32_t activeFirmwareSize;
        uint32_t activeFirmwareCrc;
        
        uint16_t versionOfFirmwareA;
        uint16_t versionOfFirmwareB;
        
        uint8_t lastAction;
        uint8_t lastActionCount;
    } ControlBlock;
    
    uint8_t flatBytes[FLASH_BLOCK_SIZE_BYTES];
} ControlBlockUnion;

bool ReadControlBlock(ControlBlockUnion* controlBlock)
{
    __far uint8_t* sourcePointer = (__far uint8_t*) CONTROL_BLOCK_ADDRESS;
    uint8_t* targetPointer = (uint8_t*) &controlBlock->flatBytes[0];
    
    for (uint16_t i = 0; i < FLASH_BLOCK_SIZE_BYTES; i++)
    {
        *targetPointer = *sourcePointer;
        
        sourcePointer += 1;
        targetPointer += 1;
    }
    
    uint32_t crc = Crc32Near((uint8_t*) &controlBlock->flatBytes[4], FLASH_BLOCK_SIZE_BYTES - 4, 0xFFFFFFFF);
    return crc == controlBlock->ControlBlock.controlBlockCrc;
}

bool WriteControlBlock(ControlBlockUnion* controlBlock)
{
    controlBlock->ControlBlock.controlBlockCrc = Crc32Near((uint8_t*) &controlBlock->flatBytes[4], FLASH_BLOCK_SIZE_BYTES - 4, 0xFFFFFFFF);
    return FlashEngineWriteBlockAndVerify(CONTROL_BLOCK_ADDRESS, controlBlock->flatBytes);
}

bool InitializeControlBlock(ControlBlockUnion* controlBlock)
{    
    controlBlock->ControlBlock.activeFirmwareSelector = FIRMWARE_0;
    controlBlock->ControlBlock.activeFirmwareVersion = 0;
    controlBlock->ControlBlock.activeFirmwareSize = MAX_FIRMWARE_IMAGE_SIZE_BYTES - 4;
    controlBlock->ControlBlock.activeFirmwareCrc = Crc32((__far uint8_t*) PROGRAM_START_ADDRESS, MAX_FIRMWARE_IMAGE_SIZE_BYTES - 4, 0xFFFFFFFF);
    
    controlBlock->ControlBlock.versionOfFirmwareA = 0;
    controlBlock->ControlBlock.versionOfFirmwareB = 0;
    
    controlBlock->ControlBlock.lastAction = FIRMWARE_0;
    controlBlock->ControlBlock.lastActionCount = 0;
    
    return WriteControlBlock(controlBlock);
}

bool ProbeControlBlock(ControlBlockUnion* controlBlock)
{
    if (!ReadControlBlock(controlBlock))
    {
        return InitializeControlBlock(controlBlock);
    }
    
    return TRUE;
}

uint8_t GetFirmwareOptionToUpdate(ControlBlockUnion* controlBlock)
{
    uint8_t optionToUpdate = FIRMWARE_A;
    if (controlBlock->ControlBlock.versionOfFirmwareB <
        controlBlock->ControlBlock.versionOfFirmwareA)
    {
        optionToUpdate = FIRMWARE_B;
    }
    
    return optionToUpdate;
}

bool UpdateManagerRun(ControlBlockUnion* controlBlock)
{
    bool isFirmwareA = FlashEngineIsFirmwarePresent(FIRMWARE_A);
    bool isFirmwareB = FlashEngineIsFirmwarePresent(FIRMWARE_B);
    
    if (!isFirmwareA &&
        !isFirmwareB)
    {
        // No firmware update images available, nothing to do
        return TRUE;
    }
    
    if (!isFirmwareA)
    {
        // Reset version for absent/broken firmware A image in control block
        controlBlock->ControlBlock.versionOfFirmwareA = 0;
    }

    if (!isFirmwareB)
    {
        // Reset version for absent/broken firmware B image in control block
        controlBlock->ControlBlock.versionOfFirmwareB = 0;
    }
    
    uint8_t targetOptionToUpdate = FIRMWARE_A;
    uint16_t targetVersionToUpdate = controlBlock->ControlBlock.versionOfFirmwareA;
    
    if (controlBlock->ControlBlock.versionOfFirmwareA <
        controlBlock->ControlBlock.versionOfFirmwareB)
    {
        targetOptionToUpdate = FIRMWARE_B;
        targetVersionToUpdate = controlBlock->ControlBlock.versionOfFirmwareB;
    }
    
    uint8_t actionToUpdate = FIRMWARE_0;
    
    while (1)
    {
        // 1. If active version has wrong CRC, program image with greater version
        if (controlBlock->ControlBlock.activeFirmwareCrc != 
            Crc32((__far uint8_t*) PROGRAM_START_ADDRESS, controlBlock->ControlBlock.activeFirmwareSize, 0xFFFFFFFF))
        {
            actionToUpdate = targetOptionToUpdate;
            break;
        }
        
        // 2. Is active version is obsolete, flash image with greater version
        if (controlBlock->ControlBlock.activeFirmwareVersion < targetVersionToUpdate)
        {
            actionToUpdate = targetOptionToUpdate;
            break;
        }
        
        break;
    }
    
    if (actionToUpdate != FIRMWARE_0)
    {
        if (controlBlock->ControlBlock.lastAction != FIRMWARE_0 &&
            controlBlock->ControlBlock.lastActionCount > MAX_FLASH_RETRIES)
        {
            return FALSE;
        }

        controlBlock->ControlBlock.activeFirmwareSelector = actionToUpdate;
        controlBlock->ControlBlock.activeFirmwareVersion = targetVersionToUpdate;
        controlBlock->ControlBlock.activeFirmwareSize = FlashEngineGetFirmwareSize(actionToUpdate);
        controlBlock->ControlBlock.activeFirmwareCrc = FlashEngineGetFirmwareCrc(actionToUpdate);
        
        controlBlock->ControlBlock.lastAction = actionToUpdate;
        controlBlock->ControlBlock.lastActionCount++;
        
        if (!FlashEngineRestoreFirmwareFromUpdate(actionToUpdate))
        {
            return FALSE;
        }
        
        if (!WriteControlBlock(controlBlock))
        {
            return FALSE;
        }
    } else
    {
        // No working program update required, normal startup
        if (controlBlock->ControlBlock.lastAction != FIRMWARE_0)
        {
            controlBlock->ControlBlock.lastAction = FIRMWARE_0;
            controlBlock->ControlBlock.lastActionCount = 0;
            
            if (!WriteControlBlock(controlBlock))
            {
                return FALSE;
            }
        }
    }
    
    return TRUE;
}

void UpdateManagerSwapFirmware(ControlBlockUnion* controlBlock)
{
    // Must be available two valid versions of the firmware to make swap
    
    bool isFirmwareA = FlashEngineIsFirmwarePresent(FIRMWARE_A);
    bool isFirmwareB = FlashEngineIsFirmwarePresent(FIRMWARE_B);
    
    if (!(isFirmwareA && isFirmwareB))
    {
        // No firmware update images available, nothing to do
        return;
    }
    
    if (controlBlock->ControlBlock.versionOfFirmwareA <
        controlBlock->ControlBlock.versionOfFirmwareB)
    {
        controlBlock->ControlBlock.versionOfFirmwareA =
            controlBlock->ControlBlock.versionOfFirmwareB + 1;
    } else
    {
        controlBlock->ControlBlock.versionOfFirmwareB =
            controlBlock->ControlBlock.versionOfFirmwareA + 1;
    }
}
