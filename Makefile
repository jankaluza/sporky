CPPFLAGS=-I. -g -fPIC -shared -Wl,-E \
-I /usr/lib/jvm/java-1.6.0-openjdk-1.6.0.0.x86_64/include \
-I /usr/lib/jvm/java-1.6.0-openjdk-1.6.0.0.x86_64/include/linux \
-I /usr/include/glib-2.0/ \
-I /usr/lib64/glib-2.0/include/ \
-I /usr/local/include/libpurple/ \
-lglib-2.0 -lpurple

#OBJ = token.o lexparser.o syntax.o command.o variable.o function.o main.o \
#commands/dim.o \
#commands/scope.o

#%.o: %.c $(DEPS)
#	$(CC) -c -o $@ $< $(CFLAGS)

all: libsporky.so

Sporky.class: Sporky.java
	javac Sporky.java

Sporky.h: Sporky.class
	javah Sporky

Session.h: Sporky.class
	javah Session

libsporky.so: Sporky.h Session.h Sporky.cpp geventloop.h debugstuff.h
	g++ $(CPPFLAGS) -o libsporky.so Sporky.cpp geventloop.c debugstuff.c

clean:
	rm Sporky.h Sporky.class Session.class Buddy.class libsporky.so

test:
	java -Djava.library.path=./ Sporky