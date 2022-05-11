#!/usr/bin/env bash

# --------------------------------------------------------------------
# Unzip 'parg'

# First of all - clear (previously) extracted directory, if
# it's exists. It is useful for re-running this script during
# sources changing/hacking
if [[ -d ./third_party/parg/ ]]; then 
    rm -rf ./third_party/parg/
fi

# Unzip 'parg'
unzip ./third_party/parg-master.zip -d ./third_party/

# Rename 'parg-master' into 'parg'
mv ./third_party/parg-master/ ./third_party/parg/

# --------------------------------------------------------------------
# Compile executable

gcc \
    -O3 \
    \
    -Wall -Wextra -Werror \
    -std=c89 -ansi -pedantic -pedantic-errors \
    \
    -I ./third_party/parg/ \
    \
    ./sources/main.c \
    ./third_party/parg/parg.c \
    \
    -o bin2src

# --------------------------------------------------------------------
# Cleanup on success

retVal=$?
if [[ $retVal -eq 0 ]]; then
    rm -rf ./third_party/parg/
fi
