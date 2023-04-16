#pragma once

#include "CryptoPad.h"

#define PC_REPLY_TIMEOUT_MS             1000
#define DATA_BLOCK_RECEIVE_TIMEOUT_MS   2000
#define UPDATE_BLOCK_SIZE_BYTES         (4 + FLASH_BLOCK_SIZE_BYTES) /* CRC32 + block data */

bool ReceiveUpdateBlock(ModemContext* modemContext, uint8_t blockIndex, uint8_t* updateBuffer)
{
    char atCommand[32], blockIndexString[8];
    itoa(blockIndex, blockIndexString, 10);
    AppendStrings2(atCommand, "UPBLK,", blockIndexString);
    
    ModemPushCommandAsync(modemContext, atCommand);
    ModemSwitchToBinaryMode(modemContext, updateBuffer, UPDATE_BLOCK_SIZE_BYTES);
    
    if (!ModemWaitForBinaryData(modemContext, DATA_BLOCK_RECEIVE_TIMEOUT_MS))
    {
        ModemPushCommandAsync(modemContext, "TIMEOUT");
        return FALSE;
    }
    
    uint32_t referenceCrc32 = *((uint32_t*) updateBuffer);

    CryptoTransformBlock((uint16_t) referenceCrc32, updateBuffer + 4, FLASH_BLOCK_SIZE_BYTES);

    uint32_t crc32 = Crc32Near(updateBuffer + 4, FLASH_BLOCK_SIZE_BYTES, 0xFFFFFFFF);    
    
    if (crc32 != referenceCrc32)
    {
        ModemPushCommandAsync(modemContext, "BADBCRC");
        return FALSE;
    }
    
    return TRUE;
}

bool ReceiveUpdates(ModemContext* modemContext, uint16_t updateBlocks,
                    uint32_t targetAddress, uint32_t firmwareCrc32)
{
    uint8_t updateBlock[UPDATE_BLOCK_SIZE_BYTES];
    
    uint32_t targetAddressPointer = targetAddress;
    
    for (uint16_t i = 0; i < updateBlocks; i++)
    {
        if (!ReceiveUpdateBlock(modemContext, i, updateBlock))
        {
            return FALSE;
        }
        
        if (!FlashEngineWriteBlockAndVerify(targetAddressPointer, &updateBlock[4]))
        {
            ModemPushCommandAsync(modemContext, "FLASHERR");
            return FALSE;
        }
        
        targetAddressPointer += FLASH_BLOCK_SIZE_BYTES;
    }
    
    uint32_t uploadedFirmwareCrc32 = Crc32((__far const uint8_t*) targetAddress, updateBlocks * FLASH_BLOCK_SIZE_BYTES, 0xFFFFFFFF);
    if (uploadedFirmwareCrc32 != firmwareCrc32)
    {
        ModemPushCommandAsync(modemContext, "BADFCRC");
        return FALSE;
    }
    
    ModemPushCommandAsync(modemContext, "UPDONE");
    return TRUE;
}

bool ProbeUartFlashing(ControlBlockUnion* controlBlock, uint8_t firmwareToUpdate)
{
    ModemContext modemContext;
    ModemAttachToUart(&modemContext, 2);
    
    if (!ModemPushCommandSync(&modemContext, "RDYUPD", PC_REPLY_TIMEOUT_MS))
    {
        // Host is not ready for flashing
        return FALSE;
    }
    
    const char* replyString = ModemGetLastReply(&modemContext);
    // Reply format: +UPDATA,XXXX,Y..
    // Where XXXX - leading zero 4 digits with total block count
    //       Y..  - CRC32 of the update as integer
    if (StartsWith(replyString, "+UPDATA,"))
    {
        uint32_t firmwareAddress = firmwareToUpdate == FIRMWARE_A
            ? A_UPDATE_START_ADDRESS
            : B_UPDATE_START_ADDRESS;
        
        uint16_t updateBlocks = StringToInt16(replyString + 8);
        uint32_t firmwareCrc32 = StringToInt32(replyString + 13);
        if (!ReceiveUpdates(&modemContext, updateBlocks, firmwareAddress, firmwareCrc32))
        {
            return FALSE;
        }
        
        if (firmwareToUpdate == FIRMWARE_A)
        {
            controlBlock->ControlBlock.versionOfFirmwareA++;
        } else if (firmwareToUpdate == FIRMWARE_B)
        {
            controlBlock->ControlBlock.versionOfFirmwareB++;
        }
    
        WriteControlBlock(controlBlock);
    }
    
    ModemDetachFromUart(&modemContext);
    
    return TRUE;
}
