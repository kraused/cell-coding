
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
	$(SPUC) $(SPUCFLAGS) $< -c -o $@

transform = \
	$(SPUOBJCOPY) --redefine-sym $(1)=main spu.o $(1).o; \
	$(SPUC) -Wl,--gc-sections -o $(1).x $(1).o; \
	ppu-embedspu -m32 stream_$(1)_spu $(1).x $(1).o
#; \
#	rm $(1).x

copy.o: spu.o
	$(call transform,copy)

scale.o: spu.o
	$(call transform,scale)

add.o: spu.o
	$(call transform,add)

triad.o: spu.o
	$(call transform,triad)

stream: main.o copy.o scale.o add.o triad.o
	$(PPUC) -o $@ $^ -lspe2

clean:
	rm -f stream main.o copy.o scale.o add.o triad.o spu.o *.x

