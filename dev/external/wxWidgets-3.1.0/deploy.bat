
SET OUTPUT=%~1
SET CONFIGURATION=%~2
SET PLATFORM=%~3

IF NOT "%PLATFORM%" == "x64" GOTO END

IF "%CONFIGURATION%" == "Debug" GOTO COPYDEBUG
IF "%CONFIGURATION%" == "DebugDLL" GOTO COPYDEBUG

:COPYRELEASE
xcopy "%0\..\lib\vc_x64_dll\wxbase31u_*.dll" "%OUTPUT%" /i /d /y
xcopy "%0\..\lib\vc_x64_dll\wxbase31u_*.pdb" "%OUTPUT%" /i /d /y
xcopy "%0\..\lib\vc_x64_dll\wxmsw31u_*.dll" "%OUTPUT%" /i /d /y
xcopy "%0\..\lib\vc_x64_dll\wxmsw31u_*.pdb" "%OUTPUT%" /i /d /y
GOTO END

:COPYDEBUG
xcopy "%0\..\lib\vc_x64_dll\wxbase31ud_*.dll" "%OUTPUT%" /i /d /y
xcopy "%0\..\lib\vc_x64_dll\wxbase31ud_*.pdb" "%OUTPUT%" /i /d /y
xcopy "%0\..\lib\vc_x64_dll\wxmsw31ud_*.dll" "%OUTPUT%" /i /d /y
xcopy "%0\..\lib\vc_x64_dll\wxmsw31ud_*.pdb" "%OUTPUT%" /i /d /y
GOTO END

:END
EXIT /B 0
