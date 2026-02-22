/*
 * monitor.h — declares the background monitoring loop and signal handlers
 * talks to: monitor.c, main.c
 * uses capture.h and session.h to repeatedly snapshot and save window state
 * functions: start_monitor(), stop_monitor(), snapshot_once()
 */

#ifndef MONITOR_H
#define MONITOR_H

#define MONITOR_INTERVAL_SECONDS 60

void start_monitor(void);
void stop_monitor(void);
void snapshot_once(void);

#endif