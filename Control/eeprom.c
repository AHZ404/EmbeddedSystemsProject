/*****************************************************************************
 * File: eeprom.c
 * Module: EEPROM
 * Description: Source file for TM4C123GH6PM EEPROM Driver (TivaWare)
 * Author: Ahmedhh
 * Date: December 14, 2025
 * 
 * This implementation uses actual TivaWare peripheral library functions:
 *   - SysCtlPeripheralEnable()
 *   - EEPROMInit()
 *   - EEPROMProgram()
 *   - EEPROMRead()
 *   - EEPROMMassErase()
 * 
 * Include paths for TivaWare:
 *   - driverlib/sysctl.h
 *   - driverlib/eeprom.h
 *****************************************************************************/

#include "eeprom.h"
#include <stdint.h>
#include <stdbool.h>

/* TivaWare includes */
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/eeprom.h"

/******************************************************************************
 *                          Private Functions                                  *
 ******************************************************************************/

/*
 * CalculateAddress
 * Calculates byte address from block and offset.
 */
static uint32_t CalculateAddress(uint32_t block, uint32_t offset)
{
    return (block * EEPROM_BLOCK_SIZE * EEPROM_WORD_SIZE) + 
           (offset * EEPROM_WORD_SIZE);
}

/******************************************************************************
 *                          Public Functions                                   *
 ******************************************************************************/

/*
 * EEPROM_Init
 * Initializes the EEPROM module using TivaWare library.
 */
uint8_t EEPROM_Init(void)
{
    uint32_t result;
    
    /* Enable EEPROM peripheral using TivaWare function */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);
    
    /* Wait for EEPROM peripheral to be ready */
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_EEPROM0))
    {
    }
    
    /* Initialize EEPROM using TivaWare function */
    result = EEPROMInit();
    
    /* Check initialization result */
    if(result != EEPROM_INIT_OK)
    {
        return EEPROM_ERROR;
    }
    
    return EEPROM_SUCCESS;
}

/*
 * EEPROM_WriteWord
 * Writes a word to EEPROM using TivaWare EEPROMProgram().
 */
uint8_t EEPROM_WriteWord(uint32_t block, uint32_t offset, uint32_t data)
{
    uint32_t address;
    uint32_t result;
    
    /* Validate parameters */
    if(block >= EEPROM_TOTAL_BLOCKS || offset >= EEPROM_BLOCK_SIZE)
    {
        return EEPROM_ERROR;
    }
    
    /* Calculate byte address */
    address = CalculateAddress(block, offset);
    
    /* Write data using TivaWare function */
    result = EEPROMProgram(&data, address, sizeof(uint32_t));
    
    if(result != 0)
    {
        return EEPROM_ERROR;
    }
    
    return EEPROM_SUCCESS;
}

/*
 * EEPROM_ReadWord
 * Reads a word from EEPROM using TivaWare EEPROMRead().
 */
uint8_t EEPROM_ReadWord(uint32_t block, uint32_t offset, uint32_t *data)
{
    uint32_t address;
    
    /* Validate parameters */
    if(block >= EEPROM_TOTAL_BLOCKS || offset >= EEPROM_BLOCK_SIZE || data == 0)
    {
        return EEPROM_ERROR;
    }
    
    /* Calculate byte address */
    address = CalculateAddress(block, offset);
    
    /* Read data using TivaWare function */
    EEPROMRead(data, address, sizeof(uint32_t));
    
    return EEPROM_SUCCESS;
}

/*
 * EEPROM_WriteBuffer
 * Writes a buffer to EEPROM using TivaWare EEPROMProgram().
 */
uint8_t EEPROM_WriteBuffer(uint32_t block, uint32_t offset, const uint8_t *buffer, uint32_t length)
{
    uint32_t address;
    uint32_t result;
    
    /* Validate parameters */
    if(buffer == 0 || (length % 4) != 0)
    {
        return EEPROM_ERROR;
    }
    
    if(block >= EEPROM_TOTAL_BLOCKS || offset >= EEPROM_BLOCK_SIZE)
    {
        return EEPROM_ERROR;
    }
    
    /* Calculate starting byte address */
    address = CalculateAddress(block, offset);
    
    /* Write buffer using TivaWare function */
    /* Note: EEPROMProgram accepts uint32_t* so we cast, but data must be word-aligned */
    result = EEPROMProgram((uint32_t*)buffer, address, length);
    
    if(result != 0)
    {
        return EEPROM_ERROR;
    }
    
    return EEPROM_SUCCESS;
}

/*
 * EEPROM_ReadBuffer
 * Reads a buffer from EEPROM using TivaWare EEPROMRead().
 */
uint8_t EEPROM_ReadBuffer(uint32_t block, uint32_t offset, uint8_t *buffer, uint32_t length)
{
    uint32_t address;
    
    /* Validate parameters */
    if(buffer == 0 || (length % 4) != 0)
    {
        return EEPROM_ERROR;
    }
    
    if(block >= EEPROM_TOTAL_BLOCKS || offset >= EEPROM_BLOCK_SIZE)
    {
        return EEPROM_ERROR;
    }
    
    /* Calculate starting byte address */
    address = CalculateAddress(block, offset);
    
    /* Read buffer using TivaWare function */
    EEPROMRead((uint32_t*)buffer, address, length);
    
    return EEPROM_SUCCESS;
}

/*
 * EEPROM_MassErase
 * Erases entire EEPROM using TivaWare EEPROMMassErase().
 */
uint8_t EEPROM_MassErase(void)
{
    uint32_t result;
    
    /* Erase using TivaWare function */
    result = EEPROMMassErase();
    
    if(result != 0)
    {
        return EEPROM_ERROR;
    }
    
    return EEPROM_SUCCESS;
}
