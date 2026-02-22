/*
 * monitor.c — runs a continuous background loop that captures and saves windows every 60s
 * talks to: capture.c (capture_windows), session.c (save_session), monitor.h
 * imports: signal.h for SIGTERM/SIGINT handling, unistd.h for sleep()
 * functions: start_monitor(), stop_monitor(), snapshot_once(), signal_handler()
 */

#include "../include/monitor.h"
#include "../include/capture.h"
#include "../include/session.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <X11/Xlib.h>

static volatile int running = 1;
static Display *display = NULL;

static void signal_handler(int sig) {
    (void)sig;
    printf("\nsessionsnap: signal received, saving final snapshot...\n");
    snapshot_once();
    running = 0;
}

void snapshot_once(void) {
    if (!display) {
        display = XOpenDisplay(NULL);
        if (!display) {
            fprintf(stderr, "sessionsnap: cannot open display\n");
            return;
        }
    }

    WindowList *list = capture_windows(display);
    if (!list) return;

    if (list->count == 0) {
        printf("sessionsnap: no user windows found to snapshot\n");
        free_window_list(list);
        return;
    }

    save_session(list, "default");
    free_window_list(list);
}

void start_monitor(void) {
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "sessionsnap: cannot open X display\n");
        return;
    }

    printf("sessionsnap: monitor started, snapshotting every %d seconds\n",
           MONITOR_INTERVAL_SECONDS);

    while (running) {
        snapshot_once();
        for (int i = 0; i < MONITOR_INTERVAL_SECONDS && running; i++) {
            sleep(1);
        }
    }

    printf("sessionsnap: monitor stopped\n");
    XCloseDisplay(display);
    display = NULL;
}

void stop_monitor(void) {
    running = 0;
}