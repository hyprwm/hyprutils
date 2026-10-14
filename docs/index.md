# hyprutils

Hyprutils is a small C++ library of utilities used across the Hypr\* ecosystem.
Public headers live under `include/hyprutils/`, and everything is installed as a
single shared library, `libhyprutils`.

This documentation is a work in progress. The signal system is documented first;
the remaining modules (`animation`, `cli`, `i18n`, `math`, `memory`, `os`,
`path`, `string`, `utils`) will follow.

```{toctree}
:maxdepth: 2
:caption: Modules

signal/index
```

## Conventions

* Every public header lives under `include/hyprutils/<module>/`, and its symbols
  live in the matching `Hyprutils::<Module>` namespace.
* Types are prefixed by kind: `C` for classes, `I` for interfaces, `S` for plain
  structs, `e` for enums.
* Examples assume the relevant `using namespace` unless the qualification
  matters to the point being made.

## Building the documentation

Python dependencies are managed with [uv](https://docs.astral.sh/uv/) and
declared in `docs/pyproject.toml`. No manual venv setup is needed, as `uv run`
creates and syncs one on demand.

```sh
$ make -C docs html          # output in docs/_build/html
$ make -C docs live          # live-reloading server on 127.0.0.1:8000
$ make -C docs clean
```

Both targets pass `-W --keep-going`, so warnings are errors. Keep new pages
warning-free, since CI builds with the same flags.
