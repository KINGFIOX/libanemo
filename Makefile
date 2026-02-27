# Makefile wrapper for Meson build
BUILD_DIR := build

.PHONY: all build clean distclean install reconfigure help
.PHONY: run-quick_start run-riscv_minimal

# Default target
all: build

# Configure (if needed) and compile
build:
	@if [ ! -d $(BUILD_DIR) ]; then \
		echo ">>> meson setup $(BUILD_DIR)"; \
		meson setup $(BUILD_DIR); \
	fi
	@echo ">>> meson compile -C $(BUILD_DIR)"
	@meson compile -C $(BUILD_DIR)

# Clean built artifacts, keep build dir
clean:
	@echo ">>> Cleaning $(BUILD_DIR)"
	@meson compile -C $(BUILD_DIR) --clean 2>/dev/null || true
	@echo "Done."

# Remove entire build directory (full clean)
distclean: clean
	@echo ">>> Removing $(BUILD_DIR)"
	@rm -rf $(BUILD_DIR)
	@echo "Done."

# Reconfigure from scratch (wipe + setup)
reconfigure:
	@echo ">>> Reconfiguring..."
	@rm -rf $(BUILD_DIR)
	@meson setup $(BUILD_DIR)
	@echo "Done. Run 'make' or 'make build' to compile."

# Install (after build)
install: build
	@echo ">>> Installing..."
	@meson install -C $(BUILD_DIR)
	@echo "Done."

# Run examples (convenience targets)
run-quick_start: build
	@./$(BUILD_DIR)/quick_start

run-riscv_minimal: build
	@./$(BUILD_DIR)/riscv_minimal

help:
	@echo "libanemo Makefile (Meson backend)"
	@echo ""
	@echo "Targets:"
	@echo "  make / make build   - Configure (if needed) and compile"
	@echo "  make clean          - Remove build artifacts, keep build dir"
	@echo "  make distclean      - Remove entire build directory"
	@echo "  make reconfigure    - Wipe and reconfigure (meson setup)"
	@echo "  make install       - Build and install to prefix"
	@echo "  make run-quick_start   - Build and run quick_start"
	@echo "  make run-riscv_minimal - Build and run riscv_minimal"
	@echo "  make help          - Show this help"
