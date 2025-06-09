# pystray-freedesktop

This is a fork of the [pystray](https://github.com/moses-palmer/pystray) library with enhanced freedesktop.org support for Linux systems.

## What's different in this fork?

This fork includes patches that improve compatibility with Linux desktop environments that follow freedesktop.org standards, particularly enhancing system tray icon functionality in modern Linux desktop environments.

## Why this fork?

While working on [GTK LLM Chat](https://gtk-llm-chat.fuentelibre.org/), we needed better Linux system tray support. The patches in this fork provide:

- Enhanced freedesktop.org compatibility
- Better integration with modern Linux desktop environments
- Improved system tray icon handling

## Installation

```bash
pip install pystray-freedesktop
```

## Usage

This fork maintains full API compatibility with the original pystray library:

```python
import pystray
from pystray import MenuItem as item
from PIL import Image

# Your existing pystray code works unchanged
def quit_action(icon):
    icon.stop()

image = Image.open("icon.png")
menu = pystray.Menu(item('Quit', quit_action))
icon = pystray.Icon("test", image, menu=menu)
icon.run()
```

## Contributing

This is a temporary fork maintained for the GTK LLM Chat project. We aim to contribute these improvements back to the upstream pystray project.

For issues related to this fork, please report them at: https://github.com/fuentelibre/gtk-llm-chat/issues

For the original pystray project, visit: https://github.com/moses-palmer/pystray

## License

This project maintains the same LGPL-3.0-or-later license as the original pystray library.

## Maintainer

This fork is maintained by Sebastian Silva (sebastian@fuentelibre.org) as part of the GTK LLM Chat project.

Original pystray author: Moses Palmér
