/*
 * libaudgui/util.c
 * Copyright 2010-2011 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#include <audacious/debug.h>
#include <audacious/gtk-compat.h>
#include <audacious/i18n.h>
#include <audacious/playlist.h>
#include <audacious/plugin.h>
#include <audacious/misc.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>

#include "config.h"
#include "init.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static GdkPixbuf * current_pixbuf;

void audgui_hide_on_delete (GtkWidget * widget)
{
    g_signal_connect (widget, "delete-event", (GCallback)
     gtk_widget_hide_on_delete, NULL);
}

static bool_t escape_cb (GtkWidget * widget, GdkEventKey * event, void
 (* action) (GtkWidget * widget))
{
    if (event->keyval == GDK_Escape)
    {
        action (widget);
        return TRUE;
    }

    return FALSE;
}

void audgui_hide_on_escape (GtkWidget * widget)
{
    g_signal_connect (widget, "key-press-event", (GCallback) escape_cb,
     (void *) gtk_widget_hide);
}

void audgui_destroy_on_escape (GtkWidget * widget)
{
    g_signal_connect (widget, "key-press-event", (GCallback) escape_cb,
     (void *) gtk_widget_destroy);
}

static void toggle_cb (GtkToggleButton * toggle, bool_t * setting)
{
    * setting = gtk_toggle_button_get_active (toggle);
}

void audgui_connect_check_box (GtkWidget * box, bool_t * setting)
{
    gtk_toggle_button_set_active ((GtkToggleButton *) box, * setting);
    g_signal_connect ((GObject *) box, "toggled", (GCallback) toggle_cb, setting);
}

void audgui_simple_message (GtkWidget * * widget, GtkMessageType type,
 const char * title, const char * text)
{
    AUDDBG ("%s\n", text);

    if (* widget != NULL)
    {
        const char * old = NULL;
        g_object_get ((GObject *) * widget, "text", & old, NULL);
        g_return_if_fail (old);

        int messages = GPOINTER_TO_INT (g_object_get_data ((GObject *)
         * widget, "messages"));

        if (messages > 10)
            text = _("\n(Further messages have been hidden.)");

        if (strstr (old, text))
            goto CREATED;

        char both[strlen (old) + strlen (text) + 2];
        snprintf (both, sizeof both, "%s\n%s", old, text);
        g_object_set ((GObject *) * widget, "text", both, NULL);

        g_object_set_data ((GObject *) * widget, "messages", GINT_TO_POINTER
         (messages + 1));

        goto CREATED;
    }

    * widget = gtk_message_dialog_new (NULL, 0, type, GTK_BUTTONS_OK, "%s", text);
    gtk_window_set_title ((GtkWindow *) * widget, title);

    g_object_set_data ((GObject *) * widget, "messages", GINT_TO_POINTER (1));

    g_signal_connect (* widget, "response", (GCallback) gtk_widget_destroy, NULL);
    audgui_destroy_on_escape (* widget);
    g_signal_connect (* widget, "destroy", (GCallback) gtk_widget_destroyed,
     widget);

CREATED:
    gtk_window_present ((GtkWindow *) * widget);
}

GdkPixbuf * audgui_pixbuf_from_data (void * data, int size)
{
    GdkPixbuf * pixbuf = NULL;
    GdkPixbufLoader * loader = gdk_pixbuf_loader_new ();
    GError * error = NULL;

    if (gdk_pixbuf_loader_write (loader, data, size, & error) &&
     gdk_pixbuf_loader_close (loader, & error))
    {
        if ((pixbuf = gdk_pixbuf_loader_get_pixbuf (loader)))
            g_object_ref (pixbuf);
    }
    else
        AUDDBG("error while loading pixbuf: %s\n", error->message);

    g_object_unref (loader);
    return pixbuf;
}

GdkPixbuf * audgui_pixbuf_for_entry (int list, int entry)
{
    char * name = aud_playlist_entry_get_filename (list, entry);
    g_return_val_if_fail (name, NULL);

    /* Don't get album art for network files -- too slow. */
    if (! strncmp (name, "http://", 7) || ! strncmp (name, "https://", 8) ||
     ! strncmp (name, "mms://", 6))
    {
        str_unref (name);
        goto FALLBACK;
    }

    AUDDBG ("Trying to load pixbuf for %s.\n", name);
    PluginHandle * decoder = aud_playlist_entry_get_decoder (list, entry, FALSE);
    if (! decoder)
        goto FALLBACK;

    void * data;
    int size;

    if (aud_file_read_image (name, decoder, & data, & size))
    {
        GdkPixbuf * p = audgui_pixbuf_from_data (data, size);
        g_free (data);
        if (p)
        {
            str_unref (name);
            return p;
        }
    }

    char * assoc = aud_get_associated_image_file (name);

    if (assoc)
    {
        GdkPixbuf * p = gdk_pixbuf_new_from_file (assoc, NULL);
        g_free (assoc);
        if (p)
        {
            str_unref (name);
            return p;
        }
    }

    str_unref (name);

FALLBACK:;
    AUDDBG ("Using fallback pixbuf.\n");
    static GdkPixbuf * fallback = NULL;
    if (! fallback)
    {
        char * path = g_strdup_printf ("%s/images/album.png",
         aud_get_path (AUD_PATH_DATA_DIR));
        fallback = gdk_pixbuf_new_from_file (path, NULL);
        g_free (path);
    }
    if (fallback)
        g_object_ref ((GObject *) fallback);
    return fallback;
}

void audgui_pixbuf_uncache (void)
{
    if (current_pixbuf)
    {
        g_object_unref ((GObject *) current_pixbuf);
        current_pixbuf = NULL;
    }
}

GdkPixbuf * audgui_pixbuf_for_current (void)
{
    if (! current_pixbuf)
    {
        int list = aud_playlist_get_playing ();
        current_pixbuf = audgui_pixbuf_for_entry (list, aud_playlist_get_position (list));
    }

    if (current_pixbuf)
        g_object_ref ((GObject *) current_pixbuf);

    return current_pixbuf;
}

void audgui_pixbuf_scale_within (GdkPixbuf * * pixbuf, int size)
{
    int width = gdk_pixbuf_get_width (* pixbuf);
    int height = gdk_pixbuf_get_height (* pixbuf);
    GdkPixbuf * pixbuf2;

    if (width > height)
    {
        height = size * height / width;
        width = size;
    }
    else
    {
        width = size * width / height;
        height = size;
    }

    if (width < 1)
        width = 1;
    if (height < 1)
        height = 1;

    pixbuf2 = gdk_pixbuf_scale_simple (* pixbuf, width, height,
     GDK_INTERP_BILINEAR);
    g_object_unref (* pixbuf);
    * pixbuf = pixbuf2;
}
