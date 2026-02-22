/*
 * restore.c — reads session JSON and relaunches each app, then repositions its window
 * talks to: session.c (load_session), capture.h (WindowInfo), restore.h
 * imports: unistd.h (fork/execvp), X11 for window repositioning via wmctrl
 * functions: restore_session(), reposition_window(), wait_for_window()
 */

#include "../include/restore.h"
#include "../include/session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

static Window find_window_by_title(Display *display, const char *title) {
    Window root = DefaultRootWindow(display);
    Atom net_client_list = XInternAtom(display, "_NET_CLIENT_LIST", True);
    if (net_client_list == None) return None;

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;

    if (XGetWindowProperty(display, root, net_client_list,
        0, (~0L), False, XA_WINDOW,
        &actual_type, &actual_format, &nitems, &bytes_after, &data) != Success) {
        return None;
    }

    Window *windows = (Window *)data;
    Window found = None;

    for (unsigned long i = 0; i < nitems; i++) {
        char *name = NULL;
        XFetchName(display, windows[i], &name);
        if (name && strstr(name, title)) {
            found = windows[i];
            XFree(name);
            break;
        }
        if (name) XFree(name);
    }

    XFree(data);
    return found;
}

static Window wait_for_window(Display *display, const char *title, int timeout_sec) {
    for (int i = 0; i < timeout_sec * 2; i++) {
        usleep(500000);
        XSync(display, False);
        Window w = find_window_by_title(display, title);
        if (w != None) return w;
    }
    return None;
}

int reposition_window(Display *display, const char *title, int x, int y, int w, int h) {
    Window win = find_window_by_title(display, title);
    if (win == None) return -1;

    XMoveResizeWindow(display, win, x, y, (unsigned int)w, (unsigned int)h);
    XFlush(display);
    return 0;
}

static void launch_app(const WindowInfo *info) {
    if (info->cmd_argc == 0) return;

    pid_t pid = fork();
    if (pid == 0) {
        char *args[MAX_CMD_ARGS + 1];
        for (int i = 0; i < info->cmd_argc; i++) {
            args[i] = (char *)info->cmd[i];
        }
        args[info->cmd_argc] = NULL;

        setsid();
        execvp(args[0], args);
        fprintf(stderr, "sessionsnap: failed to exec %s\n", args[0]);
        exit(1);
    }
}

int restore_session(const char *profile_name) {
    WindowList *list = load_session(profile_name);
    if (!list) return -1;

    if (list->count == 0) {
        printf("sessionsnap: no windows in saved session\n");
        free_window_list(list);
        return 0;
    }

    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "sessionsnap: cannot open display for restore\n");
        free_window_list(list);
        return -1;
    }

    printf("sessionsnap: restoring %d windows...\n", list->count);

    for (int i = 0; i < list->count; i++) {
        const WindowInfo *w = &list->windows[i];
        printf("  launching: %s\n", w->cmd[0]);
        launch_app(w);
        usleep(300000);
    }

    printf("sessionsnap: waiting for windows to open...\n");
    sleep(2);

    for (int i = 0; i < list->count; i++) {
        const WindowInfo *w = &list->windows[i];
        if (strlen(w->title) == 0) continue;

        printf("  repositioning: %s\n", w->title);

        char short_title[64];
        strncpy(short_title, w->title, sizeof(short_title) - 1);
        short_title[sizeof(short_title) - 1] = '\0';

        Window win = wait_for_window(display, short_title, 5);
        if (win == None) {
            printf("  warning: could not find window for '%s'\n", short_title);
            continue;
        }

        if (w->is_maximized) {
            Atom net_wm_state = XInternAtom(display, "_NET_WM_STATE", False);
            Atom max_vert = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
            Atom max_horz = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);

            XEvent ev = {0};
            ev.type = ClientMessage;
            ev.xclient.window = win;
            ev.xclient.message_type = net_wm_state;
            ev.xclient.format = 32;
            ev.xclient.data.l[0] = 1;
            ev.xclient.data.l[1] = (long)max_vert;
            ev.xclient.data.l[2] = (long)max_horz;

            XSendEvent(display, DefaultRootWindow(display), False,
                SubstructureNotifyMask | SubstructureRedirectMask, &ev);
        } else {
            XMoveResizeWindow(display, win, w->x, w->y,
                (unsigned int)w->width, (unsigned int)w->height);
        }

        XFlush(display);
    }

    XCloseDisplay(display);
    free_window_list(list);

    printf("sessionsnap: restore complete\n");
    return 0;
}