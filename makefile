SRC=bar.c fmt.c $(wildcard modules/*.c)

.PHONY: all debug

CFLAGS=-fdata-sections     \
       -ffunction-sections \
       -Wl,--gc-sections   \
       -Wl,--as-needed     \
       -flto               \
       -Os                 \
       -Wall               \
       -pedantic

LFLAGS=-lX11 -ludev -lasound -lpthread -lm

OUT=status

all: config.h
	gcc $(SRC) $(CFLAGS) -DNDEBUG $(LFLAGS) -o $(OUT)

debug: config.h
	gcc $(SRC) $(CFLAGS) $(LFLAGS) -o $(OUT)

config.h:
	cp config.def.h config.h
