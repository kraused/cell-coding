
SPUC       = spu-gcc
SPUCFLAGS  = -I../.. -Wall -std=c99 -fdata-sections -ffunction-sections
SPUOBJCOPY = spu-objcopy
# Important: Add the -m32 here to make sure that
# addresses on the SPU are not truncated.
PPUC       = ppu32-gcc -m32
PPUCFLAGS  = -I../.. -Wall -std=c99

default: stream

main.o: main.c
	$(PPUC) $(PPUCFLAGS) $< -c -o $@

spu.o: spu.c
	$(SPUC) $(SPUCFLAGS) $< -o spu.x
	ppu-embedspu -m32 stream_spu spu.x $@

stream: main.o spu.o
	$(PPUC) -o $@ $^ -lspe2

clean:
	rm -f stream main.o spu.o *.x

