# hyprutils

Hyprutils is a small C++ library for utilities used across the Hypr* ecosystem.

## Documentation

API documentation is published at <https://hyprwm.github.io/hyprutils/>, and its
source lives in [docs/](docs/). To build it locally:

```sh
make -C docs html    # output in docs/_build/html
make -C docs live    # live-reloading server on 127.0.0.1:8000
```

Python dependencies are handled by [uv](https://docs.astral.sh/uv/); no manual
virtualenv setup is needed.

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
