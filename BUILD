#!/bin/bash
#
# Vyrobi makefile a spusti make.
#

MKFIN="Makefile.in"
MKFILE="Makefile"


# vyrob makefile
(
  cat <"$MKFIN"
  echo
  # vyrob automaticky zavislosti mezi .c a .h soubory
  DEPFILES="$MKFIN" ./mkdep *.c *.h
) > "$MKFILE"

make
