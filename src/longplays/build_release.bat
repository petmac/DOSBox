echo Copying DLLs to %1

set SRC_DIR=..\src\longplays

copy %SRC_DIR%\SDL-1.2.15\lib\x86\SDL.dll %1
copy %SRC_DIR%\SDL_net-1.2.8\lib\x86\SDL_net.dll %1
copy %SRC_DIR%\pdc34dllw\pdcurses.dll %1
