/*
 * restore.h — declares functions to relaunch apps and reposition windows
 * talks to: restore.c, main.c, gui.c
 * uses session.h to load WindowList, uses fork/execvp to relaunch processes
 * functions: restore_session(), reposition_window()
 */

#ifndef RESTORE_H
#define RESTORE_H

#include "capture.h"

int restore_session(const char *profile_name);
int reposition_window(Display *display, const char *title, int x, int y, int w, int h);

#endif