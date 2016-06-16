mmm8x8: main.o serial.o command.o pattern.o crc16.o
	gcc -o mmm8x8 main.o serial.o command.o pattern.o crc16.o 

main.o: main.c serial.h command.h pattern.h crc16.h
	gcc -c main.c -I.

serial.o: serial.c serial.h
	gcc -c serial.c -I.

command.o: command.c command.h
	gcc -c command.c -I.

pattern.o: pattern.c pattern.h
	gcc -c pattern.c -I.

crc16.o: crc16.c crc16.h 
	gcc -c crc16.c -I.

clean:
	rm -f mmm8x8 *.o
