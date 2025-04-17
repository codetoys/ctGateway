#基础对象
OBJS_BASE= ChannelParam.o DeviceParam.o ProtocolParam.o gwmain_test.o\
	ProtocolStdWork.o ProtocolSvrModbus.o ProtocolSvrTunnel.o

TARGET_NAME=gwbuiltin
TARGET_OBJS=$(OBJS_BASE)

CONFIG_DIR=../../third/ctfc/platform/
include $(CONFIG_DIR)/config.mk

all=lib

all:$(all)

USER_COMPILE_FLAG= -I../../third/ctfc/src/httpd/ -I../../third/ctfc/src/function/ -I../../third/ctfc/src/env/ -I..
