/*
 * session.h — declares save and load functions for session JSON files
 * talks to: session.c, monitor.c, restore.c, main.c
 * imports capture.h for WindowList struct, uses cJSON for serialization
 * functions: save_session(), load_session(), free_loaded_session()
 */

#ifndef SESSION_H
#define SESSION_H

#include "capture.h"

#define SESSION_DIR "/.sessionsnap"
#define SESSION_FILE "/.sessionsnap/session.json"
#define SESSIONS_DIR "/.sessionsnap/sessions"

int save_session(const WindowList *list, const char *profile_name);
WindowList *load_session(const char *profile_name);
void get_session_path(char *out, size_t size, const char *profile_name);
int session_file_exists(const char *profile_name);

#endif