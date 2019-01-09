/*
    This is a plugin for DeaDBeeF player.
    http://deadbeef.sourceforge.net/

    Adds support for Gnome (via DBus) multimedia keys (Prev, Stop, Pause/Play, Next).

    Copyright (C) 2016 Zhang Hai <dreaming.in.code.zh@gmail.com>
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

#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include <gio/gio.h>
#include <glib.h>

#include <deadbeef/deadbeef.h>

#define NAME "ddb_gnome_mmkeys"

#define DEBUG

typedef struct {
    DB_plugin_t plugin;
    DB_functions_t* deadbeef;
    GDBusProxy* proxy;
} DB_mmkeys_plugin;

DB_mmkeys_plugin plugin;

void process_key(GDBusProxy* dbus_proxy, const char* not_used,
                 const char* key_pressed, GVariant *parameters,
                 gpointer nothing) {

    char *key, *application;
    int state = 0;

    if (!plugin.deadbeef->conf_get_int("ddb_gnome_mmkeys.enable", 1)) {
        return;
    }

    g_variant_get(parameters, "(ss)", &application, &key);
#ifdef DEBUG
    g_debug("%s: got media key '%s' for application '%s'\n", NAME, key,
            application);
#endif
    state = plugin.deadbeef->get_output()->state();

    if (strcmp(key, "Play") == 0) {
        if (state == OUTPUT_STATE_PLAYING) {
            plugin.deadbeef->sendmessage(DB_EV_PAUSE, 0, 0, 0);
        } else {
            plugin.deadbeef->sendmessage(DB_EV_PLAY_CURRENT, 0, 0, 0);
        }
    } else if (strcmp(key, "Stop") == 0) {
            plugin.deadbeef->sendmessage(DB_EV_STOP, 0, 0, 0);
    } else if (strcmp(key, "Previous") == 0) {
            plugin.deadbeef->sendmessage(DB_EV_PREV, 0, 0, 0);
    } else if (strcmp(key, "Next") == 0) {
            plugin.deadbeef->sendmessage(DB_EV_NEXT, 0, 0, 0);
    }
}

void first_call_complete(GObject *proxy, GAsyncResult *res, gpointer nothing) {

    GError *error = NULL;
    GVariant *result;

    result = g_dbus_proxy_call_finish(G_DBUS_PROXY(proxy), res, &error);
    if (error) {
        g_warning("%s, Unable to grab media player keys: %s", NAME,
                  error->message);
        g_clear_error(&error);
        return;
    }

#ifdef DEBUG
    g_debug("%s: Grab media player keys successfully\n", NAME);
#endif
    g_signal_connect(plugin.proxy, "g-signal", G_CALLBACK(process_key), NULL);
    g_variant_unref(result);
}

void final_call_complete(GObject *proxy, GAsyncResult *res, gpointer nothing) {

    GError *error = NULL;
    GVariant *result;

    result = g_dbus_proxy_call_finish(G_DBUS_PROXY(proxy), res, &error);
    if (error) {
        g_warning("%s: Unable to release media player keys: %s", NAME,
                  error->message);
        g_clear_error(&error);
        return;
    }

#ifdef DEBUG
    g_debug("%s: Release media player keys successfully\n", NAME);
#endif
    g_variant_unref(result);
}

GDBusProxy *create_dbus_proxy(GDBusConnection* connection, const gchar *name,
                              const gchar *object_path, const gchar *interface_name) {
    GError* error = NULL;
    GDBusProxy *proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE, NULL, name,
                                              object_path, interface_name, NULL, &error);
    if (!proxy) {
        g_warning("%s: could not create dbus proxy: %s\n", NAME, error->message);
        g_error_free(error);
    }
    return proxy;
}

void ddb_gnome_mmkeys_connect_to_dbus() {

    GError* dbus_error = NULL;
    GDBusConnection* connection = NULL;

    connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &dbus_error);
    if (connection == NULL) {
        g_warning("%s: could not connect to dbus: %s\n", NAME,
                  dbus_error->message);
        if (dbus_error) {
            g_error_free(dbus_error);
        }
    }

    if (plugin.deadbeef->conf_get_int("ddb_gnome_mmkeys.mate", 0) == 1) {
        plugin.proxy = create_dbus_proxy(connection, "org.mate.SettingsDaemon",
                                         "/org/mate/SettingsDaemon/MediaKeys",
                                         "org.mate.SettingsDaemon.MediaKeys");
    } else {
        /*
         * The MediaKeys plugin for gnome-settings-daemon <= 3.24.1 used the
         * bus name org.gnome.SettingsDaemon despite the documentation stating
         * that org.gnome.SettingsDaemon.MediaKeys should be used.
         * gnome-settings-daemon > 3.24.1 changed the bus name to match the
         * documentation.
         */
        plugin.proxy = create_dbus_proxy(connection, "org.gnome.SettingsDaemon.MediaKeys",
                                         "/org/gnome/SettingsDaemon/MediaKeys",
                                         "org.gnome.SettingsDaemon.MediaKeys");
        gboolean try_old_proxy_name = FALSE;
        if (!plugin.proxy) {
            try_old_proxy_name = TRUE;
        } else {
            gchar *name_owner = g_dbus_proxy_get_name_owner(plugin.proxy);
            if (name_owner) {
                g_free(name_owner);
            } else {
                try_old_proxy_name = TRUE;
            }
        }
        if (try_old_proxy_name) {
            if (plugin.proxy) {
                g_object_unref(plugin.proxy);
            }
            plugin.proxy = create_dbus_proxy(connection, "org.gnome.SettingsDaemon",
                                             "/org/gnome/SettingsDaemon/MediaKeys",
                                             "org.gnome.SettingsDaemon.MediaKeys");
        }
    }

    g_dbus_proxy_call(plugin.proxy,
                      "GrabMediaPlayerKeys",
                      g_variant_new("(su)", "DeadBeef", 0),
                      G_DBUS_CALL_FLAGS_NONE,
                      -1,
                      NULL,
                      first_call_complete,
                      NULL);

    if (dbus_error) {
        g_error_free(dbus_error);
    }
}

gboolean mmkeys_start_cb(gpointer nothing) {
    ddb_gnome_mmkeys_connect_to_dbus();
    return FALSE;
}

int ddb_gnome_mmkeys_start() {
    g_idle_add(mmkeys_start_cb, NULL);
    return 0;
}

int ddb_gnome_mmkeys_stop() {
    g_dbus_proxy_call(plugin.proxy,
                      "ReleaseMediaPlayerKeys",
                      g_variant_new("(s)", "DeadBeef"),
                      G_DBUS_CALL_FLAGS_NONE,
                      -1,
                      NULL,
                      (GAsyncReadyCallback) final_call_complete,
                      NULL);
    plugin.proxy = NULL;
    return 0;
}

DB_plugin_t* ddb_gnome_mmkeys_load(DB_functions_t* api) {
    plugin.deadbeef = api;
    return DB_PLUGIN(&plugin);
}

DB_mmkeys_plugin plugin = {
    .plugin = {

        .type = DB_PLUGIN_MISC,
        .api_vmajor = 1,
        .api_vminor = 1,
        .version_major = 1,
        .version_minor = 1,

        .flags = 0, // unused
        .reserved1 = 0, // unused
        .reserved2 = 0, // unused
        .reserved3 = 0, // unused

        .id = "ddb_gnome_mmkeys",
        .name = "Gnome multimedia keys support",
        .descr = "Adds support for Gnome multimedia keys (Prev, Stop, Pause/Play, Next).",
        .copyright = "Copyright (C) 2011-2012 Ruslan Khusnullin <ruslan.khusnullin@gmail.com>\n"
                     "Copyright (C) 2013-2016 Bartłomiej Bułat <bartek.bulat@gmail.com>\n"
                     "Copyright (C) 2016 Zhang Hai <dreaming.in.code.zh@gmail.com>\n"
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
        .website = "https://github.com/DreaminginCodeZH/deadbeef-gnome-mmkeys",

        .command = NULL,
        .start = ddb_gnome_mmkeys_start,
        .stop = ddb_gnome_mmkeys_stop,
        .connect = NULL,
        .disconnect = NULL,
        .exec_cmdline = NULL,
        .get_actions = NULL,
        .message = NULL,
        .configdialog = "property \"Enable\" checkbox ddb_gnome_mmkeys.enable 1;\n"
                        "property \"MATE support\" checkbox ddb_gnome_mmkeys.mate 0;\n"
    },
    .proxy = NULL,
    .deadbeef = NULL
};
