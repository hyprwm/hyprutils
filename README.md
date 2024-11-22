# hyprutils

Hyprutils is a small C++ library for utilities used across the Hypr* ecosystem.

## Stability

Hyprutils depends on the ABI stability of the stdlib implementation of your compiler. Sover bumps will be done only for hyprutils ABI breaks, not stdlib.

## Building

```sh
git clone https://github.com/hyprwm/hyprutils.git
cd hyprutils/
cmake --no-warn-unused-cli -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr -S . -B ./build
cmake --build ./build --config Release --target all -j`nproc 2>/dev/null || getconf NPROCESSORS_CONF`
sudo cmake --install build
```
