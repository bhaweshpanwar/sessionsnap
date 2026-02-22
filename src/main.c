/*
 * main.c — entry point, parses CLI args and routes to the correct mode
 * talks to: monitor.h, restore.h, session.h, gui.h — orchestrates all modules
 * imports: all project headers, X11 for display init check
 * functions: main(), print_usage()
 * usage: ./sessionsnap [--snapshot] [--restore] [--daemon] [--gui] [--list]
 */

#include "../include/monitor.h"
#include "../include/capture.h"
#include "../include/session.h"
#include "../include/restore.h"
#include "../include/gui.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>

static void print_usage(void) {
    printf("SessionSnap — Linux session restoration manager\n\n");
    printf("Usage: sessionsnap [option]\n\n");
    printf("  --snapshot              capture current windows and save to session.json\n");
    printf("  --restore               restore apps from last saved session\n");
    printf("  --daemon                run in background, auto-snapshot every 60s\n");
    printf("  --gui                   show restore dialog on startup\n");
    printf("  --list                  list all windows currently open\n");
    printf("  --profile <name>        use a named session profile\n");
    printf("  --help                  show this help\n\n");
    printf("Examples:\n");
    printf("  sessionsnap --snapshot\n");
    printf("  sessionsnap --restore\n");
    printf("  sessionsnap --snapshot --profile deep-work\n");
    printf("  sessionsnap --restore  --profile deep-work\n");
    printf("  sessionsnap --daemon\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return 0;
    }

    const char *profile = "default";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--profile") == 0 && i + 1 < argc) {
            profile = argv[++i];
        }
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        }

        if (strcmp(argv[i], "--list") == 0) {
            Display *display = XOpenDisplay(NULL);
            if (!display) {
                fprintf(stderr, "sessionsnap: cannot open X display\n");
                return 1;
            }
            WindowList *list = capture_windows(display);
            if (!list) { XCloseDisplay(display); return 1; }

            printf("Found %d user windows:\n\n", list->count);
            for (int j = 0; j < list->count; j++) {
                WindowInfo *w = &list->windows[j];
                printf("[%d] %s\n", j + 1, w->title[0] ? w->title : "(no title)");
                printf("    PID: %d  |  pos: %d,%d  |  size: %dx%d  |  desktop: %d\n",
                    w->pid, w->x, w->y, w->width, w->height, w->desktop);
                printf("    cmd: %s\n\n", w->cmd_argc > 0 ? w->cmd[0] : "(unknown)");
            }

            free_window_list(list);
            XCloseDisplay(display);
            return 0;
        }

        if (strcmp(argv[i], "--snapshot") == 0) {
            Display *display = XOpenDisplay(NULL);
            if (!display) {
                fprintf(stderr, "sessionsnap: cannot open X display\n");
                return 1;
            }
            WindowList *list = capture_windows(display);
            if (!list) { XCloseDisplay(display); return 1; }

            int result = save_session(list, profile);
            free_window_list(list);
            XCloseDisplay(display);
            return result == 0 ? 0 : 1;
        }

        if (strcmp(argv[i], "--restore") == 0) {
            return restore_session(profile) == 0 ? 0 : 1;
        }

        if (strcmp(argv[i], "--daemon") == 0) {
            start_monitor();
            return 0;
        }

        if (strcmp(argv[i], "--gui") == 0) {
            run_gui();
            return 0;
        }

        if (strcmp(argv[i], "--profile") == 0) {
            i++;
            continue;
        }

        fprintf(stderr, "sessionsnap: unknown option '%s'\n", argv[i]);
        print_usage();
        return 1;
    }

    return 0;
}