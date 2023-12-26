name:=esp8266.water.level.ino
port:=/dev/ttyUSB0
ip:=192.168.0.203

all: clean build upload

build: $(name)
	bash -c ./prepare.sh
	arduino-cli compile --output-dir ./build --fqbn esp8266:esp8266:nodemcu  $(name)
upload:
	espota.py -p 8266 -r -d -i $(ip) -f "build/$(name).bin"
upload_usb:
	arduino-cli upload --fqbn esp8266:esp8266:nodemcu -p $(port) .
clean:
	rm -rf build
