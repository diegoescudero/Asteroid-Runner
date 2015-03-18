################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../GLSL_helper.cpp \
../GeometryCreator.cpp \
../MStackHelp.cpp \
../Main.cpp 

OBJS += \
./GLSL_helper.o \
./GeometryCreator.o \
./MStackHelp.o \
./Main.o 

CPP_DEPS += \
./GLSL_helper.d \
./GeometryCreator.d \
./MStackHelp.d \
./Main.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


