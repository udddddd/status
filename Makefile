SRC=bar.c fmt.c $(wildcard modules/*.c)

CFLAGS=-fdata-sections     \
       -ffunction-sections \
       -Wl,--gc-sections   \
       -Wl,--as-needed     \
       -flto               \
       -Os                 \
       -Wall               \
       -pedantic           \
       -DNDEBUG

LFLAGS=-lX11 -ludev -lasound -lpthread -lm

OUT=status

all: config.h
	gcc $(SRC) $(CFLAGS) $(LFLAGS) -o $(OUT)
config.h:
	cp config.def.h config.h
