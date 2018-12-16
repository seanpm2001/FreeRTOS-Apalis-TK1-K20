set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_COLOR_MAKEFILE on)
set(CMAKE_ERROR_DEPRECATED on)

cmake_minimum_required(VERSION 3.6.0)
project(apalis-tk1-k20 C ASM)

set(TARGET ${CMAKE_PROJECT_NAME})

set(PROCESSOR_FAMILY ARM_CM3)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/startup
    ${CMAKE_CURRENT_SOURCE_DIR}/utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/freertos/include
    ${CMAKE_CURRENT_SOURCE_DIR}/freertos/src/portable/GCC/${PROCESSOR_FAMILY}/
    ${CMAKE_CURRENT_SOURCE_DIR}/app/include
    ${CMAKE_CURRENT_SOURCE_DIR}/board/include
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/include
    )

set(SOURCES
   startup/startup_MK20D10.S startup/system_MK20D10.c
   utilities/fsl_debug_console.c
   )

include(freertos/CMakeLists.txt)
include(drivers/CMakeLists.txt)
include(app/CMakeLists.txt)
include(board/CMakeLists.txt)

add_executable(${TARGET} ${SOURCES})

# Set a default build type if none was specified
set(default_build_type "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(WARNING "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release")
endif()

SET(CMAKE_C_FLAGS "-mthumb -fno-builtin -mcpu=cortex-m4 -mfloat-abi=soft -Wall -DCPU_MK20DN512VLK10 -DNDEBUG -std=gnu99 -ffunction-sections -fdata-sections -fomit-frame-pointer -mabi=aapcs -ffast-math -ftree-vectorize" CACHE INTERNAL "c compiler flags")
SET(CMAKE_ASM_FLAGS "-mthumb -mcpu=cortex-m4 -mfloat-abi=soft -Wa,--no-warn -Wall -DCPU_MK20DN512VLK10 -DNDEBUG -x assembler-with-cpp" CACHE INTERNAL "asm compiler flags")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections -mthumb --specs=nano.specs --specs=nosys.specs -Wl,-Map,\"${TARGET}.map\"" CACHE INTERNAL "executable linker flags")

IF(NOT CROSS_COMPILE_PREFIX)
    SET(CROSS_COMPILE_PREFIX "arm-none-eabi-")
    MESSAGE(WARNING "No CROSS_COMPILE_PREFIX specified, using default: " ${CROSS_COMPILE_PREFIX})
ENDIF()

SET(CMAKE_C_COMPILER "${CROSS_COMPILE_PREFIX}gcc")
set(CMAKE_ASM_COMPILER "${CROSS_COMPILE_PREFIX}gcc")
SET(CMAKE_OBJCOPY "${CROSS_COMPILE_PREFIX}objcopy" CACHE INTERNAL "objcopy tool")
SET(CMAKE_OBJDUMP "${CROSS_COMPILE_PREFIX}objdump" CACHE INTERNAL "objdump tool")
SET(CMAKE_SIZE "${CROSS_COMPILE_PREFIX}size" CACHE INTERNAL "size tool")

SET(CMAKE_C_FLAGS_DEBUG "-Og -g" CACHE INTERNAL "c compiler flags debug")
SET(CMAKE_ASM_FLAGS_DEBUG "-g" CACHE INTERNAL "asm compiler flags debug")
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "" CACHE INTERNAL "linker flags debug")

SET(CMAKE_C_FLAGS_RELEASE "-O2 -flto" CACHE INTERNAL "c compiler flags release")
SET(CMAKE_ASM_FLAGS_RELEASE "" CACHE INTERNAL "asm compiler flags release")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto" CACHE INTERNAL "linker flags release")

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/MK20DN512xxx10_flash.ld ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_flash.ld)

GET_TARGET_PROPERTY(TARGET_LD_FLAGS ${TARGET} LINK_FLAGS)
IF(TARGET_LD_FLAGS)
    SET(TARGET_LD_FLAGS "\"-T${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_flash.ld\" ${TARGET_LD_FLAGS}")
ELSE()
    SET(TARGET_LD_FLAGS "\"-T${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_flash.ld\"")
ENDIF()
SET_TARGET_PROPERTIES(${TARGET} PROPERTIES LINK_FLAGS ${TARGET_LD_FLAGS})

SET(FILENAME "${TARGET}")
ADD_CUSTOM_TARGET(${TARGET}.bin ALL DEPENDS ${TARGET} COMMAND ${CMAKE_OBJCOPY} -Obinary ${FILENAME} ${FILENAME}.bin)
add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_SIZE} ${FILENAME})
