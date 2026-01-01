CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99
LDFLAGS = 

# Object files
DISK_OBJ = disk.o
JOURNAL_OBJ = journal.o
MAIN_OBJ = main.o
MKFS_OBJ = mkfs.o

# Executables
VSFS = vsfs
MKFS = mkfs.vsfs

all: $(VSFS) $(MKFS)

$(VSFS): $(MAIN_OBJ) $(JOURNAL_OBJ) $(DISK_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(MKFS): $(MKFS_OBJ) $(DISK_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

main.o: main.c vsfs.h disk.h journal.h
disk.o: disk.c disk.h vsfs.h
journal.o: journal.c journal.h disk.h vsfs.h
mkfs.o: mkfs.c vsfs.h disk.h

clean:
	rm -f $(VSFS) $(MKFS) *.o *.img

test: $(VSFS) $(MKFS)
	@echo "Running basic tests..."
	./$(MKFS) test.img
	@echo ""
	@echo "=== Initial state ==="
	./$(VSFS) test.img stat
	./$(VSFS) test.img ls
	@echo ""
	@echo "=== Creating files (journaled) ==="
	./$(VSFS) test.img create file1.txt
	./$(VSFS) test.img create file2.txt
	./$(VSFS) test.img create file3.txt
	@echo ""
	@echo "=== Before install ==="
	./$(VSFS) test.img ls
	./$(VSFS) test.img check
	@echo ""
	@echo "=== Installing journal ==="
	./$(VSFS) test.img install
	@echo ""
	@echo "=== After install ==="
	./$(VSFS) test.img ls
	./$(VSFS) test.img stat
	./$(VSFS) test.img check
	@echo ""
	@echo "=== Creating more files ==="
	./$(VSFS) test.img create file4.txt
	./$(VSFS) test.img create file5.txt
	./$(VSFS) test.img install
	./$(VSFS) test.img ls
	./$(VSFS) test.img check

.PHONY: all clean test
