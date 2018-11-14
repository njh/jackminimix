AUTOMAKE_OPTIONS = foreign

SUBDIRS = src .

# Copy README.md to README when building distribution
dist-hook:
	[ -f README.md ] && cat README.md > README || true
	[ -f NEWS.md ] && cat NEWS.md > NEWS || true

EXTRA_DIST = README NEWS
README: dist-hook
NEWS: dist-hook

