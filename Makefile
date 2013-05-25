CC = gcc
INCLUDES = $(shell pkg-config --cflags gio-2.0 glib-2.0)
LIBS = $(shell pkg-config --libs gio-2.0 glib-2.0)
OPTIONS = -fPIC -shared
NAME = ddb_gnome_mmkeys
INSTALL_DIR = /usr/lib/deadbeef

all: plugin

plugin: $(NAME).so

$(NAME).so: $(NAME).c
	$(CC) $(INCLUDES) $(LIBS) $(OPTIONS) -o $(NAME).so $(NAME).c
	
install:
	cp $(NAME).so $(INSTALL_DIR)
	chmod 644 $(INSTALL_DIR)/$(NAME).so

clean:
	rm -f $(NAME).so
