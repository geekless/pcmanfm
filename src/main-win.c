/*
 *      main-win.c
 *
 *      Copyright 2009 - 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#include <unistd.h> /* for get euid */
#include <sys/types.h>
#include <ctype.h>

#include "pcmanfm.h"

#include "app-config.h"
#include "main-win.h"
#include "pref.h"
#include "tab-page.h"

static void fm_main_win_finalize              (GObject *object);
G_DEFINE_TYPE(FmMainWin, fm_main_win, GTK_TYPE_WINDOW);

static void update_statusbar(FmMainWin* win);

static void on_focus_in(GtkWidget* w, GdkEventFocus* evt);
static gboolean on_key_press_event(GtkWidget* w, GdkEventKey* evt);
static gboolean on_button_press_event(GtkWidget* w, GdkEventButton* evt);
static gboolean on_delete_event(GtkWidget* w, GdkEvent* evt);

static void on_new_win(GtkAction* act, FmMainWin* win);
static void on_new_tab(GtkAction* act, FmMainWin* win);
static void on_close_tab(GtkAction* act, FmMainWin* win);
static void on_close_win(GtkAction* act, FmMainWin* win);

static void on_open_in_new_tab(GtkAction* act, FmMainWin* win);
static void on_open_in_new_win(GtkAction* act, FmMainWin* win);

static void on_cut(GtkAction* act, FmMainWin* win);
static void on_copy(GtkAction* act, FmMainWin* win);
static void on_copy_to(GtkAction* act, FmMainWin* win);
static void on_move_to(GtkAction* act, FmMainWin* win);
static void on_paste(GtkAction* act, FmMainWin* win);
static void on_del(GtkAction* act, FmMainWin* win);
static void on_rename(GtkAction* act, FmMainWin* win);

static void on_select_all(GtkAction* act, FmMainWin* win);
static void on_invert_select(GtkAction* act, FmMainWin* win);
static void on_preference(GtkAction* act, FmMainWin* win);

static void on_add_bookmark(GtkAction* act, FmMainWin* win);

static void on_go(GtkAction* act, FmMainWin* win);
static void on_go_back(GtkAction* act, FmMainWin* win);
static void on_go_forward(GtkAction* act, FmMainWin* win);
static void on_go_up(GtkAction* act, FmMainWin* win);
static void on_go_home(GtkAction* act, FmMainWin* win);
static void on_go_desktop(GtkAction* act, FmMainWin* win);
static void on_go_trash(GtkAction* act, FmMainWin* win);
static void on_go_computer(GtkAction* act, FmMainWin* win);
static void on_go_network(GtkAction* act, FmMainWin* win);
static void on_go_apps(GtkAction* act, FmMainWin* win);
static void on_reload(GtkAction* act, FmMainWin* win);
static void on_show_hidden(GtkToggleAction* act, FmMainWin* win);
static void on_show_side_pane(GtkToggleAction* act, FmMainWin* win);
static void on_change_mode(GtkRadioAction* act, GtkRadioAction *cur, FmMainWin* win);
static void on_change_hint(GtkRadioAction* act, GtkRadioAction *cur, FmMainWin* win);
static void on_sort_by(GtkRadioAction* act, GtkRadioAction *cur, FmMainWin* win);
static void on_sort_type(GtkRadioAction* act, GtkRadioAction *cur, FmMainWin* win);
static void on_side_pane_mode(GtkRadioAction* act, GtkRadioAction *cur, FmMainWin* win);
static void on_about(GtkAction* act, FmMainWin* win);
static void on_open_folder_in_terminal(GtkAction* act, FmMainWin* win);
static void on_open_in_terminal(GtkAction* act, FmMainWin* win);
static void on_open_as_root(GtkAction* act, FmMainWin* win);

static void on_location(GtkAction* act, FmMainWin* win);

static void on_create_new(GtkAction* action, FmMainWin* win);
static void on_prop(GtkAction* action, FmMainWin* win);

static void on_notebook_switch_page(GtkNotebook* nb, GtkNotebookPage* page, guint num, FmMainWin* win);
static void on_notebook_page_added(GtkNotebook* nb, GtkWidget* page, guint num, FmMainWin* win);
static void on_notebook_page_removed(GtkNotebook* nb, GtkWidget* page, guint num, FmMainWin* win);

static void on_folder_view_clicked(FmFolderView* fv, FmFolderViewClickType type, FmFileInfo* fi, FmMainWin* win);

static void zoom(FmMainWin* win, int delta);

static void on_zoom_in(GtkAction* act, FmMainWin* win);
static void on_zoom_out(GtkAction* act, FmMainWin* win);

#include "main-win-ui.c" /* ui xml definitions and actions */

static GSList* all_wins = NULL;

static void fm_main_win_class_init(FmMainWinClass *klass)
{
    GObjectClass *g_object_class;
    GtkWidgetClass* widget_class;
    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->finalize = fm_main_win_finalize;

    widget_class = (GtkWidgetClass*)klass;
    widget_class->focus_in_event = on_focus_in;
    widget_class->delete_event = on_delete_event;
    widget_class->key_press_event = on_key_press_event;
    widget_class->button_press_event = on_button_press_event;

    fm_main_win_parent_class = (GtkWindowClass*)g_type_class_peek(GTK_TYPE_WINDOW);
}

static gboolean idle_focus_view(FmMainWin* win)
{
    if(win->folder_view)
        gtk_widget_grab_focus(win->folder_view);
    return FALSE;
}

static void on_location_activate(GtkEntry* entry, FmMainWin* win)
{
    FmPath* path = fm_path_entry_get_path(FM_PATH_ENTRY(entry));
    fm_main_win_chdir(win, path);

    /* FIXME: due to bug #650114 in GTK+, GtkEntry still call a
     * idle function for GtkEntryCompletion even if the completion
     * is set to NULL. This causes crash in pcmanfm since libfm
     * set GtkCompletition to NULL when FmPathEntry loses its
     * focus. Hence the bug is triggered when we set focus to
     * the folder view, which in turns causes FmPathEntry to lose focus.
     *
     * See related bug reports for more info:
     * https://bugzilla.gnome.org/show_bug.cgi?id=650114
     * https://sourceforge.net/tracker/?func=detail&aid=3308324&group_id=156956&atid=801864
     *
     * So here is a quick fix:
     * Set the focus to folder view in our own idle handler with lower
     * priority than GtkEntry's idle function (They use G_PRIORITY_HIGH).
     */
    g_idle_add_full(G_PRIORITY_LOW, idle_focus_view, win, NULL);
}

static gboolean open_folder_func(GAppLaunchContext* ctx, GList* folder_infos, gpointer user_data, GError** err)
{
    FmMainWin* win = FM_MAIN_WIN(user_data);
    GList* l = folder_infos;
    FmFileInfo* fi = (FmFileInfo*)l->data;
    fm_main_win_chdir(win, fi->path);
    l=l->next;
    for(; l; l=l->next)
    {
        FmFileInfo* fi = (FmFileInfo*)l->data;
        fm_main_win_add_tab(win, fi->path);
    }
    return TRUE;
}

static FmJobErrorAction on_query_target_info_error(FmJob* job, GError* err, FmJobErrorSeverity severity, FmMainWin* win)
{
    if(err->domain == G_IO_ERROR)
    {
        if(err->code == G_IO_ERROR_NOT_MOUNTED)
        {
            if(fm_mount_path(GTK_WINDOW(win), fm_file_info_job_get_current(FM_FILE_INFO_JOB(job)), TRUE))
                return FM_JOB_RETRY;
        }
        else if(err->code == G_IO_ERROR_FAILED_HANDLED)
            return FM_JOB_CONTINUE;
    }
    fm_show_error(GTK_WINDOW(win), NULL, err->message);
    return FM_JOB_CONTINUE;
}

static void update_nav_actions(FmMainWin* win)
{
    gboolean can_prev = fm_nav_history_get_can_back(win->nav_history);
    gboolean can_next = fm_nav_history_get_can_forward(win->nav_history);

    GtkAction* act;

    act = gtk_ui_manager_get_action(win->ui, "/menubar/GoMenu/Prev");
    gtk_action_set_sensitive(act, can_prev);

    act = gtk_ui_manager_get_action(win->ui, "/Prev2");
    gtk_action_set_sensitive(act, can_prev);

    /* FIXME: This blocks NavHistory popup menu. */
/*
    act = gtk_ui_manager_get_action(win->ui, "/menubar/GoMenu/Next");
    gtk_action_set_sensitive(act, can_next);

    act = gtk_ui_manager_get_action(win->ui, "/Next2");
    gtk_action_set_sensitive(act, can_next);
*/

    FmPath* cwd = fm_folder_view_get_cwd(FM_FOLDER_VIEW(win->folder_view));
    FmPath* parent = fm_path_get_parent(cwd);

    act = gtk_ui_manager_get_action(win->ui, "/menubar/GoMenu/Up");
    gtk_action_set_sensitive(act, parent != NULL);
}

static void update_selection_actions(FmMainWin* win, FmFileInfoList* files)
{
    unsigned items_num = 0;

    if (files)
        items_num = fm_list_get_length(files) ;

    GtkAction* act;

    act = gtk_ui_manager_get_action(win->ui, "/menubar/EditMenu/Cut");
    gtk_action_set_sensitive(act, items_num > 0);

    act = gtk_ui_manager_get_action(win->ui, "/menubar/EditMenu/Copy");
    gtk_action_set_sensitive(act, items_num > 0);

    act = gtk_ui_manager_get_action(win->ui, "/menubar/EditMenu/Del");
    gtk_action_set_sensitive(act, items_num > 0);

    act = gtk_ui_manager_get_action(win->ui, "/menubar/EditMenu/Rename");
    gtk_action_set_sensitive(act, items_num == 1);

    act = gtk_ui_manager_get_action(win->ui, "/menubar/EditMenu/MoveTo");
    gtk_action_set_sensitive(act, items_num > 0);

    act = gtk_ui_manager_get_action(win->ui, "/menubar/EditMenu/CopyTo");
    gtk_action_set_sensitive(act, items_num > 0);
}

static void update_sort_actions(FmMainWin* win)
{
    GtkAction* act;
    act = gtk_ui_manager_get_action(win->ui, "/menubar/ViewMenu/Sort/Asc");
    gtk_radio_action_set_current_value(GTK_RADIO_ACTION(act), FM_FOLDER_VIEW(win->folder_view)->sort_type);
    act = gtk_ui_manager_get_action(win->ui, "/menubar/ViewMenu/Sort/ByName");
    gtk_radio_action_set_current_value(GTK_RADIO_ACTION(act), FM_FOLDER_VIEW(win->folder_view)->sort_by);
}

static void update_view_actions(FmMainWin* win)
{
    GtkAction* act;
    act = gtk_ui_manager_get_action(win->ui, "/menubar/ViewMenu/ShowHidden");
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), FM_FOLDER_VIEW(win->folder_view)->show_hidden);
    act = gtk_ui_manager_get_action(win->ui, "/menubar/ViewMenu/IconView");
    gtk_radio_action_set_current_value(GTK_RADIO_ACTION(act), FM_FOLDER_VIEW(win->folder_view)->mode);
    act = gtk_ui_manager_get_action(win->ui, "/menubar/ViewMenu/Hint/HintNone");
    gtk_radio_action_set_current_value(GTK_RADIO_ACTION(act), FM_FOLDER_VIEW(win->folder_view)->hint);
}

static void on_folder_view_sort_changed(FmFolderView* fv, FmMainWin* win)
{
    update_sort_actions(win);
}

static void open_context_menu(FmFolderView* fv, FmMainWin* win)
{
    FmFileInfoList *files = fm_folder_view_get_selected_files(fv);
    FmFileInfo *info;
    if(files && !fm_list_is_empty(files))
        info = fm_list_peek_head(files);
    else
        info = NULL;
    on_folder_view_clicked(fv, FM_FV_CONTEXT_MENU, info, win);
    if(files)
        fm_list_unref(files);
}

static gboolean on_view_key_press_event(FmFolderView* fv, GdkEventKey* evt, FmMainWin* win)
{
    int modifier = ( evt->state & ( GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK ) );

    switch(evt->keyval)
    {
    case GDK_BackSpace:
        on_go_up(NULL, win);
        break;
    case GDK_Delete:
        on_del(NULL, win);
        break;
    case GDK_KP_Add:
        if (modifier & GDK_CONTROL_MASK)
        {
            //on_zoom_in(NULL, win);
            zoom(win, (modifier & GDK_SHIFT_MASK) ? 4 : 8);
        }
        break;
    case GDK_KP_Subtract:
        if (modifier & GDK_CONTROL_MASK)
        {
            //on_zoom_out(NULL, win);
            zoom(win, (modifier & GDK_SHIFT_MASK) ? -4 : -8);
        }
        break;
    case GDK_Menu:
        open_context_menu(fv, win);
        break;
    case GDK_F10:
        if(modifier & GDK_SHIFT_MASK)
        {
            open_context_menu(fv, win);
        }
        break;
    }
    return FALSE;
}

static void on_bookmark(GtkMenuItem* mi, FmMainWin* win)
{
    FmPath* path = (FmPath*)g_object_get_data(G_OBJECT(mi), "path");
    switch(FM_APP_CONFIG(fm_config)->bm_open_method)
    {
    case FM_OPEN_IN_CURRENT_TAB: /* current tab */
        fm_main_win_chdir(win, path);
        break;
    case FM_OPEN_IN_NEW_TAB: /* new tab */
        fm_main_win_add_tab(win, path);
        break;
    case FM_OPEN_IN_NEW_WINDOW: /* new window */
        fm_main_win_add_win(win, path);
        break;
    }
}

static void create_bookmarks_menu(FmMainWin* win)
{
    GList* l;
    GtkWidget* mi;
    int i = 0;

    for(l=win->bookmarks->items;l;l=l->next)
    {
        FmBookmarkItem* item = (FmBookmarkItem*)l->data;
        mi = gtk_image_menu_item_new_with_label(item->name);
        gtk_widget_show(mi);
        g_object_set_data_full(G_OBJECT(mi), "path", fm_path_ref(item->path), (GDestroyNotify)fm_path_unref);
        g_signal_connect(mi, "activate", G_CALLBACK(on_bookmark), win);
        gtk_menu_shell_insert(GTK_MENU_SHELL(win->bookmarks_menu), mi, i);
        ++i;
    }
    if(i > 0)
    {
        mi = gtk_separator_menu_item_new();
        gtk_widget_show(mi);
        gtk_menu_shell_insert(GTK_MENU_SHELL(win->bookmarks_menu), mi, i);
    }
}

static void on_bookmarks_changed(FmBookmarks* bm, FmMainWin* win)
{
    /* delete old items first. */
    GList* mis = gtk_container_get_children(GTK_CONTAINER(win->bookmarks_menu));
    GList* l;
    for(l = mis;l;l=l->next)
    {
        GtkWidget* item = (GtkWidget*)l->data;
        if( g_object_get_data(G_OBJECT(item), "path") )
            gtk_widget_destroy(item);
        else
        {
            if(GTK_IS_SEPARATOR_MENU_ITEM(item))
                gtk_widget_destroy(item);
            break;
        }
    }
    g_list_free(mis);

    create_bookmarks_menu(win);
}

static void load_bookmarks(FmMainWin* win, GtkUIManager* ui)
{
    GtkWidget* mi = gtk_ui_manager_get_widget(ui, "/menubar/BookmarksMenu");
    win->bookmarks_menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(mi));
    win->bookmarks = fm_bookmarks_get();
    g_signal_connect(win->bookmarks, "changed", G_CALLBACK(on_bookmarks_changed), win);

    create_bookmarks_menu(win);
}

static void on_history_item(GtkMenuItem* mi, FmMainWin* win)
{
    FmTabPage* page = FM_TAB_PAGE(win->current_page);
    GList* l = g_object_get_data(G_OBJECT(mi), "path");
    fm_tab_page_history(page, l);
}

static void on_show_history_menu(GtkMenuToolButton* btn, FmMainWin* win)
{
    GtkMenuShell* menu = (GtkMenuShell*)gtk_menu_tool_button_get_menu(btn);
    GList* l;
    GList* cur = fm_nav_history_get_cur_link(win->nav_history);

    /* delete old items */
    gtk_container_foreach(GTK_CONTAINER(menu), (GtkCallback)gtk_widget_destroy, NULL);

    for(l = fm_nav_history_list(win->nav_history); l; l=l->next)
    {
        const FmNavHistoryItem* item = (FmNavHistoryItem*)l->data;
        FmPath* path = item->path;
        char* str = fm_path_display_name(path, TRUE);
        GtkMenuItem* mi;
        if( l == cur )
        {
            mi = gtk_check_menu_item_new_with_label(str);
            gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(mi), TRUE);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi), TRUE);
        }
        else
            mi = gtk_menu_item_new_with_label(str);
        g_free(str);

        g_object_set_data_full(G_OBJECT(mi), "path", l, NULL);
        g_signal_connect(mi, "activate", G_CALLBACK(on_history_item), win);
        gtk_menu_shell_append(menu, mi);
    }
    gtk_widget_show_all( GTK_WIDGET(menu) );
}

static void on_tab_page_splitter_pos_changed(GtkPaned* paned, GParamSpec* ps, FmMainWin* win)
{
    GList *children, *child;
    app_config->splitter_pos = gtk_paned_get_position(paned);
    pcmanfm_save_config(FALSE);

    /* apply the pos to all other pages as well */
    /* TODO: maybe we should allow different splitter pos for different pages later? */
    children = gtk_container_get_children(GTK_CONTAINER(win->notebook));
    for(child = children; child; child = child->next)
    {
        FmTabPage* page = FM_TAB_PAGE(child->data);
        if(page != paned)
            gtk_paned_set_position(GTK_PANED(page), app_config->splitter_pos);
    }
    g_list_free(children);
}

/* This callback is only connected to side pane of current active tab page. */
static void on_side_pane_chdir(FmSidePane* sp, guint button, FmPath* path, FmMainWin* win)
{
    if(button == 2) /* middle click */
        fm_main_win_add_tab(win, path);
    else
        fm_main_win_chdir(win, path);
}

/* This callback is only connected to side pane of current active tab page. */
static void on_side_pane_mode_changed(FmSidePane* sp, FmMainWin* win)
{
    GList* children = gtk_container_get_children(GTK_CONTAINER(win->notebook));
    GList* child;
    FmSidePaneMode mode = fm_side_pane_get_mode(sp);
    /* set the side pane mode to all other tab pages */
    for(child = children; child; child = child->next)
    {
        FmTabPage* page = FM_TAB_PAGE(child->data);
        if(page != win->current_page)
            fm_side_pane_set_mode(FM_SIDE_PANE(fm_tab_page_get_side_pane(page)), mode);
    }
    g_list_free(children);

    /* update menu */
    gtk_radio_action_set_current_value(gtk_ui_manager_get_action(win->ui,
                                       "/menubar/ViewMenu/SidePane/Places"),
                                       sp->mode);

    if(mode != app_config->side_pane_mode)
    {
        app_config->side_pane_mode = mode;
        fm_config_emit_changed(app_config, "side_pane_mode");
        pcmanfm_save_config(FALSE);
    }
}

static void fm_main_win_init(FmMainWin *win)
{
    GtkWidget *vbox, *menubar, *toolitem, *btn;
    GtkUIManager* ui;
    GtkActionGroup* act_grp;
    GtkAction* act;
    GtkAccelGroup* accel_grp;
    GtkShadowType shadow_type;

    pcmanfm_ref();
    all_wins = g_slist_prepend(all_wins, win);

    gtk_window_set_icon_name(GTK_WINDOW(win), "folder");

    vbox = gtk_vbox_new(FALSE, 0);

    /* create menu bar and toolbar */
    ui = gtk_ui_manager_new();
    act_grp = gtk_action_group_new("Main");
    gtk_action_group_set_translation_domain(act_grp, NULL);
    gtk_action_group_add_actions(act_grp, main_win_actions, G_N_ELEMENTS(main_win_actions), win);
    /* FIXME: this is so ugly */
    main_win_toggle_actions[0].is_active = app_config->show_hidden;
    gtk_action_group_add_toggle_actions(act_grp, main_win_toggle_actions, G_N_ELEMENTS(main_win_toggle_actions), win);
    gtk_action_group_add_radio_actions(act_grp, main_win_mode_actions, G_N_ELEMENTS(main_win_mode_actions), app_config->view_mode, on_change_mode, win);
    gtk_action_group_add_radio_actions(act_grp, main_win_hint_actions, G_N_ELEMENTS(main_win_hint_actions), app_config->hint_type, on_change_hint, win);
    gtk_action_group_add_radio_actions(act_grp, main_win_sort_type_actions, G_N_ELEMENTS(main_win_sort_type_actions), app_config->sort_type, on_sort_type, win);
    gtk_action_group_add_radio_actions(act_grp, main_win_sort_by_actions, G_N_ELEMENTS(main_win_sort_by_actions), app_config->sort_by, on_sort_by, win);
    gtk_action_group_add_radio_actions(act_grp, main_win_side_bar_mode_actions, G_N_ELEMENTS(main_win_side_bar_mode_actions), app_config->side_pane_mode, on_side_pane_mode, win);

    accel_grp = gtk_ui_manager_get_accel_group(ui);
    gtk_window_add_accel_group(GTK_WINDOW(win), accel_grp);

    gtk_ui_manager_insert_action_group(ui, act_grp, 0);
    gtk_ui_manager_add_ui_from_string(ui, main_menu_xml, -1, NULL);

    menubar = gtk_ui_manager_get_widget(ui, "/menubar");
    win->toolbar = gtk_ui_manager_get_widget(ui, "/toolbar");
    /* FIXME: should make these optional */
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(win->toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_toolbar_set_style(GTK_TOOLBAR(win->toolbar), GTK_TOOLBAR_ICONS);

    /* create 'Next' button manually and add a popup menu to it */
    toolitem = g_object_new(GTK_TYPE_MENU_TOOL_BUTTON, NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(win->toolbar), toolitem, 2);
    gtk_widget_show(GTK_WIDGET(toolitem));
    act = gtk_ui_manager_get_action(ui, "/menubar/GoMenu/Next");
    gtk_activatable_set_related_action(GTK_ACTIVATABLE(toolitem), act);

    /* set up history menu */
    win->history_menu = gtk_menu_new();
    gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(toolitem), win->history_menu);
    g_signal_connect(toolitem, "show-menu", G_CALLBACK(on_show_history_menu), win);

    win->popup = gtk_ui_manager_get_widget(ui, "/popup");
    gtk_menu_attach_to_widget(GTK_WIDGET(win->popup), win, NULL);

    gtk_box_pack_start( (GtkBox*)vbox, menubar, FALSE, TRUE, 0 );
    gtk_box_pack_start( (GtkBox*)vbox, win->toolbar, FALSE, TRUE, 0 );

    /* load bookmarks menu */
    load_bookmarks(win, ui);

    /* the location bar */
    win->location = fm_path_entry_new();
    g_signal_connect(win->location, "activate", on_location_activate, win);
    if(geteuid() == 0) /* if we're using root, Give the user some warnings */
    {
        GtkWidget* warning = gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_widget_set_tooltip_markup(warning, _("You are in super user mode"));

        toolitem = gtk_tool_item_new();
        gtk_container_add( GTK_CONTAINER(toolitem), warning );
        gtk_toolbar_insert(GTK_TOOLBAR(win->toolbar), gtk_separator_tool_item_new(), 0);

        gtk_toolbar_insert(GTK_TOOLBAR(win->toolbar), toolitem, 0);
    }

    toolitem = gtk_tool_item_new();
    gtk_container_add( GTK_CONTAINER(toolitem), win->location );
    gtk_tool_item_set_expand(GTK_TOOL_ITEM(toolitem), TRUE);
    gtk_toolbar_insert((GtkToolbar*)win->toolbar, toolitem, gtk_toolbar_get_n_items(GTK_TOOLBAR(win->toolbar)) - 1 );

    /* notebook */
    win->notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(win->notebook), TRUE);
    gtk_container_set_border_width(win->notebook, 0);
    gtk_notebook_set_show_border(win->notebook, FALSE);

    /* We need to use connect_after here.
     * GtkNotebook handles the real page switching stuff in default
     * handler of 'switch-page' signal. The current page is changed to the new one
     * after the signal is emitted. So before the default handler is finished,
     * current page is still the old one. */
    g_signal_connect_after(win->notebook, "switch-page", G_CALLBACK(on_notebook_switch_page), win);
    g_signal_connect(win->notebook, "page-added", G_CALLBACK(on_notebook_page_added), win);
    g_signal_connect(win->notebook, "page-removed", G_CALLBACK(on_notebook_page_removed), win);

    gtk_box_pack_start( (GtkBox*)vbox, win->notebook, TRUE, TRUE, 0 );

    /* status bar */
    win->statusbar = gtk_statusbar_new();
    /* status bar column showing volume free space */
    gtk_widget_style_get(win->statusbar, "shadow-type", &shadow_type, NULL);
    win->vol_status = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(win->vol_status), shadow_type);
    gtk_box_pack_start(GTK_BOX(win->statusbar), win->vol_status, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(win->vol_status), gtk_label_new(NULL));

    gtk_box_pack_start( (GtkBox*)vbox, win->statusbar, FALSE, TRUE, 0 );
    win->statusbar_ctx = gtk_statusbar_get_context_id(GTK_STATUSBAR(win->statusbar), "status");
    win->statusbar_ctx2 = gtk_statusbar_get_context_id(GTK_STATUSBAR(win->statusbar), "status2");

    g_object_unref(act_grp);
    win->ui = ui;

    gtk_container_add( (GtkContainer*)win, vbox );
    gtk_widget_show_all(vbox);
}


GtkWidget* fm_main_win_new(FmPath* path)
{
    FmMainWin* win = (FmMainWin*)g_object_new(FM_MAIN_WIN_TYPE, NULL);
    /* create new tab */
    fm_main_win_add_tab(win, path);
    return (GtkWidget*)win;
}


static void fm_main_win_finalize(GObject *object)
{
    FmMainWin *win;

    g_return_if_fail(object != NULL);
    g_return_if_fail(IS_FM_MAIN_WIN(object));

    win = FM_MAIN_WIN(object);

    g_object_unref(win->ui);
    g_object_unref(win->bookmarks);

    /* This is mainly for removing idle_focus_view() */
    g_source_remove_by_user_data(win);

    if (G_OBJECT_CLASS(fm_main_win_parent_class)->finalize)
        (* G_OBJECT_CLASS(fm_main_win_parent_class)->finalize)(object);

    pcmanfm_unref();
}

void on_about(GtkAction* act, FmMainWin* win)
{
    GtkWidget* dlg;
    GtkBuilder* builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, PACKAGE_UI_DIR "/about.ui", NULL);
    dlg = (GtkWidget*)gtk_builder_get_object(builder, "dlg");
    g_object_unref(builder);
    gtk_dialog_run((GtkDialog*)dlg);
    gtk_widget_destroy(dlg);
}

void on_open_folder_in_terminal(GtkAction* act, FmMainWin* win)
{
    FmFileInfoList* files = fm_folder_view_get_selected_files(FM_FOLDER_VIEW(win->folder_view));
    GList* l;
    for(l=fm_list_peek_head_link(files);l;l=l->next)
    {
        FmFileInfo* fi = (FmFileInfo*)l->data;
        if(fm_file_info_is_dir(fi) /*&& !fm_file_info_is_virtual(fi)*/)
            pcmanfm_open_folder_in_terminal(GTK_WINDOW(win), fi->path);
    }
    fm_list_unref(files);
}

void on_open_in_terminal(GtkAction* act, FmMainWin* win)
{
    const FmNavHistoryItem* item = fm_nav_history_get_cur(win->nav_history);
    if(item && item->path)
        pcmanfm_open_folder_in_terminal(GTK_WINDOW(win), item->path);
}

void on_open_as_root(GtkAction* act, FmMainWin* win)
{
    GAppInfo* app;
    char* cmd;
    if(!app_config->su_cmd)
    {
        fm_show_error(GTK_WINDOW(win), NULL, _("Switch user command is not set."));
        fm_edit_preference(GTK_WINDOW(win), PREF_ADVANCED);
        return;
    }
    if(strstr(app_config->su_cmd, "%s")) /* FIXME: need to rename to pcmanfm when we reach stable release. */
        cmd = g_strdup_printf(app_config->su_cmd, "pcmanfm %U");
    else
        cmd = g_strconcat(app_config->su_cmd, " ", "pcmanfm %U", NULL);
    app = g_app_info_create_from_commandline(cmd, NULL, 0, NULL);
    g_free(cmd);
    if(app)
    {
        FmPath* cwd = fm_folder_view_get_cwd(FM_FOLDER_VIEW(win->folder_view));
        GError* err = NULL;
        GAppLaunchContext* ctx = gdk_app_launch_context_new();
        char* uri = fm_path_to_uri(cwd);
        GList* uris = g_list_prepend(NULL, uri);
        gdk_app_launch_context_set_screen(GDK_APP_LAUNCH_CONTEXT(ctx), gtk_widget_get_screen(GTK_WIDGET(win)));
        gdk_app_launch_context_set_timestamp(GDK_APP_LAUNCH_CONTEXT(ctx), gtk_get_current_event_time());
        if(!g_app_info_launch_uris(app, uris, ctx, &err))
        {
            fm_show_error(GTK_WINDOW(win), NULL, err->message);
            g_error_free(err);
        }
        g_list_free(uris);
        g_free(uri);
        g_object_unref(ctx);
        g_object_unref(app);
    }
}

void on_show_hidden(GtkToggleAction* act, FmMainWin* win)
{
    FmTabPage* page = FM_TAB_PAGE(win->current_page);
    gboolean active = gtk_toggle_action_get_active(act);
    fm_tab_page_set_show_hidden(page, active);

    if(active != app_config->show_hidden)
    {
        app_config->show_hidden = active;
        pcmanfm_save_config(FALSE);
    }
}

void on_change_mode(GtkRadioAction* act, GtkRadioAction *cur, FmMainWin* win)
{
    int mode = gtk_radio_action_get_current_value(cur);
    fm_folder_view_set_mode( FM_FOLDER_VIEW(win->folder_view), mode );
}

void on_change_hint(GtkRadioAction* act, GtkRadioAction *cur, FmMainWin* win)
{
    int hint = gtk_radio_action_get_current_value(cur);
    fm_folder_view_set_hint_type( FM_FOLDER_VIEW(win->folder_view), hint );
    if(hint != app_config->hint_type)
    {
        app_config->hint_type = hint;
        pcmanfm_save_config(FALSE);
    }
}

void on_sort_by(GtkRadioAction* act, GtkRadioAction *cur, FmMainWin* win)
{
    int val = gtk_radio_action_get_current_value(cur);
    fm_folder_view_sort(FM_FOLDER_VIEW(win->folder_view), -1, val);
    if(val != app_config->sort_by)
    {
        app_config->sort_by = val;
        pcmanfm_save_config(FALSE);
    }
}

void on_sort_type(GtkRadioAction* act, GtkRadioAction *cur, FmMainWin* win)
{
    int val = gtk_radio_action_get_current_value(cur);
    fm_folder_view_sort(FM_FOLDER_VIEW(win->folder_view), val, -1);
    if(val != app_config->sort_type)
    {
        app_config->sort_type = val;
        pcmanfm_save_config(FALSE);
    }
}

void on_side_pane_mode(GtkRadioAction* act, GtkRadioAction *cur, FmMainWin* win)
{
    FmTabPage* cur_page = FM_TAB_PAGE(win->current_page);
    FmSidePane* sp = FM_SIDE_PANE(fm_tab_page_get_side_pane(cur_page));
    int val = gtk_radio_action_get_current_value(cur);
    fm_side_pane_set_mode(sp, val);
}

void on_focus_in(GtkWidget* w, GdkEventFocus* evt)
{
    if(all_wins->data != w)
    {
        all_wins = g_slist_remove(all_wins, w);
        all_wins = g_slist_prepend(all_wins, w);
    }
    ((GtkWidgetClass*)fm_main_win_parent_class)->focus_in_event(w, evt);
}

gboolean on_delete_event(GtkWidget* w, GdkEvent* evt)
{
    FmMainWin* win = (FmMainWin*)w;
    /* store the size of last used window in config. */
    gtk_window_get_size(GTK_WINDOW(w), &app_config->win_width, &app_config->win_height);
    all_wins = g_slist_remove(all_wins, win);
//    ((GtkWidgetClass*)fm_main_win_parent_class)->delete_event(w, evt);
    return FALSE;
}

void on_new_win(GtkAction* act, FmMainWin* win)
{
    FmPath* path = fm_folder_view_get_cwd(FM_FOLDER_VIEW(win->folder_view));
    fm_main_win_add_win(win, path);
}

void on_new_tab(GtkAction* act, FmMainWin* win)
{
    FmPath* path = fm_folder_view_get_cwd(FM_FOLDER_VIEW(win->folder_view));
    fm_main_win_add_tab(win, path);
}

void on_close_win(GtkAction* act, FmMainWin* win)
{
    gtk_widget_destroy(GTK_WIDGET(win));
}

void on_close_tab(GtkAction* act, FmMainWin* win)
{
    GtkNotebook* nb = (GtkNotebook*)win->notebook;
    /* remove current page */
    gtk_notebook_remove_page(nb, gtk_notebook_get_current_page(nb));
}

void on_open_in_new_tab(GtkAction* act, FmMainWin* win)
{
    FmPathList* sels = fm_folder_view_get_selected_file_paths(FM_FOLDER_VIEW(win->folder_view));
    GList* l;
    for( l = fm_list_peek_head_link(sels); l; l=l->next )
    {
        FmPath* path = (FmPath*)l->data;
        fm_main_win_add_tab(win, path);
    }
    fm_list_unref(sels);
}


void on_open_in_new_win(GtkAction* act, FmMainWin* win)
{
    FmPathList* sels = fm_folder_view_get_selected_file_paths(FM_FOLDER_VIEW(win->folder_view));
    GList* l;
    for( l = fm_list_peek_head_link(sels); l; l=l->next )
    {
        FmPath* path = (FmPath*)l->data;
        fm_main_win_add_win(win, path);
    }
    fm_list_unref(sels);
}


void on_go(GtkAction* act, FmMainWin* win)
{
    FmPath* path = fm_path_entry_get_path(FM_PATH_ENTRY(win->location));
    fm_main_win_chdir(win, path);
}

void on_go_back(GtkAction* act, FmMainWin* win)
{
    FmTabPage* page = FM_TAB_PAGE(win->current_page);
    fm_tab_page_back(page);
}

void on_go_forward(GtkAction* act, FmMainWin* win)
{
    FmTabPage* page = FM_TAB_PAGE(win->current_page);
    fm_tab_page_forward(page);
}

void on_go_up(GtkAction* act, FmMainWin* win)
{
    FmPath* cwd = fm_folder_view_get_cwd(FM_FOLDER_VIEW(win->folder_view));
    FmPath* parent = fm_path_get_parent(cwd);
    if(parent)
        fm_main_win_chdir( win, parent);
}

void on_go_home(GtkAction* act, FmMainWin* win)
{
    fm_main_win_chdir( win, fm_path_get_home());
}

void on_go_desktop(GtkAction* act, FmMainWin* win)
{
    fm_main_win_chdir(win, fm_path_get_desktop());
}

void on_go_trash(GtkAction* act, FmMainWin* win)
{
    fm_main_win_chdir(win, fm_path_get_trash());
}

void on_go_computer(GtkAction* act, FmMainWin* win)
{
    fm_main_win_chdir_by_name( win, "computer:///");
}

void on_go_network(GtkAction* act, FmMainWin* win)
{
    fm_main_win_chdir_by_name( win, "network:///");
}

void on_go_apps(GtkAction* act, FmMainWin* win)
{
    fm_main_win_chdir(win, fm_path_get_apps_menu());
}

void fm_main_win_chdir_by_name(FmMainWin* win, const char* path_str)
{
    FmPath* path = fm_path_new(path_str);
    fm_main_win_chdir(win, path);
    fm_path_unref(path);
}

void fm_main_win_chdir(FmMainWin* win, FmPath* path)
{
    FmTabPage* page = FM_TAB_PAGE(win->current_page);
    /* NOTE: fm_tab_page_chdir() calls fm_side_pane_chdir(), which can
     * trigger on_side_pane_chdir() callback. So we need to block it here. */
    g_signal_handlers_block_by_func(win->side_pane, on_side_pane_chdir, win);
    fm_tab_page_chdir(page, path);
    g_signal_handlers_unblock_by_func(win->side_pane, on_side_pane_chdir, win);
}

static void close_btn_style_set(GtkWidget *btn, GtkRcStyle *prev, gpointer data)
{
    gint w, h;
    gtk_icon_size_lookup_for_settings(gtk_widget_get_settings(btn), GTK_ICON_SIZE_MENU, &w, &h);
    gtk_widget_set_size_request(btn, w + 2, h + 2);
}

static gboolean on_tab_label_button_pressed(GtkWidget* tab_label, GdkEventButton* evt, FmTabPage* tab_page)
{
    if(evt->button == 2) /* middle click */
    {
        gtk_widget_destroy(GTK_WIDGET(tab_page));
        return TRUE;
    }
    return FALSE;
}

static void update_statusbar(FmMainWin* win)
{
    FmTabPage* page = FM_TAB_PAGE(win->current_page);
    const char* text;
    gtk_statusbar_pop(GTK_STATUSBAR(win->statusbar), win->statusbar_ctx);
    text = fm_tab_page_get_status_text(page, FM_STATUS_TEXT_NORMAL);
    if(text)
    {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), win->statusbar_ctx, text);
        gtk_widget_set_tooltip_text(GTK_WIDGET(win->statusbar), text);
    }

    text = fm_tab_page_get_status_text(page, FM_STATUS_TEXT_SELECTED_FILES);
    if(text)
    {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), win->statusbar_ctx2, text);
        gtk_widget_set_tooltip_text(GTK_WIDGET(win->statusbar), text);
    }

    text = fm_tab_page_get_status_text(page, FM_STATUS_TEXT_FS_INFO);
    if(text)
    {
        GtkLabel* label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(win->vol_status)));
        gtk_label_set_text(label, text);
        gtk_widget_set_tooltip_text(win->vol_status, text);
        gtk_widget_show(win->vol_status);
    }
    else
        gtk_widget_hide(win->vol_status);
}

gint fm_main_win_add_tab(FmMainWin* win, FmPath* path)
{
    FmTabPage* page = (FmTabPage*)fm_tab_page_new(path);
    FmTabLabel* label = FM_TAB_LABEL(page->tab_label);
    GtkWidget* folder_view = fm_tab_page_get_folder_view(page);
    gint ret;

    gtk_paned_set_position(GTK_PANED(page), app_config->splitter_pos);

    gtk_widget_show(page);
    g_signal_connect(folder_view, "key-press-event", G_CALLBACK(on_view_key_press_event), win);

    g_signal_connect_swapped(label->close_btn, "clicked", G_CALLBACK(gtk_widget_destroy), page);
    g_signal_connect(label, "button-press-event", G_CALLBACK(on_tab_label_button_pressed), page);

    /* add the tab */
    ret = gtk_notebook_append_page(GTK_NOTEBOOK(win->notebook), GTK_WIDGET(page), page->tab_label);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(win->notebook), GTK_WIDGET(page), TRUE);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(win->notebook), ret);

    return ret;
}

FmMainWin* fm_main_win_add_win(FmMainWin* win, FmPath* path)
{
    win = fm_main_win_new(path);
    gtk_window_set_default_size(GTK_WINDOW(win),
                                app_config->win_width,
                                app_config->win_height);
    gtk_window_present(GTK_WINDOW(win));
    return win;
}

void on_cut(GtkAction* act, FmMainWin* win)
{
    GtkWidget* focus = gtk_window_get_focus((GtkWindow*)win);
    if(GTK_IS_EDITABLE(focus) &&
       gtk_editable_get_selection_bounds((GtkEditable*)focus, NULL, NULL) )
    {
        gtk_editable_cut_clipboard((GtkEditable*)focus);
    }
    else
    {
        FmPathList* files = fm_folder_view_get_selected_file_paths(FM_FOLDER_VIEW(win->folder_view));
        if(files)
        {
            fm_clipboard_cut_files(win, files);
            fm_list_unref(files);
        }
    }
}

void on_copy(GtkAction* act, FmMainWin* win)
{
    GtkWidget* focus = gtk_window_get_focus((GtkWindow*)win);
    if(GTK_IS_EDITABLE(focus) &&
       gtk_editable_get_selection_bounds((GtkEditable*)focus, NULL, NULL) )
    {
        gtk_editable_copy_clipboard((GtkEditable*)focus);
    }
    else
    {
        FmPathList* files = fm_folder_view_get_selected_file_paths(FM_FOLDER_VIEW(win->folder_view));
        if(files)
        {
            fm_clipboard_copy_files(win, files);
            fm_list_unref(files);
        }
    }
}

void on_copy_to(GtkAction* act, FmMainWin* win)
{
    FmPathList* files = fm_folder_view_get_selected_file_paths(FM_FOLDER_VIEW(win->folder_view));
    if(files)
    {
        fm_copy_files_to(GTK_WINDOW(win), files);
        fm_list_unref(files);
    }
}

void on_move_to(GtkAction* act, FmMainWin* win)
{
    FmPathList* files = fm_folder_view_get_selected_file_paths(FM_FOLDER_VIEW(win->folder_view));
    if(files)
    {
        fm_move_files_to(GTK_WINDOW(win), files);
        fm_list_unref(files);
    }
}

void on_paste(GtkAction* act, FmMainWin* win)
{
    GtkWidget* focus = gtk_window_get_focus(GTK_WINDOW(win));
    if(GTK_IS_EDITABLE(focus) )
    {
        gtk_editable_paste_clipboard(GTK_EDITABLE(focus));
    }
    else
    {
        FmPath* path = fm_folder_view_get_cwd(FM_FOLDER_VIEW(win->folder_view));
        fm_clipboard_paste_files(win->folder_view, path);
    }
}

void on_del(GtkAction* act, FmMainWin* win)
{
    FmPathList* files = fm_folder_view_get_selected_file_paths(FM_FOLDER_VIEW(win->folder_view));
    if(files)
    {
        GdkModifierType state = 0;
        if(!gtk_get_current_event_state (&state))
            state = 0;
        if( state & GDK_SHIFT_MASK ) /* Shift + Delete = delete directly */
            fm_delete_files(GTK_WINDOW(win), files);
        else
            fm_trash_or_delete_files(GTK_WINDOW(win), files);
        fm_list_unref(files);
    }
}

void on_rename(GtkAction* act, FmMainWin* win)
{
    FmPathList* files = fm_folder_view_get_selected_file_paths(FM_FOLDER_VIEW(win->folder_view));
    if( !fm_list_is_empty(files) )
    {
        fm_rename_file(GTK_WINDOW(win), fm_list_peek_head(files));
        /* FIXME: is it ok to only rename the first selected file here. */
    }
    if(files)
        fm_list_unref(files);
}

void on_select_all(GtkAction* act, FmMainWin* win)
{
    fm_folder_view_select_all(FM_FOLDER_VIEW(win->folder_view));
}

void on_invert_select(GtkAction* act, FmMainWin* win)
{
    fm_folder_view_select_invert(FM_FOLDER_VIEW(win->folder_view));
}

void on_preference(GtkAction* act, FmMainWin* win)
{
    fm_edit_preference(GTK_WINDOW(win), 0);
}

void on_add_bookmark(GtkAction* act, FmMainWin* win)
{
    FmPath* cwd = fm_folder_view_get_cwd(FM_FOLDER_VIEW(win->folder_view));
    char* disp_path = fm_path_display_name(cwd, TRUE);
    char* msg = g_strdup_printf(_("Add following folder to bookmarks:\n\'%s\'\nEnter a name for the new bookmark item:"), disp_path);
    char* disp_name = fm_path_display_basename(cwd);
    char* name;
    g_free(disp_path);
    name = fm_get_user_input(GTK_WINDOW(win), _("Add to Bookmarks"), msg, disp_name);
    g_free(disp_name);
    if(name)
    {
        fm_bookmarks_append(win->bookmarks, cwd, name);
        g_free(name);
    }
}

void on_location(GtkAction* act, FmMainWin* win)
{
    gtk_widget_grab_focus(win->location);
}

void on_prop(GtkAction* action, FmMainWin* win)
{
    FmFolderView* fv = FM_FOLDER_VIEW(win->folder_view);
    /* FIXME: should prevent directly accessing data members */
    FmFileInfo* fi = FM_FOLDER_MODEL(fv->model)->dir->dir_fi;
    FmFileInfoList* files = fm_file_info_list_new();
    fm_list_push_tail(files, fi);
    fm_show_file_properties(GTK_WINDOW(win), files);
    fm_list_unref(files);
}

/* This callback is only connected to folder view of current active tab page. */
static void on_folder_view_clicked(FmFolderView* fv, FmFolderViewClickType type, FmFileInfo* fi, FmMainWin* win)
{
    switch(type)
    {
    case FM_FV_ACTIVATED: /* file activated */
        if(fm_file_info_is_dir(fi))
            fm_main_win_chdir( win, fi->path);
        else if(fm_file_info_get_target(fi) && !fm_file_info_is_symlink(fi))
        {
            /* symlinks also has fi->target, but we only handle shortcuts here. */
            FmFileInfo* target_fi;
            FmPath* real_path = fm_path_new(fm_file_info_get_target(fi));
            /* query the info of target */
            FmJob* job = fm_file_info_job_new(NULL, 0);
            fm_file_info_job_add(FM_FILE_INFO_JOB(job), real_path);
            g_signal_connect(job, "error", G_CALLBACK(on_query_target_info_error), win);
            fm_job_run_sync_with_mainloop(job);
            target_fi = FM_FILE_INFO(fm_list_peek_head(FM_FILE_INFO_JOB(job)->file_infos));
            if(target_fi)
                fm_file_info_ref(target_fi);
            g_object_unref(job);
            if(target_fi)
            {
                if(fm_file_info_is_dir(target_fi))
                    fm_main_win_chdir( win, real_path);
                else
                    fm_launch_path_simple(GTK_WINDOW(win), NULL, real_path, open_folder_func, win);
                fm_path_unref(FM_PATH(target_fi));
            }
            fm_path_unref(real_path);
        }
        else
            fm_launch_file_simple(GTK_WINDOW(win), NULL, fi, open_folder_func, win);
        break;
    case FM_FV_CONTEXT_MENU:
        if(fi)
        {
            FmFileMenu* menu;
            GtkMenu* popup;
            FmFileInfoList* files = fm_folder_view_get_selected_files(fv);
            menu = fm_file_menu_new_for_files(GTK_WINDOW(win), files, fm_folder_view_get_cwd(fv), TRUE);
            fm_file_menu_set_folder_func(menu, open_folder_func, win);
            fm_list_unref(files);

            /* merge some specific menu items for folders */
            if(fm_file_menu_is_single_file_type(menu) && fm_file_info_is_dir(fi))
            {
                GtkUIManager* ui = fm_file_menu_get_ui(menu);
                GtkActionGroup* act_grp = fm_file_menu_get_action_group(menu);
                gtk_action_group_set_translation_domain(act_grp, NULL);
                gtk_action_group_add_actions(act_grp, folder_menu_actions, G_N_ELEMENTS(folder_menu_actions), win);
                gtk_ui_manager_add_ui_from_string(ui, folder_menu_xml, -1, NULL);
            }

            popup = fm_file_menu_get_menu(menu);
            gtk_menu_popup(popup, NULL, NULL, NULL, fi, 3, gtk_get_current_event_time());
        }
        else /* no files are selected. Show context menu of current folder. */
            gtk_menu_popup(GTK_MENU(win->popup), NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
        break;
    case FM_FV_MIDDLE_CLICK:
        if(fm_file_info_is_dir(fi))
            fm_main_win_add_tab(win, fi->path);
        break;
    }
}

/* This callback is only connected to current active tab page. */
static void on_folder_view_sel_changed(FmFolderView* fv, FmFileInfoList* files, FmMainWin* win)
{
    update_selection_actions(win, files);
}

/* This callback is only connected to current active tab page. */
static void on_tab_page_status_text(FmTabPage* page, guint type, const char* status_text, FmMainWin* win)
{
    switch(type)
    {
    case FM_STATUS_TEXT_NORMAL:
        gtk_statusbar_pop(GTK_STATUSBAR(win->statusbar), win->statusbar_ctx);
        if(status_text)
            gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), win->statusbar_ctx, status_text);
        gtk_widget_set_tooltip_text(GTK_WIDGET(win->statusbar), status_text);
        break;
    case FM_STATUS_TEXT_SELECTED_FILES:
        gtk_statusbar_pop(GTK_STATUSBAR(win->statusbar), win->statusbar_ctx2);
        if(status_text)
            gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), win->statusbar_ctx2, status_text);
        gtk_widget_set_tooltip_text(GTK_WIDGET(win->statusbar), status_text);
        break;
    case FM_STATUS_TEXT_FS_INFO:
        if(status_text)
        {
            GtkLabel* label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(win->vol_status)));
            gtk_label_set_text(label, status_text);
            gtk_widget_show(win->vol_status);
        }
        else
            gtk_widget_hide(win->vol_status);
        gtk_widget_set_tooltip_text(win->vol_status, status_text);
        break;
    }
}

static void on_tab_page_chdir(FmTabPage* page, FmPath* path, FmMainWin* win)
{
    fm_path_entry_set_path(win->location, path);
    gtk_window_set_title(GTK_WINDOW(win), fm_tab_page_get_title(page));
    update_nav_actions(win);
}

static void on_notebook_switch_page(GtkNotebook* nb, GtkNotebookPage* new_page, guint num, FmMainWin* win)
{
    FmTabPage* page = FM_TAB_PAGE(new_page);
    FmFolderView* folder_view;
    FmSidePane* side_pane;
    FmPath* cwd;

    /* disconnect from previous active page */
    if(win->current_page)
    {
        g_signal_handlers_disconnect_by_func(win->current_page,
                                             on_tab_page_splitter_pos_changed, win);
        g_signal_handlers_disconnect_by_func(win->current_page,
                                             on_tab_page_chdir, win);
        g_signal_handlers_disconnect_by_func(win->current_page,
                                             on_tab_page_status_text, win);
        g_signal_handlers_disconnect_by_func(win->folder_view,
                                             on_folder_view_sort_changed, win);
        g_signal_handlers_disconnect_by_func(win->folder_view,
                                             on_folder_view_clicked, win);
        g_signal_handlers_disconnect_by_func(win->folder_view,
                                             on_folder_view_sel_changed, win);
        g_signal_handlers_disconnect_by_func(win->side_pane,
                                             on_side_pane_mode_changed, win);
        g_signal_handlers_disconnect_by_func(win->side_pane,
                                             on_side_pane_chdir, win);
    }

    /* connect to the new active page */
    win->current_page = new_page;
    folder_view = fm_tab_page_get_folder_view(page);
    win->folder_view = folder_view;
    win->nav_history = fm_tab_page_get_history(page);
    win->side_pane = fm_tab_page_get_side_pane(page);

    g_signal_connect(page, "notify::position",
                     G_CALLBACK(on_tab_page_splitter_pos_changed), win);
    g_signal_connect(page, "chdir",
                     G_CALLBACK(on_tab_page_chdir), win);
    g_signal_connect(page, "status",
                     G_CALLBACK(on_tab_page_status_text), win);
    g_signal_connect(folder_view, "sort-changed",
                     G_CALLBACK(on_folder_view_sort_changed), win);
    g_signal_connect(folder_view, "clicked",
                     G_CALLBACK(on_folder_view_clicked), win);
    g_signal_connect(folder_view, "sel-changed",
                     G_CALLBACK(on_folder_view_sel_changed), win);
    g_signal_connect(win->side_pane, "mode-changed",
                     G_CALLBACK(on_side_pane_mode_changed), win);
    g_signal_connect(win->side_pane, "chdir",
                     G_CALLBACK(on_side_pane_chdir), win);

    cwd = fm_folder_view_get_cwd(folder_view);
    fm_path_entry_set_path( FM_PATH_ENTRY(win->location), cwd);
    gtk_window_set_title((GtkWindow*)win, fm_tab_page_get_title(page));

    FmFileInfoList *files = fm_folder_view_get_selected_files(folder_view);

    update_sort_actions(win);
    update_view_actions(win);
    update_nav_actions(win);
    update_selection_actions(win, files);
    update_statusbar(win);

    if (files)
        fm_list_unref(files);

    /* FIXME: this does not work sometimes due to limitation of GtkNotebook.
     * So weird. After page switching with mouse button, GTK+ always tries
     * to focus the left pane, instead of the folder_view we specified. */
    gtk_widget_grab_focus(folder_view);
}

void on_notebook_page_added(GtkNotebook* nb, GtkWidget* page, guint num, FmMainWin* win)
{
    if(gtk_notebook_get_n_pages(nb) > 1
       || app_config->always_show_tabs)
        gtk_notebook_set_show_tabs(nb, TRUE);
    else
        gtk_notebook_set_show_tabs(nb, FALSE);
}

void on_notebook_page_removed(GtkNotebook* nb, GtkWidget* page, guint num, FmMainWin* win)
{
    if(gtk_notebook_get_n_pages(nb) > 1 || app_config->always_show_tabs)
        gtk_notebook_set_show_tabs(nb, TRUE);
    else
        gtk_notebook_set_show_tabs(nb, FALSE);

    /* all notebook pages are removed, let's destroy the main window */
    if(gtk_notebook_get_n_pages(nb) == 0)
        gtk_widget_destroy(GTK_WIDGET(win));
}

void on_create_new(GtkAction* action, FmMainWin* win)
{
    FmFolderView* fv = FM_FOLDER_VIEW(win->folder_view);
    const char* name = gtk_action_get_name(action);

    if( strcmp(name, "NewFolder") == 0 )
        name = TEMPL_NAME_FOLDER;
    else if( strcmp(name, "NewBlank") == 0 )
        name = TEMPL_NAME_BLANK;
    pcmanfm_create_new(GTK_WINDOW(win), fm_folder_view_get_cwd(fv), name);
}

FmMainWin* fm_main_win_get_last_active()
{
    return all_wins ? (FmMainWin*)all_wins->data : NULL;
}

void fm_main_win_open_in_last_active(FmPath* path)
{
    FmMainWin* win = fm_main_win_get_last_active();
    if(!win)
        fm_main_win_add_win(NULL, path);
    else
        fm_main_win_add_tab(win, path);
    gtk_window_present(GTK_WINDOW(win));
}

gboolean on_key_press_event(GtkWidget* w, GdkEventKey* evt)
{
    FmMainWin* win = FM_MAIN_WIN(w);

    if(evt->state == GDK_MOD1_MASK) /* Alt */
    {
        if(isdigit(evt->keyval)) /* Alt + 0 ~ 9, nth tab */
        {
            int n;
            if(evt->keyval == '0')
                n = 9;
            else
                n = evt->keyval - '1';
            gtk_notebook_set_current_page(GTK_NOTEBOOK(win->notebook), n);
            return TRUE;
        }
    }
    else if(evt->state == GDK_CONTROL_MASK) /* Ctrl */
    {
        if(evt->keyval == GDK_Tab || evt->keyval == GDK_ISO_Left_Tab) /* Ctrl + Tab, next tab */
        {
            int n = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->notebook));
            if(n < gtk_notebook_get_n_pages(GTK_NOTEBOOK(win->notebook)) - 1)
                ++n;
            else
                n = 0;
            gtk_notebook_set_current_page(GTK_NOTEBOOK(win->notebook), n);
            return TRUE;
        }
    }
    else if(evt->state == (GDK_CONTROL_MASK|GDK_SHIFT_MASK)) /* Ctrl + Shift */
    {
        if(evt->keyval == GDK_Tab || evt->keyval == GDK_ISO_Left_Tab) /* Ctrl + Shift + Tab, previous tab */
        {
            int n = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->notebook));
            if(n > 0)
                --n;
            else
                n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(win->notebook)) - 1;
            gtk_notebook_set_current_page(GTK_NOTEBOOK(win->notebook), n);
            return TRUE;
        }
    }
    else if(evt->keyval == '/' || evt->keyval == '~')
    {
        if (!gtk_widget_is_focus(win->location))
        {
            gtk_widget_grab_focus(win->location);
            char path[] = {evt->keyval, 0};
            gtk_entry_set_text(GTK_ENTRY(win->location), path);
            gtk_editable_set_position(GTK_EDITABLE(win->location), -1);
            return TRUE;
        }
    }
    else if(evt->keyval == GDK_Escape)
    {
        if (gtk_widget_is_focus(win->location))
        {
            gtk_widget_grab_focus(win->folder_view);
            fm_path_entry_set_path(FM_PATH_ENTRY(win->location),
                                   fm_folder_view_get_cwd(FM_FOLDER_VIEW(win->folder_view)));
            return TRUE;
        }
    }
    return GTK_WIDGET_CLASS(fm_main_win_parent_class)->key_press_event(w, evt);
}

gboolean on_button_press_event(GtkWidget* w, GdkEventButton* evt)
{
    FmMainWin* win = FM_MAIN_WIN(w);
    GtkAction* act;
    if(evt->button == 8) /* back */
    {
        act = gtk_ui_manager_get_action(win->ui, "/Prev2");
        gtk_action_activate(act);
    }
    else if(evt->button == 9) /* forward */
    {
        act = gtk_ui_manager_get_action(win->ui, "/Next2");
        gtk_action_activate(act);
    }

    if (GTK_WIDGET_CLASS(fm_main_win_parent_class)->button_press_event)
        return GTK_WIDGET_CLASS(fm_main_win_parent_class)->button_press_event(w, evt);
    else
        return TRUE;
}

static void on_reload(GtkAction* act, FmMainWin* win)
{
    FmTabPage* page = FM_TAB_PAGE(win->current_page);
    fm_tab_page_reload(page);
}

static void on_show_side_pane(GtkToggleAction* act, FmMainWin* win)
{
    /* TODO: hide the side pane if the user wants to. */
}


static void zoom(FmMainWin* win, int delta)
{
    guint * val;
    char * key_name;
    guint max_value = 96;

    FmFolderViewMode mode = fm_folder_view_get_mode(FM_FOLDER_VIEW(win->folder_view));
    switch (mode)
    {
        case FM_FV_ICON_VIEW:
            val = &fm_config->big_icon_size;
            key_name = "big_icon_size";
            max_value = 256;
            break;
        case FM_FV_LIST_VIEW:
        case FM_FV_COMPACT_VIEW:
        case FM_FV_VERTICAL_COMPACT_VIEW:
            val = &fm_config->small_icon_size;
            key_name = "small_icon_size";
            break;
        case FM_FV_THUMBNAIL_VIEW:
            val = &fm_config->thumbnail_size;
            key_name = "thumbnail_size";
            max_value = 256;
            break;
        default:
            return;
    }

    guint v = *val;

    if (v > 32 || (v == 32 && delta > 0))
        delta *= 2;
    if (v > 64 || (v == 64 && delta > 0))
        delta *= 2;

    v += delta;

    if (delta < 0)
        delta = - delta;
    v = v - (v % delta);

    if (v > max_value)
        v = max_value;
    if (v < 12)
        v = 12;

    *val = v;

    //g_print("icon size %d\n", v);

    fm_config_emit_changed(fm_config, key_name);
}


static void on_zoom_in(GtkAction* act, FmMainWin* win)
{
    zoom(win, 8);
}

static void on_zoom_out(GtkAction* act, FmMainWin* win)
{
    zoom(win, -8);
}

