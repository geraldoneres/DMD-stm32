################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/display/max7219/src/GFX_BW.c \
../Core/display/max7219/src/max7219_matrix.c 

OBJS += \
./Core/display/max7219/src/GFX_BW.o \
./Core/display/max7219/src/max7219_matrix.o 

C_DEPS += \
./Core/display/max7219/src/GFX_BW.d \
./Core/display/max7219/src/max7219_matrix.d 


# Each subdirectory must supply rules for building sources it contributes
Core/display/max7219/src/GFX_BW.o: ../Core/display/max7219/src/GFX_BW.c Core/display/max7219/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G071xx -c -I../Core/Inc -I../Drivers/STM32G0xx_HAL_Driver/Inc -I../Drivers/STM32G0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/display/max7219/src/GFX_BW.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/display/max7219/src/max7219_matrix.o: ../Core/display/max7219/src/max7219_matrix.c Core/display/max7219/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G071xx -c -I../Core/Inc -I../Drivers/STM32G0xx_HAL_Driver/Inc -I../Drivers/STM32G0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/display/max7219/src/max7219_matrix.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

