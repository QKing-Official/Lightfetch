CC = gcc
CFLAGS = -Wall -std=c99
LDFLAGS =
TARGET = lightfetch
SRC = lightfetch.c
INSTALL_DIR = /usr/local/bin

# Detect operating system
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    # Check if we're in WSL
    ifeq ($(shell grep -q Microsoft /proc/version && echo 1 || echo 0),1)
        OS = WSL
    else
        OS = Linux
    endif
else ifeq ($(UNAME_S),Darwin)
    OS = macOS
else
    OS = Unknown
endif

# Install appropriate dependencies based on OS
install_deps:
        @echo "Installing dependencies for $(OS)..."
        @if [ "$(OS)" = "Linux" ] || [ "$(OS)" = "WSL" ]; then \
                if command -v apt-get >/dev/null 2>&1; then \
                        sudo apt-get update && sudo apt-get install -y util-linux pciutils procps coreutils; \
                elif command -v dnf >/dev/null 2>&1; then \
                        sudo dnf install -y util-linux pciutils procps-ng coreutils; \
                elif command -v pacman >/dev/null 2>&1; then \
                        sudo pacman -Sy --noconfirm util-linux pciutils procps-ng coreutils; \
                else \
                        echo "Unknown package manager, please install dependencies manually."; \
                fi \
        elif [ "$(OS)" = "macOS" ]; then \
                if command -v brew >/dev/null 2>&1; then \
                        brew install coreutils pciutils procps; \
                else \
                        echo "Homebrew not found. Please install Homebrew and try again."; \
                fi \
        else \
                echo "Unsupported OS, please install dependencies manually."; \
        fi

# Build the program
$(TARGET): $(SRC)
        $(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Install the program
install: $(TARGET)
        @echo "Installing $(TARGET) to $(INSTALL_DIR)..."
        @if [ -w $(INSTALL_DIR) ]; then \
                cp $(TARGET) $(INSTALL_DIR) && chmod +x $(INSTALL_DIR)/$(TARGET); \
        else \
                sudo cp $(TARGET) $(INSTALL_DIR) && sudo chmod +x $(INSTALL_DIR)/$(TARGET); \
        fi
        @echo "Installation complete!"

# Clean build artifacts
clean:
        rm -f $(TARGET)

# Run the program
run: $(TARGET)
        ./$(TARGET)

# First-time setup
first_run: install_deps $(TARGET)
        @echo "First-time setup complete!"

# Uninstall the program
uninstall:
        @echo "Uninstalling $(TARGET)..."
        @if [ -w $(INSTALL_DIR) ]; then \
                rm -f $(INSTALL_DIR)/$(TARGET); \
        else \
                sudo rm -f $(INSTALL_DIR)/$(TARGET); \
        fi
        @echo "Uninstallation complete!"

.PHONY: install_deps install clean run first_run uninstall
