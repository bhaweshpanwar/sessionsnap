/*
 * gui.c — shows a GTK dialog asking user to restore session on startup
 * talks to: restore.c (restore_session), session.c (session_file_exists), gui.h
 * imports: gtk/gtk.h for UI widgets, restore.h to trigger session restore
 * functions: show_restore_dialog(), run_gui()
 */

#include "../include/gui.h"
#include "../include/restore.h"
#include "../include/session.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

static void on_restore_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *dialog = (GtkWidget *)data;
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);
}

static void on_skip_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *dialog = (GtkWidget *)data;
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_NO);
}

int show_restore_dialog(const char *profile_name) {
    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "SessionSnap");
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 20);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    GtkWidget *icon_label = gtk_label_new("🖥");
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_scale_new(3.0));
    gtk_label_set_attributes(GTK_LABEL(icon_label), attrs);
    pango_attr_list_unref(attrs);
    gtk_box_pack_start(GTK_BOX(vbox), icon_label, FALSE, FALSE, 0);

    GtkWidget *title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_label),
        "<span size='large' weight='bold'>Restore Previous Session?</span>");
    gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 0);

    char msg[256];
    snprintf(msg, sizeof(msg),
        "SessionSnap found a saved workspace%s%s.\nWould you like to restore it?",
        profile_name && strcmp(profile_name, "default") != 0 ? ": " : "",
        profile_name && strcmp(profile_name, "default") != 0 ? profile_name : "");

    GtkWidget *sub_label = gtk_label_new(msg);
    gtk_label_set_justify(GTK_LABEL(sub_label), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), sub_label, FALSE, FALSE, 0);

    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(btn_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), btn_box, FALSE, FALSE, 8);

    GtkWidget *restore_btn = gtk_button_new_with_label("  Restore Session  ");
    GtkWidget *skip_btn = gtk_button_new_with_label("  Skip  ");

    GtkStyleContext *ctx = gtk_widget_get_style_context(restore_btn);
    gtk_style_context_add_class(ctx, "suggested-action");

    g_signal_connect(restore_btn, "clicked", G_CALLBACK(on_restore_clicked), dialog);
    g_signal_connect(skip_btn, "clicked", G_CALLBACK(on_skip_clicked), dialog);

    gtk_box_pack_start(GTK_BOX(btn_box), restore_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_box), skip_btn, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return (response == GTK_RESPONSE_YES) ? 1 : 0;
}

void run_gui(void) {
    gtk_init(NULL, NULL);

    if (!session_file_exists("default")) {
        printf("sessionsnap: no saved session found, skipping restore prompt\n");
        return;
    }

    int should_restore = show_restore_dialog("default");

    if (should_restore) {
        printf("sessionsnap: user chose to restore session\n");
        restore_session("default");
    } else {
        printf("sessionsnap: user skipped restore\n");
    }
}