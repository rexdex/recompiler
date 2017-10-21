mkdir _deploy
xcopy /Y /S _bin\Release\*.dll _deploy\bin\release\
xcopy /Y /S _bin\Release\*.exe _deploy\bin\release\
xcopy /Y /S _bin\Release\*.xml _deploy\bin\release\
xcopy /Y /S data\*.* _deploy\data\
xcopy /Y projects\xenon\dolphin\*.* _deploy\projects\xenon\dolphin\
xcopy /Y /S projects\xenon\dolphin\Media\*.* _deploy\projects\xenon\dolphin\Media\
xcopy /Y dev\src\host_core\*.h _deploy\dev\src\host_core\
xcopy /Y dev\src\xenon_launcher\xenonCPU.h _deploy\dev\src\xenon_launcher\
