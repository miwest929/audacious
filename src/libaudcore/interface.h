/*
 * interface.h
 * Copyright 2014 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#ifndef LIBAUDCORE_INTERFACE_H
#define LIBAUDCORE_INTERFACE_H

enum {
    AUD_MENU_MAIN,
    AUD_MENU_PLAYLIST,
    AUD_MENU_PLAYLIST_ADD,
    AUD_MENU_PLAYLIST_REMOVE,
    AUD_MENU_COUNT
};

// this enum is also in plugin.h
#ifndef _AUD_VIS_TYPE_DEFINED
#define _AUD_VIS_TYPE_DEFINED
enum {
    AUD_VIS_TYPE_CLEAR,
    AUD_VIS_TYPE_MONO_PCM,
    AUD_VIS_TYPE_MULTI_PCM,
    AUD_VIS_TYPE_FREQ,
    AUD_VIS_TYPES
};
#endif

typedef void (* VisClearFunc) ();
typedef void (* VisMonoPCMFunc) (const float * pcm);
typedef void (* VisMultiPCMFunc) (const float * pcm, int channels);
typedef void (* VisFreqFunc) (const float * freq);

/* generic type */
typedef void (* VisFunc) ();

void aud_ui_show (bool show);
bool aud_ui_is_shown ();

void aud_ui_show_error (const char * message);  /* thread-safe */

void aud_ui_show_about_window ();
void aud_ui_hide_about_window ();
void aud_ui_show_filebrowser (bool open);
void aud_ui_hide_filebrowser ();
void aud_ui_show_jump_to_song ();
void aud_ui_hide_jump_to_song ();
void aud_ui_show_prefs_window ();
void aud_ui_hide_prefs_window ();

void aud_plugin_menu_add (int id, void (* func) (), const char * name, const char * icon);
void aud_plugin_menu_remove (int id, void (* func) ());

void aud_vis_func_add (int type, VisFunc func);
void aud_vis_func_remove (VisFunc func);

#endif
