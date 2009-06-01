default: dos2unix

all:	dos2unix link install


dos2unix:
	gcc $(CFLAGS) dos2unix.c -o dos2unix


link:	
	ln -s dos2unix mac2unix
	ln -s dos2unix.1 mac2unix.1		

clean:
	rm dos2unix

install:
	mkdir -p /resources/dos2unix/bin /resources/dos2unix/man/man1
	install -m 755 dos2unix /resources/dos2unix/bin
	install -m 644 dos2unix.1 /resources/dos2unix/man/man1
	install -m 755 mac2unix /resources/dos2unix/bin
	install -m 644 mac2unix.1 /resources/dos2unix/man/man1
