all: buzzer.a

CPU             = msp430g2553
CFLAGS          = -mmcu=${CPU} -Os -I../h

#switch the compiler (for the internal make rules)
CC              = msp430-elf-gcc
AS              = msp430-elf-as
AR              = msp430-elf-ar

libBuzzer.a: buzzer.o
	$(AR) crs $@ $^

install: libBuzzer.a
	mkdir -p ../h ../lib
	mv $^ ../lib
	cp *.h ../h

clean:
	rm -f timerLib.a *.o

