mmm8x8: main.o serial.o crc16.o
	gcc -o mmm8x8 main.o serial.o crc16.o

main.o: main.c crc16.h serial.h
	gcc -c main.c -I.

serial.o: serial.c serial.h
	gcc -c serial.c -I.

crc16.o: crc16.c crc16.h 
	gcc -c crc16.c -I.

clean:
	rm -f mmm8x8 *.o
