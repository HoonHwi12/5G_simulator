################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/channel/propagation-model/channel-realization.cpp \
../src/channel/propagation-model/propagation-loss-model.cpp \

OBJS += \
./src/channel/propagation-model/channel-realization.o \
./src/channel/propagation-model/propagation-loss-model.o \

CPP_DEPS += \
./src/channel/propagation-model/channel-realization.d \
./src/channel/propagation-model/propagation-loss-model.d \


# Each subdirectory must supply rules for building sources it contributes
src/channel/propagation-model/%.o: ../src/channel/propagation-model/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -w -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


