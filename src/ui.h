/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2004 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#ifndef UI_H
#define UI_H


#include <config.h>
#include <gnome.h>
#include "actions.h"
#include "goo-stock.h"


static GtkActionEntry action_entries[] = {
	{ "CDMenu", NULL, N_("_CD") },
	{ "EditMenu", NULL, N_("_Edit") },
	{ "ViewMenu", NULL, N_("_View") },
	{ "HelpMenu", NULL, N_("_Help") },
	{ "CDCoverMenu", NULL, N_("CD C_over") },

	{ "About", GNOME_STOCK_ABOUT,
	  NULL, NULL,
	  N_("Information about the program"),
	  G_CALLBACK (activate_action_about) },
	{ "Contents", GTK_STOCK_HELP,
	  NULL, "F1",
	  N_("Display the manual"),
	  G_CALLBACK (activate_action_manual) },
	{ "Shortcuts", NULL,
	  N_("_Keyboard shortcuts"), "",
	  " ",
	  G_CALLBACK (activate_action_shortcuts) },
	{ "TogglePlay", GTK_STOCK_MEDIA_PLAY,
	  N_("_Play"), "space",
	  N_("Play/Pause"),
	  G_CALLBACK (activate_action_toggle_play) },
	{ "Play", GTK_STOCK_MEDIA_PLAY,
	  N_("_Play"), NULL,
	  N_("Play"),
	  G_CALLBACK (activate_action_play) },
	{ "PlaySelected", GTK_STOCK_MEDIA_PLAY,
	  N_("_Play"), NULL,
	  N_("Play this track"),
	  G_CALLBACK (activate_action_play_selected) },
	{ "Pause", GTK_STOCK_MEDIA_PAUSE,
	  N_("_Pause"), NULL,
	  N_("Pause"),
	  G_CALLBACK (activate_action_pause) },
	{ "Stop", GTK_STOCK_MEDIA_STOP,
	  N_("_Stop"), "Escape",
	  N_("Stop playing"),
	  G_CALLBACK (activate_action_stop) },
	{ "Next", GTK_STOCK_MEDIA_NEXT,
	  N_("_Next"), "N",
	  N_("Play the next track"),
	  G_CALLBACK (activate_action_next) },
	{ "Prev", GTK_STOCK_MEDIA_PREVIOUS,
	  N_("Pre_v"), "P",
	  N_("Play the previous track"),
	  G_CALLBACK (activate_action_prev) },
	{ "Eject", NULL,
	  N_("_Eject"), "J",
	  N_("Eject the CD"),
	  G_CALLBACK (activate_action_eject) },
	{ "EjectToolBar", GOO_STOCK_EJECT,
	  N_("_Eject"), "J",
	  N_("Eject the CD"),
	  G_CALLBACK (activate_action_eject) },	  
	{ "Preferences", GTK_STOCK_PREFERENCES,
	  NULL, NULL,
	  N_("Edit various preferences"),
	  G_CALLBACK (activate_action_preferences) },
	{ "Extract", GOO_STOCK_EXTRACT,
	  N_("E_xtract Tracks"), "E",
	  N_("Save the tracks to disk as files"),
	  G_CALLBACK (activate_action_extract) },
	{ "Properties", GTK_STOCK_PROPERTIES,
	  N_("_CD Properties"), NULL,
	  N_("Edit the CD artist, album and tracks titles"),
	  G_CALLBACK (activate_action_edit_cddata) },
	{ "Quit", GTK_STOCK_QUIT,
	  NULL, NULL,
	  N_("Quit the application"),
	  G_CALLBACK (activate_action_quit) },
	{ "PickCoverFromDisk", GTK_STOCK_OPEN,
	  N_("_Choose from Disk"), "",
	  N_("Choose a CD cover from the local disk"),
	  G_CALLBACK (activate_action_pick_cover_from_disk) },
	{ "SearchCoverFromWeb", NULL,
	  N_("_Search on Internet"), NULL,
	  N_("Search for a CD cover on Internet"),
	  G_CALLBACK (activate_action_search_cover_on_internet) },
	{ "RemoveCover", GTK_STOCK_REMOVE,
	  N_("_Remove Cover"), NULL,
	  N_("Remove current CD cover"),
	  G_CALLBACK (activate_action_remove_cover) },
	{ "ToggleVisibility", NULL,
	  N_("_Hide Window"), NULL /*"H"*/,
	  N_("Hide the main window"),
	  G_CALLBACK (activate_action_toggle_visibility) },
	{ "Reload", GTK_STOCK_REFRESH,
	  NULL, "<Ctrl>R",
	  NULL,
	  G_CALLBACK (activate_action_reload) }
};
static guint n_action_entries = G_N_ELEMENTS (action_entries);


static GtkToggleActionEntry action_toggle_entries[] = {
	{ "ViewToolbar", NULL,
	  N_("_Toolbar"), NULL,
	  N_("View the main toolbar"),
	  G_CALLBACK (activate_action_view_toolbar), 
	  TRUE },
	{ "ViewStatusbar", NULL,
	  N_("Stat_usbar"), NULL,
	  N_("View the statusbar"),
	  G_CALLBACK (activate_action_view_statusbar), 
	  TRUE },
	{ "PlayAll", NULL,
	  N_("Play _All"), NULL,
	  N_("Play all tracks"),
	  G_CALLBACK (activate_action_play_all),
	  TRUE },
	{ "Repeat", NULL,
	  N_("_Repeat"), NULL,
	  N_("Restart playing when finished"),
	  G_CALLBACK (activate_action_repeat),
	  FALSE },
	{ "Shuffle", NULL,
	  N_("S_huffle"), NULL,
	  N_("Play tracks in a random order"),
	  G_CALLBACK (activate_action_shuffle),
	  FALSE },
};
static guint n_action_toggle_entries = G_N_ELEMENTS (action_toggle_entries);


static const gchar *ui_info = 
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu action='CDMenu'>"
"      <menuitem action='TogglePlay'/>"
"      <menuitem action='Stop'/>"
"      <menuitem action='Next'/>"
"      <menuitem action='Prev'/>"
"      <separator />"
"      <menuitem action='Eject'/>"
"      <separator />"
"      <menuitem action='Extract'/>"
"      <separator />"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='EditMenu'>"
"      <menuitem action='PlayAll'/>"
"      <menuitem action='Repeat'/>"
"      <menuitem action='Shuffle'/>"
"      <separator name='sep01'/>"
"      <menuitem action='Properties'/>"
"      <menu action='CDCoverMenu'>"
"        <menuitem action='PickCoverFromDisk'/>"
"        <menuitem action='SearchCoverFromWeb'/>"
"        <separator name='sep01'/>"
"        <menuitem action='RemoveCover'/>"
"      </menu>"
"      <separator name='sep02'/>"
"      <menuitem action='Preferences'/>"
"    </menu>"
"    <menu action='ViewMenu'>"
"      <menuitem action='ViewToolbar'/>"
"      <menuitem action='ViewStatusbar'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='Contents'/>"
"      <menuitem action='Shortcuts'/>"
"      <separator name='sep01'/>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"  <toolbar  name='ToolBar'>"
"    <toolitem action='Play'/>"
"    <toolitem action='Pause'/>"
"    <toolitem action='Next'/>"
"    <separator />"
"    <toolitem action='EjectToolBar'/>"
"  </toolbar>"
"  <popup name='ListPopupMenu'>"
"    <menuitem action='PlaySelected'/>"
"    <menuitem action='Extract'/>"
"  </popup>"
"  <popup name='TrayPopupMenu'>"
"    <menuitem action='TogglePlay'/>"
"    <menuitem action='Stop'/>"
"    <menuitem action='Next'/>"
"    <menuitem action='Eject'/>"
"    <separator/>"
"    <menuitem action='ToggleVisibility'/>"
"    <separator/>"
"    <menuitem action='Quit'/>"
"  </popup>"
"  <popup name='CoverPopupMenu'>"
"    <menuitem action='PickCoverFromDisk'/>"
"    <menuitem action='SearchCoverFromWeb'/>"
"    <separator name='sep01'/>"
"    <menuitem action='RemoveCover'/>"
"  </popup>"
"</ui>";


#endif /* UI_H */
