################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/protocolStack/mac/AMCModule.cpp \
../src/protocolStack/mac/gnb-mac-entity.cpp \
../src/protocolStack/mac/harq-manager.cpp \
../src/protocolStack/mac/henb-mac-entity.cpp \
../src/protocolStack/mac/mac-entity.cpp \
../src/protocolStack/mac/nb-AMCModule.cpp \
../src/protocolStack/mac/ue-mac-entity.cpp

OBJS += \
./src/protocolStack/mac/AMCModule.o \
./src/protocolStack/mac/gnb-mac-entity.o \
./src/protocolStack/mac/harq-manager.o \
./src/protocolStack/mac/henb-mac-entity.o \
./src/protocolStack/mac/mac-entity.o \
./src/protocolStack/mac/nb-AMCModule.o \
./src/protocolStack/mac/ue-mac-entity.o 

CPP_DEPS += \
./src/protocolStack/mac/AMCModule.d \
./src/protocolStack/mac/gnb-mac-entity.d \
./src/protocolStack/mac/harq-manager.d \
./src/protocolStack/mac/henb-mac-entity.d  \
./src/protocolStack/mac/mac-entity.d \
./src/protocolStack/mac/nb-AMCModule.d \
./src/protocolStack/mac/ue-mac-entity.d 


# Each subdirectory must supply rules for building sources it contributes
src/protocolStack/mac/%.o: ../src/protocolStack/mac/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -w -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
