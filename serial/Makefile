build: serial_without_global_vars.cpp
	g++ serial_without_global_vars.cpp -o no_global

run: build
	./no_global

global: serial.cpp
	g++ serial.cpp -o serial
	./serial

old: serial_old.cpp
	g++ serial_old.cpp -o serial_old
	./serial_old

clean:
	rm -f serial serial_old no_global
