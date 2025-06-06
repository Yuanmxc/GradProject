CLANG ?= clang
CLANG++ ?= clang++
CFLAGS := -g -O2 -Wall
ARCH := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/')

BPF_CFLAGS := -target bpf -D__TARGET_ARCH_$(ARCH) -g -O2 -D__BPF_TRACING__

all: build/test build/dynlib_monitor

build:
	mkdir -p build

build/test: src/test.cpp build
	$(CLANG++) $(CFLAGS) -o $@ $< -ldl

build/dynlib_monitor: src/dynlib_monitor.cpp src/dynlib_monitor.skel.h
	$(CLANG++) $(CFLAGS) -o $@ $< -lbpf -lelf

src/dynlib_monitor.skel.h: src/dynlib_monitor.bpf.o
	bpftool gen skeleton $< > $@

src/dynlib_monitor.bpf.o: src/dynlib_monitor.bpf.c
	$(CLANG) $(BPF_CFLAGS) -I/usr/include/$(shell uname -m)-linux-gnu -c -o $@ $<

clean:
	rm -rf build src/*.o src/*.skel.h

.PHONY: all clean build 