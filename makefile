# includes and libs lists got from:
# pkg-config --cflags dbus-glib-1 dbus-1 glib-2.0
# pkg-config --libs dbus-glib-1 dbus-1 glib-2.0
# thanks to http://stackoverflow.com/questions/5751276/compiling-c-program-with-dbus-header-files

INCLUDES = -I/usr/include/dbus-1.0/ -I/usr/lib/dbus-1.0/include/ -I/usr/include/glib-2.0/ -I/usr/lib/glib-2.0/include/
LIBS = -pthread -L/lib -ldbus-glib-1 -ldbus-1 -lpthread -lgobject-2.0 -lgthread-2.0 -lrt -lglib-2.0

all:
	gcc $(INCLUDES) $(LIBS) -std=c99 -shared -O2 -o gnome_mmkeys.so gnome_mmkeys_marshal.c gnome_mmkeys.c
	chmod 644 gnome_mmkeys.so

install: all
	sudo cp gnome_mmkeys.so /usr/lib/deadbeef/

clean:
	rm gnome_mmkeys.so
