# Minimal makefile for Sphinx documentation.
# Python dependencies are managed with uv (see pyproject.toml); `uv run`
# discovers this directory's project and creates/syncs the venv on demand.

UV            ?= uv
SPHINXOPTS    ?= -W --keep-going
SPHINXBUILD   ?= $(UV) run sphinx-build
SOURCEDIR     = .
BUILDDIR      = _build

.PHONY: help live Makefile

help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

# Live-reloading dev server on http://127.0.0.1:8000
live:
	@$(UV) run --group dev sphinx-autobuild "$(SOURCEDIR)" "$(BUILDDIR)/html" \
		--ignore "$(BUILDDIR)/*" $(SPHINXOPTS) $(O)

# Catch-all: route everything else to sphinx-build ("make html", "make clean", ...)
%: Makefile
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
