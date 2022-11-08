################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/protocolStack/mac/random-access/allocation-map.cpp \
../src/protocolStack/mac/random-access/enb-nb-iot-random-access.cpp \
../src/protocolStack/mac/random-access/gnb-enhanced-random-access.cpp \
../src/protocolStack/mac/random-access/gnb-random-access.cpp \
../src/protocolStack/mac/random-access/ue-baseline-random-access.cpp \
../src/protocolStack/mac/random-access/ue-enhanced-random-access.cpp \
../src/protocolStack/mac/random-access/ue-nb-iot-random-access.cpp \
../src/protocolStack/mac/random-access/ue-random-access.cpp


OBJS += \
./src/protocolStack/mac/random-access/allocation-map.o \
./src/protocolStack/mac/random-access/enb-nb-iot-random-access.o \
./src/protocolStack/mac/random-access/gnb-enhanced-random-access.o \
./src/protocolStack/mac/random-access/gnb-random-access.o \
./src/protocolStack/mac/random-access/ue-baseline-random-access.o \
./src/protocolStack/mac/random-access/ue-enhanced-random-access.o \
./src/protocolStack/mac/random-access/ue-nb-iot-random-access.o \
./src/protocolStack/mac/random-access/ue-random-access.o

CPP_DEPS += \
./src/protocolStack/mac/random-access/allocation-map.d \
./src/protocolStack/mac/random-access/enb-nb-iot-random-access.d \
./src/protocolStack/mac/random-access/gnb-enhanced-random-access.d \
./src/protocolStack/mac/random-access/gnb-random-access.d \
./src/protocolStack/mac/random-access/ue-baseline-random-access.d \
./src/protocolStack/mac/random-access/ue-enhanced-random-access.d \
./src/protocolStack/mac/random-access/ue-nb-iot-random-access.d \
./src/protocolStack/mac/random-access/ue-random-access.d


# Each subdirectory must supply rules for building sources it contributes
src/protocolStack/mac/packet-scheduler/%.o: ../src/protocolStack/mac/packet-scheduler/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -w -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


