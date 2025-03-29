CC = gcc
CFLAGS = -Wall -Wextra -g -I./include
LDFLAGS = -lncurses -lm

TARGET = tbudget
SRC_DIR = src
SRC = $(SRC_DIR)/main.c $(SRC_DIR)/actions.c $(SRC_DIR)/utils.c $(SRC_DIR)/ui.c $(SRC_DIR)/globals.c $(SRC_DIR)/piechart.c $(SRC_DIR)/flexlayout.c
PREFIX = /usr/local

all: $(TARGET)

$(TARGET): $(SRC)
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