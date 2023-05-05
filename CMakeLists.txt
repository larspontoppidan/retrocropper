
cmake_minimum_required(VERSION 3.5)

# Project name
project("Retro Scaler")
set(PRODUCT_NAME retro_scaler)

include_directories("src")

## AVR Chip Configuration
set(F_CPU 20000000UL)
set(MCU attiny84)
set(MCU_AVRDUDE t84)

# Configure programmer and port
set(PROG_TYPE avrispmkII)


# AVR Fuses and lockbits
set(L_FUSE 0xFE)
set(H_FUSE 0xD7)
set(E_FUSE 0xFF)


# Use AVR GCC toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_ASM_COMPILER avr-gcc)

# Pass options to compiler
add_definitions(
    -DF_CPU=${F_CPU}
)

add_compile_options(
    -mmcu=${MCU} # MCU
    -std=gnu99 # C99 standard
    -O2 # optimize for speed
    -Wall # enable warnings
)

# Set linker commands including mcu type
set(CMAKE_EXE_LINKER_FLAGS "-mmcu=${MCU} -Wl,--relax,--gc-sections" )

file(GLOB_RECURSE SRC_FILES "src/*.c") # Load all files in src folder

# Create target output
add_executable(${PRODUCT_NAME} ${SRC_FILES})

# Rename the output to .elf
set_target_properties(${PRODUCT_NAME} PROPERTIES OUTPUT_NAME ${PRODUCT_NAME}.elf)

# Make listing
add_custom_target(listing ALL avr-objdump -h -S -z ${PRODUCT_NAME}.elf > ${PRODUCT_NAME}.lss DEPENDS ${PRODUCT_NAME})

# Transform binary into hex file
add_custom_target(hex ALL avr-objcopy -R .eeprom -O ihex ${PRODUCT_NAME}.elf ${PRODUCT_NAME}.hex DEPENDS ${PRODUCT_NAME})

# Upload the firmware with avrdude
add_custom_target(upload avrdude -c ${PROG_TYPE} -p ${MCU_AVRDUDE} -U flash:w:${PRODUCT_NAME}.hex DEPENDS hex)

# Burn fuses, using a low speed (-B 10), to be able to program a new chip running slowly from the internal oscillator
add_custom_target(fuses avrdude -c ${PROG_TYPE} -p ${MCU_AVRDUDE} -B 10 -U lfuse:w:${L_FUSE}:m -U hfuse:w:${H_FUSE}:m -U efuse:w:${E_FUSE}:m )

# Clean extra files
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${PRODUCT_NAME}.hex;${PRODUCT_NAME}.eeprom;${PRODUCT_NAME}.lst")


