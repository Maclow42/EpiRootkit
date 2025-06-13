ROOT_DIR := $(shell pwd)
BOOT_DIR := $(ROOT_DIR)/boot

all: prepare start doc

prepare:
	@echo "Preparing the environment..."
	cd $(BOOT_DIR)
	@chmod +x $(BOOT_DIR)/1__setup.sh
	@sudo $(BOOT_DIR)/1__setup.sh $(BOOT_DIR)

start:
	cd $(BOOT_DIR)
	@chmod +x $(BOOT_DIR)/2__launch.sh
	@$(BOOT_DIR)/2__launch.sh $(BOOT_DIR)

doc:
	@echo "Generating documentation..."
	@cd $(ROOT_DIR)/docs && doxygen Doxyfile > /dev/null 2>&1
	@echo "Documentation generated successfully."
	@echo "You can find the documentation in the 'docs/html' directory."
	@echo "To view the documentation, open 'file://$(ROOT_DIR)/docs/html/index.html' in your web browser."

clean:
	@echo "Cleaning up..."
	@echo "Removing generated documentation..."
	@rm -rf $(ROOT_DIR)/docs/html
	@echo "Cleaning network interfaces..."
	cd $(BOOT_DIR)
	@chmod +x $(BOOT_DIR)/3__clean.sh
	@$(BOOT_DIR)/3__clean.sh $(BOOT_DIR)
	@echo "Cleanup completed."
	
.PHONY: prepare doc start clean