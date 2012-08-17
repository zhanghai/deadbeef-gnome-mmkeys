#!/bin/sh

name='ddb_gnome_mmkeys'

[ $# -gt 0 ] && {
  [ "$1" = 'clean' ] && {
    rm -vf ${name}_marshal.list ${name}_marshal.c ${name}_marshal.h ${name}.so &&
    echo 'clean successful'
    exit 0
  }
  [ "$1" = 'install' ] && {
    [ -f ${name}.so ] || sh $0
    sudo cp ${name}.so /usr/lib/deadbeef/ &&
    sudo chmod 644 /usr/lib/deadbeef/${name}.so &&
    echo 'installation successful' || {
      echo 'installation failed'
      exit 1
    }
    exit 0
  }
  echo 'usage:' $0 '[clean|install]'
  echo 'without arguments will run build procedure'
  exit 1
}

CC='tcc'
#CC='gcc'
includes=$(pkg-config --cflags dbus-glib-1 dbus-1 glib-2.0)
libs=$(pkg-config --libs dbus-glib-1 dbus-1 glib-2.0)
options='-shared'
#options='-shared -std=c99 -O2'

{ echo 'VOID:STRING,STRING' >${name}_marshal.list
  glib-genmarshal --header --prefix=marshal ${name}_marshal.list > ${name}_marshal.h &&
  glib-genmarshal --body   --prefix=marshal ${name}_marshal.list > ${name}_marshal.c &&
  echo 'marshal generation successful' || {
    echo 'marshal generation failed'
    echo 'did you install prerequisites? try:'
    echo '$ sudo apt-get install libglib2.0-dev'
    exit 1
  }
}

{ $CC $includes $libs $options -o ${name}.so ${name}_marshal.c ${name}.c &&
  echo 'build successful' || {
  echo 'build failed'
  echo 'did you install prerequisites? try:'
  echo '$ sudo apt-get install' $CC 'libglib2.0-dev libdbus-glib-1-dev deadbeef-plugins-dev'
  exit 1
  }
}

