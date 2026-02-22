/*
 * capture.c — scans all visible windows using X11 _NET_CLIENT_LIST property
 * talks to: capture.h, session.c (passes WindowList), monitor.c (called in loop)
 * imports: Xlib, Xatom, dirent (for /proc reading), psutil-equivalent via /proc
 * functions: capture_windows(), get_window_pid(), get_window_geometry(), get_process_cmd()
 */

#include "../include/capture.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

static int get_window_pid(Display *display, Window window) {
    Atom net_wm_pid = XInternAtom(display, "_NET_WM_PID", True);
    if (net_wm_pid == None) return -1;

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = NULL;

    int result = XGetWindowProperty(display, window, net_wm_pid,
        0, 1, False, XA_CARDINAL,
        &actual_type, &actual_format, &nitems, &bytes_after, &prop);

    if (result != Success || !prop) return -1;

    int pid = (int)(*(unsigned long *)prop);
    XFree(prop);
    return pid;
}

static void get_window_geometry(Display *display, Window window, WindowInfo *info) {
    Window root_return, child_return;
    int x, y;
    unsigned int width, height, border, depth;

    XGetGeometry(display, window, &root_return, &x, &y, &width, &height, &border, &depth);

    int dest_x, dest_y;
    XTranslateCoordinates(display, window, root_return, 0, 0, &dest_x, &dest_y, &child_return);

    info->x = dest_x;
    info->y = dest_y;
    info->width = (int)width;
    info->height = (int)height;
}

static void get_window_state(Display *display, Window window, WindowInfo *info) {
    Atom net_wm_state = XInternAtom(display, "_NET_WM_STATE", True);
    Atom maximized_vert = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", True);
    Atom maximized_horz = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", True);
    Atom hidden = XInternAtom(display, "_NET_WM_STATE_HIDDEN", True);

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = NULL;

    info->is_maximized = 0;
    info->is_minimized = 0;

    if (XGetWindowProperty(display, window, net_wm_state,
        0, 1024, False, XA_ATOM,
        &actual_type, &actual_format, &nitems, &bytes_after, &prop) == Success && prop) {

        Atom *states = (Atom *)prop;
        int has_max_vert = 0, has_max_horz = 0;
        for (unsigned long i = 0; i < nitems; i++) {
            if (states[i] == maximized_vert) has_max_vert = 1;
            if (states[i] == maximized_horz) has_max_horz = 1;
            if (states[i] == hidden) info->is_minimized = 1;
        }
        if (has_max_vert && has_max_horz) info->is_maximized = 1;
        XFree(prop);
    }
}

static int get_window_desktop(Display *display, Window window) {
    Atom net_wm_desktop = XInternAtom(display, "_NET_WM_DESKTOP", True);
    if (net_wm_desktop == None) return 0;

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = NULL;

    int result = XGetWindowProperty(display, window, net_wm_desktop,
        0, 1, False, XA_CARDINAL,
        &actual_type, &actual_format, &nitems, &bytes_after, &prop);

    if (result != Success || !prop) return 0;

    int desktop = (int)(*(unsigned long *)prop);
    XFree(prop);
    return desktop;
}

static void get_process_cmd(int pid, WindowInfo *info) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

    FILE *f = fopen(path, "r");
    if (!f) return;

    char buf[MAX_CMD_ARGS * MAX_ARG_LEN];
    size_t len = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[len] = '\0';

    info->cmd_argc = 0;
    size_t i = 0;
    while (i < len && info->cmd_argc < MAX_CMD_ARGS) {
        strncpy(info->cmd[info->cmd_argc], buf + i, MAX_ARG_LEN - 1);
        info->cmd[info->cmd_argc][MAX_ARG_LEN - 1] = '\0';
        i += strlen(buf + i) + 1;
        info->cmd_argc++;
    }

    char exe_path[256];
    snprintf(exe_path, sizeof(exe_path), "/proc/%d/exe", pid);
    ssize_t r = readlink(exe_path, info->exe_path, sizeof(info->exe_path) - 1);
    if (r > 0) info->exe_path[r] = '\0';
}

static int is_system_process(const char *cmd) {
    const char *system_procs[] = {
        "systemd", "dbus", "Xorg", "xfce4-session", "xfwm4",
        "xfdesktop", "xfce4-panel", "pulseaudio", "pipewire",
        "bash", "sh", "zsh", "fish", NULL
    };
    for (int i = 0; system_procs[i] != NULL; i++) {
        if (strstr(cmd, system_procs[i])) return 1;
    }
    return 0;
}

WindowList *capture_windows(Display *display) {
    WindowList *list = calloc(1, sizeof(WindowList));
    if (!list) return NULL;

    Window root = DefaultRootWindow(display);
    Atom net_client_list = XInternAtom(display, "_NET_CLIENT_LIST", True);
    if (net_client_list == None) {
        fprintf(stderr, "sessionsnap: _NET_CLIENT_LIST not supported\n");
        return list;
    }

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;

    if (XGetWindowProperty(display, root, net_client_list,
        0, (~0L), False, XA_WINDOW,
        &actual_type, &actual_format, &nitems, &bytes_after, &data) != Success) {
        return list;
    }

    Window *windows = (Window *)data;

    for (unsigned long i = 0; i < nitems && list->count < MAX_WINDOWS; i++) {
        WindowInfo info;
        memset(&info, 0, sizeof(info));

        info.window_id = windows[i];
        info.pid = get_window_pid(display, windows[i]);

        if (info.pid <= 0) continue;

        char *name = NULL;
        XFetchName(display, windows[i], &name);
        if (name) {
            strncpy(info.title, name, sizeof(info.title) - 1);
            XFree(name);
        }

        get_window_geometry(display, windows[i], &info);
        get_window_state(display, windows[i], &info);
        info.desktop = get_window_desktop(display, windows[i]);
        get_process_cmd(info.pid, &info);

        if (info.cmd_argc > 0 && is_system_process(info.cmd[0])) continue;
        if (info.cmd_argc == 0) continue;

        list->windows[list->count++] = info;
    }

    XFree(data);
    return list;
}

void free_window_list(WindowList *list) {
    free(list);
}