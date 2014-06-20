
SPUC       	= spu-gcc
SPUCFLAGS 	= -I/opt/cell/sdk/usr/spu/include -I.. -Wall -std=c99
SPULDFLAGS	= -L/opt/cell/sdk/usr/spu/lib
SPULIBS 	= -lsync
PPUC       	= ppu-gcc
PPUCFLAGS  	= -I/opt/cell/sdk/usr/include -I.. -Wall -std=c99

default: ring

main.o: main.c
	$(PPUC) $(PPUCFLAGS) $< -c -o $@

spu: spu.c
	$(SPUC) $(SPUCFLAGS) $< -o $@ $(SPULDFLAGS) $(SPULIBS)

spu.o: spu
	ppu-embedspu -m64 spu_program $< $@

ring: main.o spu.o
	$(PPUC) -o $@ $^ -lspe2

clean:
	rm -f ring main.o spu spu.o

