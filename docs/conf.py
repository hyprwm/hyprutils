# Configuration file for the Sphinx documentation builder.
# https://www.sphinx-doc.org/en/master/usage/configuration.html

from pathlib import Path

# -- Project information -----------------------------------------------------

project = "hyprutils"
author = "hyprwm"
copyright = "hyprwm"

# Keep the docs in sync with the VERSION file at the repository root.
_version_file = Path(__file__).resolve().parent.parent / "VERSION"
release = _version_file.read_text().strip() if _version_file.is_file() else "unknown"
# MAJOR.MINOR convention
version = ".".join(release.split(".")[:2])

# -- General configuration ---------------------------------------------------

extensions = [
    "myst_parser",
    "sphinx_copybutton",
    "sphinx_design",
]

exclude_patterns = ["_build", ".venv", "Thumbs.db", ".DS_Store"]

# deflist: term/definition lists. colon_fence: ::: directives, which can wrap
# ``` code fences without counting backticks.
myst_enable_extensions = ["deflist", "colon_fence"]

# Everything here is C++, so default to it for code fences and inline roles.
primary_domain = "cpp"
highlight_language = "cpp"

cpp_index_common_prefix = ["Hyprutils::Signal::", "Hyprutils::"]

# -- Options for HTML output -------------------------------------------------

html_theme = "furo"
html_title = f"hyprutils {release}"

html_theme_options = {
    "source_repository": "https://github.com/hyprwm/hyprutils/",
    "source_branch": "main",
    "source_directory": "docs/",
}

# -- Options for sphinx-copybutton -------------------------------------------

# Strip shell prompts so copying a `$ cmd` block yields just `cmd`.
copybutton_prompt_text = r"\$ "
copybutton_prompt_is_regexp = True
