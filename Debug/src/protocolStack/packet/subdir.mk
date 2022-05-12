################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/protocolStack/packet/Header.cpp \
../src/protocolStack/packet/packet-burst.cpp \
../src/protocolStack/packet/Packet.cpp \
../src/protocolStack/packet/PacketTAGs.cpp 

OBJS += \
./src/protocolStack/packet/Header.o \
./src/protocolStack/packet/Packet.o \
./src/protocolStack/packet/PacketTAGs.o \
./src/protocolStack/packet/packet-burst.o 

CPP_DEPS += \
./src/protocolStack/packet/Header.d \
./src/protocolStack/packet/packet-burst.d \
./src/protocolStack/packet/Packet.d \
./src/protocolStack/packet/PacketTAGs.d 


# Each subdirectory must supply rules for building sources it contributes
src/protocolStack/packet/%.o: ../src/protocolStack/packet/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -w -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '