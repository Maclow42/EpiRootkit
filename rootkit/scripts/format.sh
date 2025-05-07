#!/bin/sh

echo "Applying clang-format to all .c files recursively..."
find ../ -type f -name '*.c' -exec clang-format -i {} +
echo "Done."
