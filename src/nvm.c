
// Project  : RetroCropper
//
// Filename : nvm.c
// Created  : April, 2023
// Author   : Lars Ole Pontoppidan
// License  : MIT License, see LICENSE file in project root

// ------------------------------   INCLUDES   ---------------------------------

#include "nvm.h"

#include <avr/io.h>

// -------------------------   TYPES AND CONSTANTS   ---------------------------

// EEPROM address to use
#define EEPROM_ADDR 0x00 

// First byte must have this value in order to accept contents of EEPROM
#define EEPROM_COOKIE 0xAB

// ---------------------------   LOCAL VARIABLES   -----------------------------


// -----------------------------   PROTOTYPES   --------------------------------


// --------------------------   PRIVATE FUNCTIONS   ----------------------------


static void EEPROM_write(uint16_t addr, uint8_t data)
{
    // Wait for completion of previous write operation
    while (EECR & (1 << EEPE));

    // Set up address and data registers
    EEAR = addr;
    EEDR = data;

    // Enable EEPROM write
    EECR |= (1 << EEMPE);

    // Start EEPROM write
    EECR |= (1 << EEPE);
}

static uint8_t EEPROM_read(uint16_t addr)
{
    // Wait for completion of previous write operation
    while (EECR & (1 << EEPE));

    // Set up address register
    EEAR = addr;

    // Start EEPROM read
    EECR |= (1 << EERE);

    // Return data from data register
    return EEDR;
}


// ---------------------------   PUBLIC FUNCTIONS   ----------------------------


uint8_t nvmReadValue(uint8_t fallback)
{
    if (EEPROM_read(EEPROM_ADDR) == EEPROM_COOKIE)
    {
        return EEPROM_read(EEPROM_ADDR + 1);
    }
    return fallback;
}

void nvmWriteValue(uint8_t value)
{
    if (EEPROM_read(EEPROM_ADDR) != EEPROM_COOKIE)
    {
        EEPROM_write(EEPROM_ADDR, EEPROM_COOKIE);
    }
    EEPROM_write(EEPROM_ADDR + 1, value);
}


