name:=esp8266.water.level.ino
port:=/dev/ttyUSB0
ip:=192.168.1.156

all:
	make clean
	make build
	make upload_usb

build: $(name)
	arduino-cli compile --output-dir ./build --fqbn esp8266:esp8266:nodemcu  $(name)
upload:
	espota.py -d -i $(ip) -f "build/$(name).bin"
upload_usb:
	arduino-cli upload --fqbn esp8266:esp8266:nodemcu -p $(port) .
clean:
	rm -rf build
