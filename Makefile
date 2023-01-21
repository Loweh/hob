SRCFILES := $(shell find src -name '*.c')
INCLUDES := -lssl -lcrypto

compile:
	gcc $(SRCFILES) -o bin/hob -Wall $(INCLUDES)

run:
	make compile
	bin/hob

clean:
	rm -r bin
	mkdir bin