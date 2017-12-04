TARGET=CMP

$(TARGET):main.o
	gcc main.o -o $(TARGET) -lm
main.o:main.c simulator.h cmp.h cmpfunction.h cmpdevice.h queue.h
	gcc -c main.c
clean:
	rm -f $(TARGET) *.o *.bin *.rpt *.exe *.out

