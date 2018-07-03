@echo off
set params=

echo Games
echo --------
echo.

set /p Apocalypse=Compile Apocalypse-22? (y): 
If not "%Apocalypse%"=="n" (
    set params=%params% /apocalypse
)

set /p SCP=Compile SCP? (y): 
If not "%SCP%"=="n" (
    set params=%params% /scp
)

echo.
echo Engine Configuration
echo --------
echo.

::set /p SoundEngine=Sound Engine [steamaudio_fmod,fmodstudio,source]:

::If "%SoundEngine%"=="steamaudio_fmod" (
    ::set params=%params% /define:USE_PHONON /define:USE_FMOD
::)

::If "%SoundEngine%"=="openal" (
    ::set params=%params% /define:USE_OPENAL
::)

::If "%SoundEngine%"=="fmodstudio" (
    ::set params=%params% /define:USE_FMOD
::)

set /p CEF=Use Chromium Embedded Framework? (y): 
If not "%CEF%"=="n" (
    set params=%params% /define:USE_CEF
)

::set /p L4D2Hacks=Use L4D2 Hacks? (y):
::If not "%L4D2Hacks%"=="n" (
    ::set params=%params% /define:USE_L4D2_HACKS
::)

echo.

devtools\bin\vpc.exe +gamedlls %params% /define:USE_L4D2_HACKS /mksln games.sln /2013
pause