#基础对象
OBJS_BASE= _cJSON.o ProtocolDemo.o

TARGET_NAME=protocol_demo
TARGET_OBJS=$(OBJS_BASE)

CONFIG_DIR=../../third/ctfc/platform/
include $(CONFIG_DIR)/config.mk

all=so

all:$(all)

protocol_demo.exe:
	$(MAKE) exe TARGET_NAME=$@ TARGET_OBJS="$(OBJS_BASE) protocol_demo_t.o"

# -L/home/user/libiconv_install/lib/ -l:libiconv.a -l:libcharset.a 这么做没用，不是缺少入口，而是入口已经被定义
USER_COMPILE_FLAG= -I../../third/ctfc/src/httpd/ -I../../third/ctfc/src/function/ -I../../third/ctfc/src/env/ -I../
COMMONLIB=-L../../third/ctfc/lib/
