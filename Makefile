# SOURCE: https://github.com/esp8266/arduino-esp8266fs-plugin/issues/51#issuecomment-739433154
sketch          := rotctl-simplefoc.ino
boardconfig     := esp32:esp32:esp32

ARDUINO_CLI ?= ~/arduino/arduino-cli
MKSPIFFS    ?= /home/martin/.arduino15/packages/esp32/tools/mkspiffs/0.2.3/mkspiffs
ESPTOOL     ?= /home/martin/.arduino15/packages/esp32/tools/esptool_py/5.1.0/esptool
PORT        ?= /dev/ttyUSB4
BC          ?= bc


all: build

.PHONY: build
build:
	$(ARDUINO_CLI) compile --fqbn $(boardconfig) --output-dir build --build-property compiler.cpp.extra_flags="-Isrc/Arduino-FOC/src" $(sketch)

.PHONY: upload
upload: # filesystem.bin
	$(ARDUINO_CLI) upload --fqbn $(boardconfig) --port $(PORT)

.PHONY: monitor
monitor:
	$(ARDUINO_CLI) monitor --fqbn $(boardconfig) --port $(PORT) -c baudrate=115200

.ONESHELL:
spiffs.bin: data/config.json
	SPIFFS_SIZE=$$(grep spiffs build/esp32.esp32.esp32/partitions.csv |cut -d, -f5)
	$(MKSPIFFS) -c data -s $$SPIFFS_SIZE $@

.PHONY: flash-fs
flash-fs: spiffs.bin
	SPIFFS_START=$$(grep spiffs build/esp32.esp32.esp32/partitions.csv |cut -d, -f4)
	@echo "=== Flashe SPIFFS ==="
	$(ESPTOOL) \
		--chip esp32 \
		--port $(PORT) \
		--before default-reset \
		--after hard-reset \
		write-flash -z \
		--flash-mode dio \
		--flash-freq 80m \
		--flash-size detect \
		$$SPIFFS_START spiffs.bin
.PHONY: deploy
deploy: build upload monitor

.PHONY: deploy-fs
deploy-fs: build flash-fs upload monitor

.PHONY: clean
clean:
	rm -rf build
	rm -f filesystem.bin
