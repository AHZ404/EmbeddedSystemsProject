/*****************************************************************************
 * File: eeprom.h
 * Module: EEPROM
 * Description: Header file for TM4C123GH6PM EEPROM Driver (TivaWare)
 * Author: Ahmedhh
 * Date: December 14, 2025
 * 
 * This driver uses TivaWare peripheral library functions:
 *   - SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0)
 *   - SysCtlPeripheralReady(SYSCTL_PERIPH_EEPROM0)
 *   - EEPROMInit()
 *   - EEPROMProgram(pui32Data, ui32Address, ui32Count)
 *   - EEPROMRead(pui32Data, ui32Address, ui32Count)
 *   - EEPROMMassErase()
 * 
 * Required TivaWare includes (in eeprom.c):
 *   - inc/hw_memmap.h
 *   - inc/hw_types.h
 *   - driverlib/sysctl.h
 *   - driverlib/eeprom.h
 * 
 * EEPROM Specifications:
 *   - Total size: 2KB (2048 bytes)
 *   - Organized as 32 blocks of 16 words (64 bytes) each
 *   - Word size: 32 bits (4 bytes)
 *   - Access: Word-aligned addresses only
 *****************************************************************************/

#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 *                              Definitions                                    *
 ******************************************************************************/

/* EEPROM Return Codes */
#define EEPROM_SUCCESS          0
#define EEPROM_ERROR            1

/* EEPROM Configuration */
#define EEPROM_BLOCK_SIZE       16      /* 16 words per block */
#define EEPROM_WORD_SIZE        4       /* 4 bytes per word */
#define EEPROM_TOTAL_BLOCKS     32      /* 32 blocks total */
#define EEPROM_TOTAL_SIZE       2048    /* 2KB total */

/******************************************************************************
 *                          Function Prototypes                                *
 ******************************************************************************/

/*
 * EEPROM_Init
 * Initializes the EEPROM module using TivaWare.
 * Returns: EEPROM_SUCCESS on success, EEPROM_ERROR on failure
 */
uint8_t EEPROM_Init(void);

/*
 * EEPROM_WriteWord
 * Writes a 32-bit word to the specified EEPROM address.
 * Parameters:
 *   block  - Block number (0-31)
 *   offset - Word offset within block (0-15)
 *   data   - 32-bit data to write
 * Returns: EEPROM_SUCCESS on success, EEPROM_ERROR on failure
 */
uint8_t EEPROM_WriteWord(uint32_t block, uint32_t offset, uint32_t data);

/*
 * EEPROM_ReadWord
 * Reads a 32-bit word from the specified EEPROM address.
 * Parameters:
 *   block  - Block number (0-31)
 *   offset - Word offset within block (0-15)
 *   data   - Pointer to store the read data
 * Returns: EEPROM_SUCCESS on success, EEPROM_ERROR on failure
 */
uint8_t EEPROM_ReadWord(uint32_t block, uint32_t offset, uint32_t *data);

/*
 * EEPROM_WriteBuffer
 * Writes a buffer of bytes to the EEPROM starting at specified address.
 * Parameters:
 *   block  - Starting block number (0-31)
 *   offset - Starting word offset within block (0-15)
 *   buffer - Pointer to data buffer
 *   length - Number of bytes to write (must be multiple of 4)
 * Returns: EEPROM_SUCCESS on success, EEPROM_ERROR on failure
 */
uint8_t EEPROM_WriteBuffer(uint32_t block, uint32_t offset, const uint8_t *buffer, uint32_t length);

/*
 * EEPROM_ReadBuffer
 * Reads a buffer of bytes from the EEPROM starting at specified address.
 * Parameters:
 *   block  - Starting block number (0-31)
 *   offset - Starting word offset within block (0-15)
 *   buffer - Pointer to buffer to store read data
 *   length - Number of bytes to read (must be multiple of 4)
 * Returns: EEPROM_SUCCESS on success, EEPROM_ERROR on failure
 */
uint8_t EEPROM_ReadBuffer(uint32_t block, uint32_t offset, uint8_t *buffer, uint32_t length);

/*
 * EEPROM_MassErase
 * Erases the entire EEPROM (sets all bits to 1).
 * Returns: EEPROM_SUCCESS on success, EEPROM_ERROR on failure
 */
uint8_t EEPROM_MassErase(void);

#endif /* EEPROM_H_ */
