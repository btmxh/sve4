# Get all .unix.txt files that are NOT already .dos.txt
SRC := $(wildcard *.unix.txt)
DST := $(SRC:.unix.txt=.dos.txt)

# Default target
all: $(DST)

# Conversion rule
%.dos.txt: %.unix.txt
	sed 's/$$/\r/' $< > $@

# Clean generated files
clean:
	rm -f $(DST)
