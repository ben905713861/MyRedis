################################################################################
# 自动生成的文件。不要编辑！
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Auth.cpp \
../src/Connect.cpp \
../src/MessageHandler.cpp \
../src/MyRedis.cpp \
../src/Worker.cpp 

OBJS += \
./src/Auth.o \
./src/Connect.o \
./src/MessageHandler.o \
./src/MyRedis.o \
./src/Worker.o 

CPP_DEPS += \
./src/Auth.d \
./src/Connect.d \
./src/MessageHandler.d \
./src/MyRedis.d \
./src/Worker.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo '正在构建文件： $<'
	@echo '正在调用： GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo '已结束构建： $<'
	@echo ' '


