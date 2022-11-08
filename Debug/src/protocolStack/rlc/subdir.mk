################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/protocolStack/rlc/am-rlc-entity.cpp \
../src/protocolStack/rlc/am-state-variables.cpp \
../src/protocolStack/rlc/amd-record.cpp \
../src/protocolStack/rlc/rlc-entity.cpp \
../src/protocolStack/rlc/tm-rlc-entity.cpp \
../src/protocolStack/rlc/um-rlc-entity.cpp 

OBJS += \
./src/protocolStack/rlc/am-rlc-entity.o \
./src/protocolStack/rlc/am-state-variables.o \
./src/protocolStack/rlc/amd-record.o \
./src/protocolStack/rlc/rlc-entity.o \
./src/protocolStack/rlc/tm-rlc-entity.o \
./src/protocolStack/rlc/um-rlc-entity.o 

CPP_DEPS += \
./src/protocolStack/rlc/am-rlc-entity.d \
./src/protocolStack/rlc/am-state-variables.d \
./src/protocolStack/rlc/amd-record.d \
./src/protocolStack/rlc/rlc-entity.d \
./src/protocolStack/rlc/tm-rlc-entity.d \
./src/protocolStack/rlc/um-rlc-entity.d 


# Each subdirectory must supply rules for building sources it contributes
src/protocolStack/rlc/%.o: ../src/protocolStack/rlc/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -w -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
