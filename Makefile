# Top-level Makefile

.PHONY: all clean kernel_module library tests

all: kernel_module library tests

kernel_module:
	$(MAKE) -C src/driver

library:
	$(MAKE) -C src/lib

tests: library
	$(MAKE) -C tests

clean:
	$(MAKE) -C src/driver clean
	$(MAKE) -C src/lib clean
	$(MAKE) -C tests clean
