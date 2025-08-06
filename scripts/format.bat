@echo off
setlocal

echo Checking formatting for all .cpp and .h files in src/...
echo ---

set "MODIFIED_COUNT=0"
set "UNTOUCHED_COUNT=0"

rem Recursively find all target files in the src directory
for /r "src" %%f in (*.cpp, *.h) do (
    rem Create a temporary copy to compare against
    copy "%%f" "%%f.bak" > nul

    rem Apply formatting to the original file
    clang-format -i -style=file "%%f"

    rem Compare the formatted file with the backup. fc returns errorlevel 1 if different.
    fc /b "%%f" "%%f.bak" > nul
    if errorlevel 1 (
        echo [Formatted] %%f
        set /a "MODIFIED_COUNT+=1"
    ) else (
        echo [Unchanged] %%f
        set /a "UNTOUCHED_COUNT+=1"
    )

    rem Clean up the temporary file
    del "%%f.bak"
)

echo.
echo ---
echo Formatting complete.
echo %MODIFIED_COUNT% file(s) were reformatted.
echo %UNTOUCHED_COUNT% file(s) were already compliant.