
SET OUTPUT=%~1
SET CONFIGURATION=%~2
SET PLATFORM=%~3

IF NOT "%PLATFORM%" == "x64" GOTO END
xcopy "%0\..\xinput1_3.dll" "%OUTPUT%" /i /d /y

:END
EXIT /B 0
