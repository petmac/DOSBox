set DLL_SRC_DIR=..\src\longplays
set DLL_DST_DIR=%1
set DOC_SRC_DIR=..
set DOC_DST_DIR=%1Documentation

echo Copying DLLs to %DLL_DST_DIR%

copy %DLL_SRC_DIR%\SDL-1.2.15\lib\x86\SDL.dll %DLL_DST_DIR%
copy %DLL_SRC_DIR%\SDL_net-1.2.8\lib\x86\SDL_net.dll %DLL_DST_DIR%
copy %DLL_SRC_DIR%\pdc34dllw\pdcurses.dll %DLL_DST_DIR%

echo Copying documentation to %DOC_DST_DIR%

if not exist %DOC_DST_DIR% (
	mkdir %DOC_DST_DIR%
)

copy %DOC_SRC_DIR%\AUTHORS %DOC_DST_DIR%\AUTHORS.txt
copy %DOC_SRC_DIR%\COPYING %DOC_DST_DIR%\COPYING.txt
copy %DOC_SRC_DIR%\INSTALL %DOC_DST_DIR%\INSTALL.txt
copy %DOC_SRC_DIR%\NEWS %DOC_DST_DIR%\NEWS.txt
copy %DOC_SRC_DIR%\README %DOC_DST_DIR%\README.txt
copy %DOC_SRC_DIR%\THANKS %DOC_DST_DIR%\THANKS.txt
