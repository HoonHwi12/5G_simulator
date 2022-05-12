################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/phy/error-model.cpp \
../src/phy/interference.cpp \
../src/phy/simple-error-model.cpp \
../src/phy/error-model.cpp \
../src/phy/henb-phy.cpp \
../src/phy/multicast-destination-phy.cpp \
../src/phy/phy.cpp \
../src/phy/precoding-calculator.cpp \
../src/phy/sinr-calculator.cpp \
../src/phy/ue-phy.cpp \
../src/phy/wideband-cqi-eesm-error-model.cpp 

OBJS += \
./src/phy/error-model.o \
./src/phy/interference.o \
./src/phy/simple-error-model.o \
./src/phy/error-model.o \
./src/phy/henb-phy.o \
./src/phy/multicast-destination-phy.o \
./src/phy/phy.o \
./src/phy/precoding-calculator.o \
./src/phy/sinr-calculator.o \
./src/phy/ue-phy.o \
./src/phy/wideband-cqi-eesm-error-model.o 

CPP_DEPS += \
./src/phy/error-model.d \
./src/phy/interference.d \
./src/phy/simple-error-model.d \
./src/phy/error-model.d \
./src/phy/henb-phy.d \
./src/phy/multicast-destination-phy.d \
./src/phy/phy.d \
./src/phy/precoding-calculator.d \
./src/phy/sinr-calculator.d \
./src/phy/ue-phy.d \
./src/phy/wideband-cqi-eesm-error-model.d 


# Each subdirectory must supply rules for building sources it contributes
src/phy/%.o: ../src/phy/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -w -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


