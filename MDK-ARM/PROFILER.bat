@echo off
del "OUTPUTFILE.TXT"
UV4.exe -b %1 -j0 -o"OUTPUTFILE.TXT"
echo.
echo ================================================================ BUILD LOG ================================================================
type OUTPUTFILE.TXT
