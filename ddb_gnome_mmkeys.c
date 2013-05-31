/*
    This is a plugin for DeaDBeeF player.
    http://deadbeef.sourceforge.net/

    Adds support for Gnome (via DBus) multimedia keys (Prev, Stop, Pause/Play, Next).

    Copyright (C) 2011-2012 Ruslan Khusnullin <ruslan.khusnullin@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
    Big thanks to:
    deadbeef.h
    http://developer.pidgin.im/wiki/DbusHowto
    http://dbus.freedesktop.org/doc/dbus-glib/
    http://sourceforge.net/apps/mediawiki/deadbeef/index.php?title=Plugin_HOWTO
    http://live.gnome.org/GnomeScreensaver/FrequentlyAskedQuestions
    http://dbus.freedesktop.org/doc/dbus-tutorial.html
    http://pragha.wikispaces.com/message/view/home/25294057
    http://www.vimtips.org/12/
    http://stackoverflow.com/questions/5751276/compiling-c-program-with-dbus-header-files
*/

#include <unistd.h>
#include <stdio.h>
#include <gio/gio.h>
#include <glib.h>
#include <deadbeef/deadbeef.h>

#define NAME "ddb_gnome_mmkeys"
#define DEBUG

typedef struct DB_mmkeys_plugin_s {
  DB_plugin_t plugin;
  DB_functions_t* deadbeef;
  GDBusProxy* proxy;
} DB_mmkeys_plugin;

DB_mmkeys_plugin plugin;

void process_key (GDBusProxy* dbus_proxy, const char* not_used, const char* key_pressed,
			  GVariant *parameters, gpointer user_data) {
    char *key, *application;
    int state = 0;

    if (plugin.deadbeef->conf_get_int ("ddb_gnome_mmkeys.enable", 0) == 0) {
        return;
    }
    
    g_variant_get (parameters, "(ss)", &application, &key);
    
#ifdef DEBUG
    g_debug("%s: got media key '%s' for application '%s'\n", NAME, key, application);
#endif

    state = plugin.deadbeef->get_output()->state();
    
    if (strcmp (key, "Play") == 0) {
        if (state == OUTPUT_STATE_PLAYING) {
            plugin.deadbeef->sendmessage (DB_EV_PAUSE, 0, 0, 0);
        } else {
            plugin.deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
        }
    } else if (strcmp (key, "Stop") == 0) {
            plugin.deadbeef->sendmessage (DB_EV_STOP, 0, 0, 0);
    } else if (strcmp (key, "Previous") == 0) {
            plugin.deadbeef->sendmessage (DB_EV_PREV, 0, 0, 0);
    } else if (strcmp (key, "Next") == 0) {
            plugin.deadbeef->sendmessage (DB_EV_NEXT, 0, 0, 0);
    }
}

static void
first_call_complete (GObject *proxy, GAsyncResult *res, DB_mmkeys_plugin *plugin) {
	GVariant *result;
	GError *error = NULL;

	result = g_dbus_proxy_call_finish (G_DBUS_PROXY (proxy), res, &error);
	if (error != NULL) {
		g_warning ("%s, Unable to grab media player keys: %s", NAME, error->message);
		g_clear_error (&error);
		return;
	}
#ifdef DEBUG
    g_debug("%s: Grab media player keys successfully\n", NAME);
#endif
	g_signal_connect (plugin->proxy, "g-signal", G_CALLBACK (process_key), NULL);
	
	g_variant_unref (result);
}
static void
final_call_complete (GObject *proxy, GAsyncResult *res, gpointer nothing)
{
	GError *error = NULL;
	GVariant *result;

	result = g_dbus_proxy_call_finish (G_DBUS_PROXY (proxy), res, &error);
	if (error != NULL) {
		g_warning ("%s: Unable to release media player keys: %s", NAME, error->message);
		g_clear_error (&error);
	} else {
#ifdef DEBUG
    g_debug("%s: Release media player keys successfully\n", NAME);
#endif
		g_variant_unref (result);
	}
}

void ddb_gnome_mmkeys_connect_to_dbus () {
    GError* dbus_error = NULL;
    GDBusConnection* connection = NULL;

    connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &dbus_error);
    
    if (connection == NULL) {
      g_warning ("%s: could not connect to dbus: %s\n", NAME, (*dbus_error).message);
      if (dbus_error != NULL) g_error_free (dbus_error);
    }

    plugin.proxy = g_dbus_proxy_new_sync (connection,
						       G_DBUS_PROXY_FLAGS_NONE,
						       NULL,
						       "org.gnome.SettingsDaemon", 
						       "/org/gnome/SettingsDaemon/MediaKeys", 
						       "org.gnome.SettingsDaemon.MediaKeys",
						       NULL,
						       &dbus_error);
						       
    if (plugin.proxy == NULL) {
        g_warning ("%s: dbus: could not sync with SettingsDaemon: %s\n", NAME, (*dbus_error).message);
        if (dbus_error != NULL) g_error_free (dbus_error);
    }

    
    g_dbus_proxy_call (plugin.proxy,
					   "GrabMediaPlayerKeys",
					   g_variant_new ("(su)", "DeadBeef", 0),
					   G_DBUS_CALL_FLAGS_NONE,
					   -1,
					   NULL,
					   (GAsyncReadyCallback) first_call_complete,
					   &plugin);

    if (dbus_error != NULL)
        g_error_free (dbus_error);
}

gboolean mmkeys_start_cb (gpointer ctx) {
    ddb_gnome_mmkeys_connect_to_dbus();
    return FALSE;
}

int ddb_gnome_mmkeys_start (void) {
    g_idle_add (mmkeys_start_cb, NULL);
    return 0;
}

int ddb_gnome_mmkeys_stop (void) {
    g_dbus_proxy_call (plugin.proxy,
					   "ReleaseMediaPlayerKeys",
					   g_variant_new ("(s)", "DeadBeef"),
					   G_DBUS_CALL_FLAGS_NONE,
					   -1,
					   NULL,
					   (GAsyncReadyCallback) final_call_complete,
					   NULL);
					   
		plugin.proxy = NULL;
					   
    return 0;
}


DB_plugin_t* ddb_gnome_mmkeys_load (DB_functions_t* api) {
    plugin.deadbeef = api;
    return DB_PLUGIN (&plugin);
}

DB_mmkeys_plugin plugin = {
    .plugin = {
      .type =          DB_PLUGIN_MISC, // type
      .api_vmajor =    1, // api version major
      .api_vminor =    1, // api version minor
      .version_major = 1, // version major
      .version_minor = 1, // version minor

      .flags =         0, // unused
      .reserved1 =     0, // unused
      .reserved2 =     0, // unused
      .reserved3 =     0, // unused

      .id =            "ddb_gnome_mmkeys", // id
      .name =          "Gnome multimedia keys support", // name
      .descr =         "Adds support for Gnome multimedia keys (Prev, Stop, Pause/Play, Next).", // description
      .copyright =     "Copyright (C) 2011-2012 Ruslan Khusnullin <ruslan.khusnullin@gmail.com>\n" // copyright
                       "Copyright (C) 2013 Bartłomiej Bułat <bartek.bulat@gmail.com>\n"
                       "\n"
                       "This program is free software: you can redistribute it and/or modify\n"
                       "it under the terms of the GNU General Public License as published by\n"
                       "the Free Software Foundation, either version 2 of the License, or\n"
                       "(at your option) any later version.\n"
                       "\n"
                       "This program is distributed in the hope that it will be useful,\n"
                       "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
                       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
                       "GNU General Public License for more details.\n"
                       "\n"
                       "You should have received a copy of the GNU General Public License\n"
                       "along with this program.  If not, see <http://www.gnu.org/licenses/>.\n",
      .website =       "http://code.google.com/p/deadbeef-gnome-mmkeys", // website

      .command =       NULL, // command interface function
      .start =         ddb_gnome_mmkeys_start, // start function
      .stop =          ddb_gnome_mmkeys_stop, // stop function
      .connect =       NULL, // connect function
      .disconnect =    NULL, // disconnect function
      .exec_cmdline =  NULL, // command line processing function
      .get_actions =   NULL, // linked list of actions function
      .message =       NULL, // message processing function
      .configdialog =  "property \"Enable\" checkbox ddb_gnome_mmkeys.enable 0;\n" // config dialog function
    },
    .proxy = NULL,
    .deadbeef = NULL
};

