#pragma once

/// run external application, returns exit code of the application
/// all output of the application is passed to the log
extern RECOMPILER_API uint32 RunApplication(ILogOutput& log, const std::wstring& executablePath, const std::wstring& commandLine);
