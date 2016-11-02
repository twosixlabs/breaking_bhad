.PHONY: clean all

CC=mipsel-linux-uclibc-gcc
CFLAGS=-fno-builtin -nostdinc -march=4kc -mtune=4kc -mabicalls -c
LD=mipsel-linux-uclibc-ld
OBJCOPY=mipsel-linux-uclibc-objcopy
OUT=spi

all: $(OUT)

$(OUT): $(OUT).elf
	@$(OBJCOPY) -O binary $< $@

$(OUT).elf: header.o $(OUT).o
	@$(LD) -Bstatic -T linker.lds -Map=$(OUT).map -o $@ $^

header.o: header.S Makefile
	@$(CC) $(CFLAGS) -o $@ $<

spi.o: spi.c Makefile
	@$(CC) $(CFLAGS) -o $@ $<

clean:
	@rm -f *.o
	@rm -f $(OUT) $(OUT).elf $(OUT).map

