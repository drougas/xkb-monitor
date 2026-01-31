# xkb-monitor

A lightweight utility that monitors keyboard state on Wayland displays. It
reports keyboard layout changes and lock key states (Caps Lock, Num Lock,
Scroll Lock) in real-time.

Tested on labwc.

## Dependencies

- `libwayland-client` - Wayland client library
- `libxkbcommon` - XKB keyboard handling library
- `libxkbregistry` - XKB registry for layout lookup (part of libxkbcommon)

### Debian / Ubuntu / Linux Mint

```bash
sudo apt install libwayland-dev libxkbcommon-dev
```

### Arch Linux / Manjaro

```bash
sudo pacman -S wayland libxkbcommon
```

## Building

```bash
make
```

For a debug build with address sanitizer:

```bash
make debug
```

## Installation

Build, then Copy the binary to a directory in your `$PATH`:

```bash
make
sudo cp xkb-monitor /usr/local/bin/
```

## Usage

```bash
xkb-monitor        # CSV output
xkb-monitor -j     # JSON output
xkb-monitor -s     # Symbol only (layout changes only)
xkb-monitor -js    # JSON symbol only (Waybar compatible)
```

### Output Formats

**CSV** (default):

```
index,description,name,variant,symbol,caps,num,scroll
0,English (US),us,,us,0,0,0
1,Greek,gr,,gr,0,0,0
```

**JSON** (`xkb-monitor -j`):

```json
{"index":0,"description":"English (US)","name":"us","variant","","symbol":"us","caps":false,"num":false,"scroll":false}
{"index":1,"description":"Greek","name":"gr","variant","","symbol":"gr","caps":false,"num":false,"scroll":false}
```

**Symbol only** (`xkb-monitor -s`):

```
us
gr
```

**Symbol only - JSON** (`xkb-monitor -js`):

```
{"text":"us"}
{"text":"gr"}
```

### Waybar Integration

Add to your Waybar config:

```json
"custom/keyboard": {
    "exec": "xkb-monitor -s",
    "format": "⌨ {}"
}
```

To process the output (ie., running `sed` etc), make sure the tool does not
buffer stdout. For example, use `sed -u`:

```json
  "custom/keyboard-layout": {
    "exec": "xkb-monitor -s | sed -u 's/us/e/; s/gr/λ/'",
    "format": "⌨ {}"
  }
```

## License

MIT
