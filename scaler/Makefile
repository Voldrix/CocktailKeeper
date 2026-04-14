CC = gcc
CFLAGS = -Wall -mavx2 -mbmi2 -O2 -ffast-math -fno-math-errno -fno-tree-loop-distribute-patterns -Ijpeg-9e
LDFLAGS = -lm -l:libjpeg.a -lpng -lwebp -lwebpdemux -lvpx

PROC := $(shell grep -om1 Intel /proc/cpuinfo)
ifeq ($(PROC),Intel)
  TUNE = -mtune=intel
else
  TUNE = -mtune=native
endif

.SILENT: scaler clean
.PHONY: clean all

all: clean scaler

scaler: main.c
	$(CC) $(CFLAGS) $(TUNE) $^ $(LDFLAGS) -o ckeeper.scaler
	echo "ckeeper.scaler"

clean:
	rm -f ckeeper.scaler *.o

