.phony all: 
all: mts

mts: mts.c
	gcc -o mts mts.c -lpthread


.PHONY clean:
clean:
	-rm -f mts
