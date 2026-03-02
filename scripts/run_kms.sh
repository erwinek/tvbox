#!/usr/bin/env bash
set -euo pipefail

export SDL_VIDEODRIVER=kmsdrm
export SDL_RENDER_DRIVER=opengles2
export SDL_FBDEV=/dev/fb0

./build/tvbox_gui config/app.yaml
