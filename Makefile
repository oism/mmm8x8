mmm8x8: main.o crc16.o
	gcc -o mmm8x8 main.o crc16.o

crc16.o: crc16.c 
	gcc -c crc16.c -I.

main.o: main.c 
	gcc -c main.c -I.

clean:
	rm -f mmm8x8 *.o
