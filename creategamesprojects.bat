@echo off
set params=

echo Games
echo --------
echo.

set /p Apocalypse= Apocalypse-22?: (y) 
If not "%Apocalypse%"=="n" (
    set params=%params% /apocalypse
)

set /p SCP= SCP?: (y) 
If not "%SCP%"=="n" (
    set params=%params% /scp
)

echo.
echo Engine Configuration
echo --------
echo.

set /p SoundEngine=Sound Engine: (openal,fmod,source) 

If "%SoundEngine%"=="openal" (
    set params=%params% /define:USE_OPENAL
)

If "%SoundEngine%"=="fmod" (
    set params=%params% /define:USE_FMOD
)

set /p CEF= Use CEF?: (y) 
If not "%CEF%"=="n" (
    set params=%params% /define:USE_CEF
)

set /p L4D2Hacks= Use L4D2 Hacks?: (y) 
If not "%L4D2Hacks%"=="n" (
    set params=%params% /define:USE_L4D2_HACKS
)

devtools\bin\vpc.exe +gamedlls %params% /mksln games.sln /2013
pause