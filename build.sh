#!/bin/sh
set -e

SRC="typit.c"
OUT="typit"

TESTDURATION_OVERRIDE=""
LISTPATH_OVERRIDE=""

while [ $# -gt 0 ]; do
  case "$1" in
    --td)
      [ -n "$2" ] || { echo "Missing value for --td"; exit 1; }
      TESTDURATION_OVERRIDE="$2"
      shift 2
      ;;
    --words)
      [ -n "$2" ] || { echo "Missing value for --words"; exit 1; }
      LISTPATH_OVERRIDE="$2"
      shift 2
      ;;
    -h|--help)
      cat <<EOF
Usage: $0 [--td <seconds>] [--words <path>]

  --td <seconds>   Override TESTDURATION macro (positive integer)
  --words <path>   Override LISTPATH macro (string path)

Examples:
  $0 --td 45
  $0 --words ./lists/english.txt
  $0 --td 60 --words /tmp/mywords.txt
EOF
      exit 0
      ;;
    *)
      echo "Unknown option: $1 (use -h)" >&2
      exit 1
      ;;
  esac
done

echo ">>> Checking for ncurses..."
if pkg-config --exists ncurses; then
  CFLAGS="$(pkg-config --cflags ncurses)"
  LIBS="$(pkg-config --libs ncurses)"
else
  if echo '#include <ncurses.h>' | cc -E - >/dev/null 2>&1; then
    CFLAGS=""
    LIBS="-lncurses"
  else
    echo "Error: ncurses development files not found."
    echo
    echo "Please install ncurses for your system:"
    echo "  Debian/Ubuntu:   sudo apt install libncurses5-dev libncursesw5-dev"
    echo "  Fedora:          sudo dnf install ncurses-devel"
    echo "  Arch Linux:      sudo pacman -S ncurses"
    exit 1
  fi
fi

DFLAGS=""
if [ -n "$TESTDURATION_OVERRIDE" ]; then
  case "$TESTDURATION_OVERRIDE" in
    ''|*[!0-9]*)
      echo "Invalid --td (must be a positive integer)" >&2
      exit 1 ;;
    *)
      if [ "$TESTDURATION_OVERRIDE" -le 0 ]; then
        echo "Invalid --td (must be > 0)" >&2
        exit 1
      fi
      DFLAGS="$DFLAGS -DTESTDURATION=$TESTDURATION_OVERRIDE"
      ;;
  esac
fi

if [ -n "$LISTPATH_OVERRIDE" ]; then
  ESCAPED=$(printf '%s' "$LISTPATH_OVERRIDE" | sed 's/\\/\\\\/g; s/"/\\"/g')
  DFLAGS="$DFLAGS -DLISTPATH=\"${ESCAPED}\""
fi

echo ">>> Compiling $SRC -> $OUT"
cc -O2 -Wall -Wextra $CFLAGS $DFLAGS "$SRC" -o "$OUT" $LIBS -lpthread

echo ">>> Build complete. Run with: ./$OUT"
