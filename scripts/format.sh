#!/bin/bash


echo "Checking formatting for all .cpp and .h files in src/..."
echo "---"

MODIFIED_COUNT=0
UNTOUCHED_COUNT=0

# Recursively find all target files in the src directory
while IFS= read -r -d '' file; do
    # Create a temporary copy to compare against
    cp "$file" "$file.bak" > /dev/null

    # Apply formatting to the original file
    clang-format -i -style=file "$file"

    # Compare the formatted file with the backup
    if ! cmp -s "$file" "$file.bak"; then
        echo "[Formatted] $file"
        ((MODIFIED_COUNT++))
    else
        echo "[Unchanged] $file"
        ((UNTOUCHED_COUNT++))
    fi

    # Clean up the temporary file
    rm "$file.bak"
done < <(find src -type f \( -name "*.cpp" -o -name "*.h" \) -print0)

echo
echo "---"
echo "Formatting complete."
echo "$MODIFIED_COUNT file(s) were reformatted."
echo "$UNTOUCHED_COUNT file(s) were already compliant."
