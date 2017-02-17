@ECHO OFF

TITLE FFprobe

IF NOT EXIST ffprobe.exe (
  CLS
  ECHO ffprobe.exe could not be found.
  GOTO:error
)

ECHO ffprobe found.

ffprobe.exe -i ../video/VideoCameraRecorder.avi

:error
ECHO.
ECHO Press any key to exit.
PAUSE >nul
GOTO:EOF