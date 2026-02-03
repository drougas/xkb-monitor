# xkb-monitor

A lightweight utility that monitors keyboard state on Wayland compositors. It
reports keyboard layout changes and lock key states (Caps Lock, Num Lock,
Scroll Lock) in real-time.

Tested on labwc.

## Dependencies

### Required

- `libwayland-client` - Wayland client library
- `libxkbcommon` - XKB keyboard handling library
- `libxkbregistry` - XKB registry for layout lookup (part of libxkbcommon)

### Optional (for accurate initial layout reporting)

- `wayland-protocols` - Wayland protocol XML files
- `wayland-scanner` - Wayland protocol code generator
- `curl` - For downloading wlr-layer-shell protocol

When these optional dependencies are available, xkb-monitor uses
wlr-layer-shell to query the current keyboard layout on startup. Without them,
the initial layout may default to index 0 until a key is pressed.

### Debian / Ubuntu / Linux Mint

```bash
# Required
sudo apt install libwayland-dev libxkbcommon-dev

# Optional (for accurate initial layout)
sudo apt install wayland-protocols curl
```

### Arch Linux / Manjaro

```bash
# Required
sudo pacman -S wayland libxkbcommon

# Optional (for accurate initial layout)
sudo pacman -S wayland-protocols curl
```

## Building

```bash
make
```

This will automatically detect if `wayland-protocols` and `wayland-scanner` are
available. If found, the build will:
1. Generate xdg-shell protocol files from wayland-protocols
2. Download and generate wlr-layer-shell protocol files
3. Enable accurate initial layout reporting

To explicitly disable layer-shell support even when dependencies are available:

```bash
make SKIP_WLR_LAYER_SHELL=1
```

For a debug build with address sanitizer:

```bash
make debug
```

To clean generated protocol files:

```bash
make distclean
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
xkb-monitor -n     # Name only (layout changes only)
xkb-monitor -jn    # JSON name only (Waybar compatible)
```

### Output Formats

**CSV** (default):

```
index,description,name,variant,caps,num,scroll
0,English (US),us,,0,0,0
1,Greek,gr,,0,0,0
```

**JSON** (`xkb-monitor -j`):

```json
{"index":0,"description":"English (US)","name":"us","variant":"","caps":false,"num":false,"scroll":false}
{"index":1,"description":"Greek","name":"gr","variant":"","caps":false,"num":false,"scroll":false}
```

**Name only** (`xkb-monitor -n`):

```
us
gr
```

**Name only - JSON** (`xkb-monitor -jn`):

```
{"text":"us"}
{"text":"gr"}
```

### Waybar Integration

Add to your Waybar config:

```json
// Alternative: modules-center, or modules-left
"modules-right": [
  ...
  "custom/keyboard-layout",
  ...
],

"custom/keyboard": {
    "exec": "xkb-monitor -n",
    "format": "⌨ {}"
}
```

To process the output (ie., running `sed` etc), make sure the tool does not
buffer stdout. For example, use `sed -u`:

```json
"custom/keyboard-layout": {
  "exec": "xkb-monitor -n | sed -u 's/us/e/; s/gr/λ/'",
  "format": "⌨ {}"
}
```

## License

MIT
