################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/core/eventScheduler/calendar.cpp \
../src/core/eventScheduler/event.cpp \
../src/core/eventScheduler/simulator.cpp 

OBJS += \
./src/core/eventScheduler/calendar.o \
./src/core/eventScheduler/event.o \
./src/core/eventScheduler/simulator.o 

CPP_DEPS += \
./src/core/eventScheduler/calendar.d \
./src/core/eventScheduler/event.d \
./src/core/eventScheduler/simulator.d 


# Each subdirectory must supply rules for building sources it contributes
src/core/eventScheduler/%.o: ../src/core/eventScheduler/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -w -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '