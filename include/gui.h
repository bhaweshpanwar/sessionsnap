/*
 * gui.h — declares GTK popup dialog and system tray icon functions
 * talks to: gui.c, main.c
 * uses GTK3 for the dialog, calls restore.h when user clicks "Restore"
 * functions: show_restore_dialog(), init_tray_icon(), run_gui()
 */

#ifndef GUI_H
#define GUI_H

int show_restore_dialog(const char *profile_name);
void run_gui(void);

#endif