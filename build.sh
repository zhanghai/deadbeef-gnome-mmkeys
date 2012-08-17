#!/bin/sh

# thanks to http://stackoverflow.com/questions/5751276/compiling-c-program-with-dbus-header-files

CC='tcc'
#CC='gcc'
includes=$(pkg-config --cflags dbus-glib-1 dbus-1 glib-2.0)
libs=$(pkg-config --libs dbus-glib-1 dbus-1 glib-2.0)
options='-shared'
#options='-shared -std=c99 -O2'

{ $CC $includes $libs $options -o gnome_mmkeys.so gnome_mmkeys_marshal.c gnome_mmkeys.c &&

  echo 'build successful' || {
  echo 'build failed'
  echo 'did you install prerequisites? try:'
  echo '$ sudo apt-get install' $CC 'libglib2.0-dev libdbus-glib-1-dev deadbeef-plugins-dev'
  }
}

