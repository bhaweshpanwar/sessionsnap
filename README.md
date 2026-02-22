# SessionSnap

A lightweight session restoration tool for Linux. It watches your open windows, saves their state to a JSON file every 60 seconds, and brings everything back exactly where you left it after a reboot or crash.

No cloud. No electron. No bloat. Just a small C binary reading `/proc` and talking to X11.

---

## What it actually does

- Scans all open windows using X11's `_NET_CLIENT_LIST`
- Reads each window's position, size, PID, and command from `/proc`
- Saves that to `~/.sessionsnap/session.json`
- On next login, shows a GTK popup — one click restores everything
- Supports named profiles like `deep-work` or `gaming`

---

## Requirements

- Linux with X11 (Wayland not supported)
- XFCE or any EWMH-compliant window manager
- GCC, Make

Install dependencies on Arch:

```bash
sudo pacman -S libx11 gtk3 pkg-config wmctrl xdotool gdb
```

On Debian/Ubuntu:

```bash
sudo apt install libx11-dev libgtk-3-dev pkg-config wmctrl xdotool build-essential
```

---

## Setup

**1. Clone the repo**

```bash
git clone https://github.com/yourusername/sessionsnap.git
cd sessionsnap
```

**2. Get cJSON** (single file JSON library, dropped into vendor/)

```bash
cd vendor
curl -O https://raw.githubusercontent.com/DaveGamble/cJSON/master/cJSON.c
curl -O https://raw.githubusercontent.com/DaveGamble/cJSON/master/cJSON.h
cd ..
```

**3. Build**

```bash
make
```

That's it. Binary lands in the project root as `sessionsnap`.

---

## Project structure

```
sessionsnap/
├── src/
│   ├── main.c        entry point, CLI arg routing
│   ├── capture.c     X11 window scanning via /proc
│   ├── session.c     save and load session JSON
│   ├── restore.c     relaunch apps and reposition windows
│   ├── monitor.c     background loop, snapshots every 60s
│   └── gui.c         GTK restore dialog on startup
├── include/          header files for all modules
├── vendor/           cJSON (you add this manually, see setup)
└── Makefile
```

---

## Usage

```bash
./sessionsnap --list                          # show all detected windows
./sessionsnap --snapshot                      # save current session
./sessionsnap --restore                       # restore last saved session
./sessionsnap --daemon                        # run in background, auto-saves every 60s
./sessionsnap --gui                           # show restore popup dialog

./sessionsnap --snapshot --profile deep-work  # save a named profile
./sessionsnap --restore  --profile deep-work  # restore a named profile
```

Sessions are stored in `~/.sessionsnap/`:

```
~/.sessionsnap/
├── session.json          last auto-saved session
└── sessions/
    ├── deep-work.json
    └── morning.json
```

---

## Autostart on login

Once you're happy with it, make it run automatically on every login.

**Create the service file:**

```bash
mkdir -p ~/.config/systemd/user
nano ~/.config/systemd/user/sessionsnap.service
```

**Paste this** (update the path to match where your binary is):

```ini
[Unit]
Description=SessionSnap restore prompt
After=graphical-session.target

[Service]
Type=oneshot
ExecStart=/home/youruser/sessionsnap/sessionsnap --gui

[Install]
WantedBy=graphical-session.target
```

**Enable it:**

```bash
systemctl --user enable sessionsnap.service
systemctl --user start sessionsnap.service
```

For the background daemon that keeps auto-saving:

```ini
[Unit]
Description=SessionSnap background monitor
After=graphical-session.target

[Service]
Type=simple
ExecStart=/home/youruser/sessionsnap/sessionsnap --daemon
Restart=on-failure

[Install]
WantedBy=graphical-session.target
```

```bash
systemctl --user enable sessionsnap-daemon.service
systemctl --user start sessionsnap-daemon.service
```

---

## Makefile targets

```bash
make                  # build
make clean            # remove binary
make test-list        # run --list
make test-snapshot    # run --snapshot
make test-restore     # run --restore
make install          # copy binary to /usr/local/bin
make uninstall        # remove from /usr/local/bin
```

---

## Known limitations

- X11 only — Wayland support would require a full rewrite using wlroots or similar
- Some apps don't restore tabs or internal state, only the window position
- Apps that take long to open may not reposition correctly — increase the timeout in `restore.c` if needed
- Terminal sessions are relaunched but their history/content is not preserved

---

## Built with

- C99
- X11 / Xlib
- GTK3
- [cJSON](https://github.com/DaveGamble/cJSON) by Dave Gamble