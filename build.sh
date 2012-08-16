#!/bin/sh

# thanks to http://stackoverflow.com/questions/5751276/compiling-c-program-with-dbus-header-files

includes=$(pkg-config --cflags dbus-glib-1 dbus-1 glib-2.0)
libs=$(pkg-config --libs dbus-glib-1 dbus-1 glib-2.0)
options="-std=c99 -shared -O2"

gcc $includes $libs $options -o gnome_mmkeys.so gnome_mmkeys_marshal.c gnome_mmkeys.c

[ $? -eq 0 ] && echo 'build successful' || {
  echo 'build failed'
  echo 'did you install prerequisites?'
  echo '  sudo apt-get install libdbus-glib-1-dev deadbeef-plugins-dev'
}

