/*
 * capture.h — defines the WindowInfo struct and declares capture functions
 * talks to: capture.c, session.c, monitor.c
 * uses X11 (Xlib) to query window properties from the display server
 * functions: capture_windows(), free_window_list()
 */

#ifndef CAPTURE_H
#define CAPTURE_H

#include <X11/Xlib.h>

#define MAX_WINDOWS 256
#define MAX_CMD_ARGS 64
#define MAX_ARG_LEN 512

typedef struct {
    unsigned long window_id;
    int pid;
    int x, y;
    int width, height;
    int desktop;
    int is_maximized;
    int is_minimized;
    char title[256];
    char cmd[MAX_CMD_ARGS][MAX_ARG_LEN];
    int cmd_argc;
    char exe_path[512];
} WindowInfo;

typedef struct {
    WindowInfo windows[MAX_WINDOWS];
    int count;
} WindowList;

WindowList *capture_windows(Display *display);
void free_window_list(WindowList *list);

#endif