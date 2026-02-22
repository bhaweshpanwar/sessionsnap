/*
 * session.c — saves WindowList to JSON and loads it back into a WindowList
 * talks to: capture.h (WindowList struct), vendor/cJSON for serialization
 * imports: cJSON.h, capture.h, stdio, stdlib, string, sys/stat for mkdir
 * functions: save_session(), load_session(), get_session_path(), session_file_exists()
 */

#include "../include/session.h"
#include "../vendor/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void get_session_path(char *out, size_t size, const char *profile_name) {
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";

    if (!profile_name || strcmp(profile_name, "default") == 0) {
        snprintf(out, size, "%s%s", home, SESSION_FILE);
    } else {
        snprintf(out, size, "%s%s/%s.json", home, SESSIONS_DIR, profile_name);
    }
}

static void ensure_dirs_exist(void) {
    const char *home = getenv("HOME");
    if (!home) return;

    char path[512];
    snprintf(path, sizeof(path), "%s%s", home, SESSION_DIR);
    mkdir(path, 0755);

    snprintf(path, sizeof(path), "%s%s", home, SESSIONS_DIR);
    mkdir(path, 0755);
}

int session_file_exists(const char *profile_name) {
    char path[512];
    get_session_path(path, sizeof(path), profile_name);
    return access(path, F_OK) == 0;
}

int save_session(const WindowList *list, const char *profile_name) {
    ensure_dirs_exist();

    cJSON *root = cJSON_CreateObject();
    cJSON *windows_arr = cJSON_CreateArray();

    for (int i = 0; i < list->count; i++) {
        const WindowInfo *w = &list->windows[i];
        cJSON *win = cJSON_CreateObject();

        cJSON_AddNumberToObject(win, "pid", w->pid);
        cJSON_AddStringToObject(win, "title", w->title);
        cJSON_AddStringToObject(win, "exe_path", w->exe_path);
        cJSON_AddNumberToObject(win, "x", w->x);
        cJSON_AddNumberToObject(win, "y", w->y);
        cJSON_AddNumberToObject(win, "width", w->width);
        cJSON_AddNumberToObject(win, "height", w->height);
        cJSON_AddNumberToObject(win, "desktop", w->desktop);
        cJSON_AddNumberToObject(win, "is_maximized", w->is_maximized);
        cJSON_AddNumberToObject(win, "is_minimized", w->is_minimized);

        cJSON *cmd_arr = cJSON_CreateArray();
        for (int j = 0; j < w->cmd_argc; j++) {
            cJSON_AddItemToArray(cmd_arr, cJSON_CreateString(w->cmd[j]));
        }
        cJSON_AddItemToObject(win, "cmd", cmd_arr);

        cJSON_AddItemToArray(windows_arr, win);
    }

    cJSON_AddItemToObject(root, "windows", windows_arr);
    cJSON_AddNumberToObject(root, "count", list->count);

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!json_str) return -1;

    char path[512];
    get_session_path(path, sizeof(path), profile_name);

    FILE *f = fopen(path, "w");
    if (!f) {
        free(json_str);
        return -1;
    }

    fputs(json_str, f);
    fclose(f);
    free(json_str);

    printf("sessionsnap: saved %d windows to %s\n", list->count, path);
    return 0;
}

WindowList *load_session(const char *profile_name) {
    char path[512];
    get_session_path(path, sizeof(path), profile_name);

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "sessionsnap: cannot open %s\n", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);

    char *buf = malloc(fsize + 1);
    if (!buf) { fclose(f); return NULL; }

    fread(buf, 1, fsize, f);
    buf[fsize] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    free(buf);

    if (!root) {
        fprintf(stderr, "sessionsnap: failed to parse JSON\n");
        return NULL;
    }

    WindowList *list = calloc(1, sizeof(WindowList));
    if (!list) { cJSON_Delete(root); return NULL; }

    cJSON *windows_arr = cJSON_GetObjectItem(root, "windows");
    int count = cJSON_GetArraySize(windows_arr);

    for (int i = 0; i < count && i < MAX_WINDOWS; i++) {
        cJSON *win = cJSON_GetArrayItem(windows_arr, i);
        WindowInfo *w = &list->windows[i];

        cJSON *pid = cJSON_GetObjectItem(win, "pid");
        cJSON *title = cJSON_GetObjectItem(win, "title");
        cJSON *exe = cJSON_GetObjectItem(win, "exe_path");
        cJSON *x = cJSON_GetObjectItem(win, "x");
        cJSON *y = cJSON_GetObjectItem(win, "y");
        cJSON *width = cJSON_GetObjectItem(win, "width");
        cJSON *height = cJSON_GetObjectItem(win, "height");
        cJSON *desktop = cJSON_GetObjectItem(win, "desktop");
        cJSON *is_max = cJSON_GetObjectItem(win, "is_maximized");
        cJSON *is_min = cJSON_GetObjectItem(win, "is_minimized");
        cJSON *cmd_arr = cJSON_GetObjectItem(win, "cmd");

        if (pid) w->pid = (int)pid->valuedouble;
        if (title) strncpy(w->title, title->valuestring, sizeof(w->title) - 1);
        if (exe) strncpy(w->exe_path, exe->valuestring, sizeof(w->exe_path) - 1);
        if (x) w->x = (int)x->valuedouble;
        if (y) w->y = (int)y->valuedouble;
        if (width) w->width = (int)width->valuedouble;
        if (height) w->height = (int)height->valuedouble;
        if (desktop) w->desktop = (int)desktop->valuedouble;
        if (is_max) w->is_maximized = (int)is_max->valuedouble;
        if (is_min) w->is_minimized = (int)is_min->valuedouble;

        if (cmd_arr) {
            int argc = cJSON_GetArraySize(cmd_arr);
            for (int j = 0; j < argc && j < MAX_CMD_ARGS; j++) {
                cJSON *arg = cJSON_GetArrayItem(cmd_arr, j);
                if (arg && arg->valuestring) {
                    strncpy(w->cmd[j], arg->valuestring, MAX_ARG_LEN - 1);
                }
            }
            w->cmd_argc = argc;
        }

        list->count++;
    }

    cJSON_Delete(root);
    return list;
}