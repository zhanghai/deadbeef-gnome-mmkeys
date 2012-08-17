#!/bin/sh

{ glib-genmarshal --header --prefix=marshal marshal.list > gnome_mmkeys_marshal.h &&
  glib-genmarshal --body --prefix=marshal marshal.list > gnome_mmkeys_marshal.c &&
  echo 'marshal generation successful' ||
  echo 'marshal generation failed'
}

