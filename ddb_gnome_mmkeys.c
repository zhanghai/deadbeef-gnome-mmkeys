/*
    This is a plugin for DeaDBeeF player.
    http://deadbeef.sourceforge.net/

    Adds support for Gnome (via DBus) multimedia keys (Prev, Stop, Pause/Play, Next).

    Copyright (C) 2011 Ruslan Khusnullin <ruslan.khusnullin@gmail.com>

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
#include <dbus/dbus-glib.h>
#include <deadbeef/deadbeef.h>
#include "ddb_gnome_mmkeys_marshal.h"

DB_functions_t* deadbeef = NULL;
DB_plugin_t plugin_info;

ddb_event_t* play_event = NULL;
ddb_event_t* pause_event = NULL;
ddb_event_t* stop_event = NULL;
ddb_event_t* prev_event = NULL;
ddb_event_t* next_event = NULL;

void process_key (DBusGProxy* dbus_proxy, const char* not_used, const char* key_pressed, gpointer user_data) {
    DB_output_t* output = NULL;
    int output_state = 0;
    gboolean is_playing = FALSE;

    if ((*deadbeef).conf_get_int("ddb_gnome_mmkeys.enable", 0) == 0) {
        return;
    }

    output = (*deadbeef).get_output();
    output_state = (*output).state();
    if (output_state == OUTPUT_STATE_STOPPED || output_state == OUTPUT_STATE_PAUSED) {
        is_playing = FALSE;
    }
    else if (output_state == OUTPUT_STATE_PLAYING) {
        is_playing = TRUE;
    }

//    g_print("ddb_gnome_mmkeys: key pressed: %s\n", key_pressed);
    if (g_strcmp0(key_pressed, "Play") == 0) {
        if (is_playing) {
            (*deadbeef).event_send(pause_event, 0, 0);
        }
        else {
            (*deadbeef).event_send(play_event, 0, 0);
        }
    }
    else if (g_strcmp0(key_pressed, "Stop") == 0) {
            (*deadbeef).event_send(stop_event, 0, 0);
    }
    else if (g_strcmp0(key_pressed, "Previous") == 0) {
            (*deadbeef).event_send(prev_event, 0, 0);
    }
    else if (g_strcmp0(key_pressed, "Next") == 0) {
            (*deadbeef).event_send(next_event, 0, 0);
    }
}

#define DBUS_SERVICE "org.gnome.SettingsDaemon"
#define DBUS_PATH "/org/gnome/SettingsDaemon/MediaKeys"
#define DBUS_INTERFACE "org.gnome.SettingsDaemon.MediaKeys"
#define DBUS_MEMBER "MediaPlayerKeyPressed"

void ddb_gnome_mmkeys_thread (void* context) {
    GError* dbus_error = NULL;
    DBusGConnection* dbus_connection = NULL;
    DBusGProxy* dbus_proxy = NULL;
    g_type_init();

    dbus_connection = dbus_g_bus_get(DBUS_BUS_SESSION, &dbus_error);
    if(dbus_connection == NULL) {
        g_printerr("ddb_gnome_mmkeys: could not connect to dbus: %s\n", (*dbus_error).message);
        if (dbus_error != NULL)
            g_error_free(dbus_error);
        (*deadbeef).thread_exit(NULL);
    }

    dbus_proxy = dbus_g_proxy_new_for_name(dbus_connection, DBUS_SERVICE, DBUS_PATH, DBUS_INTERFACE);
    if(dbus_proxy == NULL) {
        g_printerr("ddb_gnome_mmkeys: dbus: could not listen SettingsDaemon signals: %s\n", (*dbus_error).message);
        if (dbus_error != NULL)
            g_error_free(dbus_error);
        (*deadbeef).thread_exit(NULL);
    }

    dbus_g_object_register_marshaller(marshal_VOID__STRING_STRING, G_TYPE_NONE, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);
    dbus_g_proxy_add_signal(dbus_proxy, DBUS_MEMBER, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);
    dbus_g_proxy_connect_signal(dbus_proxy, DBUS_MEMBER, G_CALLBACK(process_key), dbus_connection, NULL);

    if (dbus_error != NULL)
        g_error_free(dbus_error);
    (*deadbeef).thread_exit(NULL);
}

int ddb_gnome_mmkeys_start (void) {
    intptr_t thread_id = 0;

    play_event = (*deadbeef).event_alloc(DB_EV_PLAY_CURRENT);
    pause_event = (*deadbeef).event_alloc(DB_EV_PAUSE);
    stop_event = (*deadbeef).event_alloc(DB_EV_STOP);
    prev_event = (*deadbeef).event_alloc(DB_EV_PREV);
    next_event = (*deadbeef).event_alloc(DB_EV_NEXT);

    thread_id = (*deadbeef).thread_start(ddb_gnome_mmkeys_thread, NULL);
    if (thread_id != 0)
        (*deadbeef).thread_detach(thread_id);
    return 0;
}

int ddb_gnome_mmkeys_stop (void) {
    (*deadbeef).event_free(play_event);
    (*deadbeef).event_free(pause_event);
    (*deadbeef).event_free(stop_event);
    (*deadbeef).event_free(prev_event);
    (*deadbeef).event_free(next_event);
    return 0;
}


DB_plugin_t* ddb_gnome_mmkeys_load (DB_functions_t* api) {
    deadbeef = api;
    return DB_PLUGIN(&plugin_info);
}

DB_plugin_t plugin_info = {
    .type =          DB_PLUGIN_MISC, // type
    .api_vmajor =    1, // api version major
    .api_vminor =    1, // api version minor
    .version_major = 1, // version major
    .version_minor = 0, // version minor

    .flags =         0, // unused
    .reserved1 =     0, // unused
    .reserved2 =     0, // unused
    .reserved3 =     0, // unused

    .id =            "ddb_gnome_mmkeys", // id
    .name =          "Gnome multimedia keys support", // name
    .descr =         "Adds support for Gnome multimedia keys (Prev, Stop, Pause/Play, Next).", // description
    .copyright =     "Copyright (C) 2011 Ruslan Khusnullin <ruslan.khusnullin@gmail.com>\n" // copyright
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
};

