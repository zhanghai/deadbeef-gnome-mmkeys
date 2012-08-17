#!/bin/sh

{ cp ddb_gnome_mmkeys.so /usr/lib/deadbeef/ &&
  chmod 644 /usr/lib/deadbeef/ddb_gnome_mmkeys.so &&

  echo 'installation successful' || {
  echo 'installation failed'
  echo "usage: sudo sh $0"
  }
}

