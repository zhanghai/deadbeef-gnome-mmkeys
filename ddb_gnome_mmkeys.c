/*
    This is a plugin for DeaDBeeF player.
    http://deadbeef.sourceforge.net/

    Adds support for GNOME (via DBus) multimedia keys (Prev, Stop, Pause/Play, Next).

    Copyright (C) 2011-2012 Ruslan Khusnullin <ruslan.khusnullin@gmail.com>
    Copyright (C) 2013-2016 Bartłomiej Bułat <bartek.bulat@gmail.com>
    Copyright (C) 2016 Hai Zhang <dreaming.in.code.zh@gmail.com>

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

#define PLUGIN_ID "ddb_gnome_mmkeys"

#define DEBUG

typedef struct {
    DB_plugin_t plugin;
    DB_functions_t *functions;
    GDBusProxy *proxy;
} ddb_gnome_mmkeys_plugin_t;

static ddb_gnome_mmkeys_plugin_t plugin;

static GDBusProxy *create_dbus_proxy(GDBusConnection *connection, const gchar *name,
                                     const gchar *object_path, const gchar *interface_name) {
    GError *error = NULL;
    GDBusProxy *proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE, NULL, name,
                                              object_path, interface_name, NULL, &error);
    if (error) {
        g_warning("%s: Failed to create DBus proxy: %s", PLUGIN_ID, error->message);
        g_clear_error(&error);
        return NULL;
    }
    return proxy;
}

static void media_player_key_pressed_callback(GDBusProxy *proxy, gchar *sender_name,
                                              gchar *signal_name, GVariant *parameters,
                                              gpointer user_data) {

    if (!plugin.functions->conf_get_int("ddb_gnome_mmkeys.enable", 1)) {
        return;
    }

    gchar *application;
    gchar *key;
    g_variant_get(parameters, "(ss)", &application, &key);
#ifdef DEBUG
    g_debug("%s: Media player key '%s' pressed for application '%s'", PLUGIN_ID, key,
            application);
#endif
    if (strcmp(key, "Play") == 0) {
        if (plugin.functions->get_output()->state() == OUTPUT_STATE_PLAYING) {
            plugin.functions->sendmessage(DB_EV_PAUSE, 0, 0, 0);
        } else {
            plugin.functions->sendmessage(DB_EV_PLAY_CURRENT, 0, 0, 0);
        }
    } else if (strcmp(key, "Stop") == 0) {
        plugin.functions->sendmessage(DB_EV_STOP, 0, 0, 0);
    } else if (strcmp(key, "Previous") == 0) {
        plugin.functions->sendmessage(DB_EV_PREV, 0, 0, 0);
    } else if (strcmp(key, "Next") == 0) {
        plugin.functions->sendmessage(DB_EV_NEXT, 0, 0, 0);
    }
}

static void grab_media_player_keys_callback(GObject *proxy, GAsyncResult *res, gpointer user_data) {

    GError *error = NULL;
    GVariant *value = g_dbus_proxy_call_finish(G_DBUS_PROXY(proxy), res, &error);
    if (error) {
        g_warning("%s: Failed to grab media player keys: %s", PLUGIN_ID, error->message);
        g_clear_error(&error);
        return;
    }
    g_variant_unref(value);
#ifdef DEBUG
    g_debug("%s: Grabbed media player keys", PLUGIN_ID);
#endif

    g_signal_connect(plugin.proxy, "g-signal", G_CALLBACK(media_player_key_pressed_callback), NULL);
}

static void grab_media_player_keys() {

    GError *error = NULL;
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (error) {
        g_warning("%s: Failed not connect to dbus: %s", PLUGIN_ID, error->message);
        if (error) {
            g_clear_error(&error);
        }
        return;
    }

    if (plugin.functions->conf_get_int("ddb_gnome_mmkeys.mate", 0) == 1) {
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
        if (plugin.proxy) {
            gchar *name_owner = g_dbus_proxy_get_name_owner(plugin.proxy);
            if (!name_owner) {
                g_clear_object(&plugin.proxy);
            } else {
                g_free(name_owner);
            }
        }
        if (!plugin.proxy) {
            plugin.proxy = create_dbus_proxy(connection, "org.gnome.SettingsDaemon",
                                             "/org/gnome/SettingsDaemon/MediaKeys",
                                             "org.gnome.SettingsDaemon.MediaKeys");
        }
    }
    g_clear_object(&connection);
    if (!plugin.proxy) {
        return;
    }

    g_dbus_proxy_call(plugin.proxy, "GrabMediaPlayerKeys", g_variant_new("(su)", "DeadBeef", 0),
                      G_DBUS_CALL_FLAGS_NONE, -1, NULL, grab_media_player_keys_callback, NULL);
}

static void release_media_player_keys_callback(GObject *source_object, GAsyncResult *res,
                                               gpointer user_data) {
    GDBusProxy *proxy = G_DBUS_PROXY(source_object);
    GError *error = NULL;
    GVariant *value = g_dbus_proxy_call_finish(proxy, res, &error);
    if (error) {
        g_warning("%s: Failed to release media player keys: %s", PLUGIN_ID, error->message);
        g_clear_error(&error);
        return;
    }
    g_variant_unref(value);
#ifdef DEBUG
    g_debug("%s: Released media player keys", PLUGIN_ID);
#endif
}

static void release_media_player_keys() {
    if (!plugin.proxy) {
        return;
    }
    g_dbus_proxy_call(plugin.proxy, "ReleaseMediaPlayerKeys", g_variant_new("(s)", "DeadBeef"),
                      G_DBUS_CALL_FLAGS_NONE, -1, NULL, release_media_player_keys_callback, NULL);
    g_clear_object(&plugin.proxy);
}

static gboolean grab_media_player_keys_function(gpointer user_data) {
    grab_media_player_keys();
    return FALSE;
}

static int plugin_start() {
    g_idle_add(grab_media_player_keys_function, NULL);
    return 0;
}

static int plugin_stop() {
    release_media_player_keys();
    return 0;
}

static ddb_gnome_mmkeys_plugin_t plugin = {
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

        .id = PLUGIN_ID,
        .name = "GNOME multimedia keys support",
        .descr = "Adds support for GNOME multimedia keys (Play/Pause, Stop, Prev, Next).",
        .copyright = "Copyright (C) 2011-2012 Ruslan Khusnullin <ruslan.khusnullin@gmail.com>\n"
                     "Copyright (C) 2013-2016 Bartłomiej Bułat <bartek.bulat@gmail.com>\n"
                     "Copyright (C) 2016 Hai Zhang <dreaming.in.code.zh@gmail.com>\n"
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
        .start = plugin_start,
        .stop = plugin_stop,
        .connect = NULL,
        .disconnect = NULL,
        .exec_cmdline = NULL,
        .get_actions = NULL,
        .message = NULL,
        .configdialog = "property \"Enable\" checkbox ddb_gnome_mmkeys.enable 1;\n"
                        "property \"MATE support\" checkbox ddb_gnome_mmkeys.mate 0;\n"
    },
    .functions = NULL,
    .proxy = NULL
};

DB_plugin_t *ddb_gnome_mmkeys_load(DB_functions_t *api) {
    plugin.functions = api;
    return &plugin.plugin;
}
