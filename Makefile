CC = gcc
CFLAGS = -Wall -Wextra -g -I./include
LDFLAGS = -lncurses -lm

SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*/*.c)

TARGET = tbudget
PREFIX = /usr/local

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

install: $(TARGET)
	@echo "Installing $(TARGET) to $(PREFIX)/bin..."
	@mkdir -p $(PREFIX)/bin
	@cp $(TARGET) $(PREFIX)/bin/
	@chmod 755 $(PREFIX)/bin/$(TARGET)
	@echo "Installation complete. You can now run '$(TARGET)' from anywhere."

uninstall:
	@echo "Uninstalling $(TARGET) from $(PREFIX)/bin..."
	@rm -f $(PREFIX)/bin/$(TARGET)
	@echo "Uninstallation complete."

clean:
	rm -f $(TARGET)

.PHONY: all clean install uninstall 