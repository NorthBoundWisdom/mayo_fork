#!/bin/bash

# set the clang-format path
CLANG_FORMAT="/opt/homebrew/bin/clang-format"

# set the target directory
TARGET_DIR="/Users/henrykang/Documents/mayo/src"
# switch to the target directory
cd "$TARGET_DIR" || exit

# counter
count=0
total=$(find . \( -name "*.cpp" -o -name "*.h" -o -name "*.cc" \) | wc -l)

# find all .cpp, .cc and .h files and process them one by one
find . \( -name "*.cpp" -o -name "*.h" -o -name "*.cc" \) -print0 | while IFS= read -r -d '' file; do
    ((count++))
    echo "[$count/$total] Formatting: $file"
    "$CLANG_FORMAT" -i "$file"

    # ensure the file ends with a newline without creating backup files
    sed -i '' -e '$a\' "$file"
done

echo "Formatting complete. $total files processed in $TARGET_DIR"
