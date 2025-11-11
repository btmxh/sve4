# Get all .txt files that are NOT already .dos.txt
SRC := $(filter-out %.dos.txt, $(wildcard *.txt))
DST := $(SRC:.txt=.dos.txt)

# Default target
all: $(DST)

# Conversion rule
%.dos.txt: %.txt
	sed 's/$$/\r/' $< > $@

# Clean generated files
clean:
	rm -f $(DST)
