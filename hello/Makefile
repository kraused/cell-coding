
SPUC      = spu-gcc
SPUCFLAGS = -I.. -Wall -std=c99
PPUC      = ppu-gcc
PPUCFLAGS = -I.. -Wall -std=c99

default: hello

main.o: main.c
	$(PPUC) $(PPUCFLAGS) $< -c -o $@

spu: spu.c
	$(SPUC) $(SPUCFLAGS) $< -o $@

spu.o: spu
	ppu-embedspu -m64 spu_program $< $@

hello: main.o spu.o
	$(PPUC) -o $@ $^ -lspe2

clean:
	rm -f hello main.o spu spu.o

