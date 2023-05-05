
// Project  : RetroCropper
//
// Filename : nvm.h
// Created  : April, 2023
// Author   : Lars Ole Pontoppidan
// License  : MIT License, see LICENSE file in project root

#ifndef __SRC_NVM_H__
#define __SRC_NVM_H__

// ------------------------------   INCLUDES   ---------------------------------

#include <stdint.h>

// -------------------------   TYPES AND CONSTANTS   ---------------------------


// ------------------------------   FUNCTIONS   --------------------------------


uint8_t nvmReadValue(uint8_t fallback);

void nvmWriteValue(uint8_t value);


#endif // __SRC_NVM_H__
