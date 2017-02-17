@ECHO OFF

TITLE FFPlay

IF NOT EXIST ffplay.exe (
  CLS
  ECHO ffplay.exe could not be found.
  GOTO:error
)

ECHO ffplay found.

ffplay.exe -window_title "FFPlay Demo" -x 720 -y 576 -t 00:10 -autoexit -i ../video/out.h264

:error
ECHO.
ECHO Press any key to exit.
PAUSE >nul
GOTO:EOF