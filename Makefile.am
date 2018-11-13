AUTOMAKE_OPTIONS = foreign

SUBDIRS = src .

# Copy README.md to README when building distribution
dist-hook:
	[ -f README.md ] && cat README.md > README || true
