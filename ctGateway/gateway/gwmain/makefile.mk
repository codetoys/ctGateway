#基础对象
OBJS_BASE= _cJSON.o gwmain.o MqttSvr.o gwconfig.o gwUtil.o LedControl.o LocalConfig.o gwdefine.o \
	IProtocol.o ProtocolSvr.o ProtocolMgr.o PlatformCT.o

TARGET_NAME=gwmain
TARGET_OBJS=$(OBJS_BASE)

CONFIG_DIR=../../third/ctfc/platform/
include $(CONFIG_DIR)/config.mk

all=lib testso2 testso gwmain.exe

all:$(all)

gwmain.exe:
	$(MAKE) exe TARGET_NAME=$@ TARGET_OBJS="gwmain_t.o"

testso:
	$(MAKE) so TARGET_NAME=$@ TARGET_OBJS="testso.o"
testso2:
	$(MAKE) so TARGET_NAME=$@ TARGET_OBJS="testso2.o "

# -L/home/user/libiconv_install/lib/ -l:libiconv.a -l:libcharset.a 这么做没用，不是缺少入口，而是入口已经被定义
USER_COMPILE_FLAG= -I../../third/ctfc/src/httpd/ -I../../third/ctfc/src/function/ -I../../third/ctfc/src/env/ -I../
COMMONLIB=-Wl,-E -L../../third/ctfc/lib/ -lgwmain -lgwbuiltin -lmodbus -lmyhttpd -lenv -lmosquittopp -lmosquitto -lgmssl -lssl -lcrypto -lz

# -ldlt645 -lmyOpen62541
