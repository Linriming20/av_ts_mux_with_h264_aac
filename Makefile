SRC := aac_adts.c \
       aac_adts.h \
       h264_nalu.c \
       h264_nalu.h \
       ts.c \
       ts.h \
       crcLib.c\
       crcLib.h \
       main.c

TARGET := ts_mux_h264_aac

CC := gcc
CFLAGS := -g -Wall
LDFLAGS :=

ifeq ($(DEBUG), 1)
CFLAGS += -DENABLE_DEBUG
endif


$(TARGET) : $(SRC)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@

clean :
	rm -rf $(TARGET) out*
.PHONY := clean
