#!/bin/sh

FORMAT_FOLDERS="renderer/renderer engine/engine examples"

find $FORMAT_FOLDERS  -regex '.*\.\(c\|h\)' | xargs clang-format -i
