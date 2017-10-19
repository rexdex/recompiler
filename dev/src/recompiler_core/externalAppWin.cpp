#include "build.h"
#include "externalApp.h"

#if defined(_WIN32) || defined(_WIN64)

#include <Windows.h>

namespace prv
{

	//--------------------------------------------------------------------------

	class TaskIOReader
	{
	public:
		TaskIOReader(ILogOutput& log, const char* prefix, const bool stripErrors);
		~TaskIOReader();

		inline HANDLE GetHandle() { return m_hStdOutWrite; }

	private:
		HANDLE m_hStdOutRead;
		HANDLE m_hStdOutWrite;

		HANDLE m_hThread;
		volatile bool m_exitThread;

		bool m_stripErrors;

		const char* m_prefix;

		class ILogOutput*	m_log;

		class LineBuilder
		{
		public:
			LineBuilder(const bool stripErrors);
			void Append(ILogOutput& log, const char* prefix, const uint8* data, const uint32 dataSize);

		private:
			std::vector<uint8>	m_buffer;
			bool				m_stripErrors;
		};

		static DWORD WINAPI ThreadProc(LPVOID pData);
	};

	TaskIOReader::LineBuilder::LineBuilder(const bool stripErrors)
		: m_stripErrors(stripErrors)
	{
	}

	void TaskIOReader::LineBuilder::Append(ILogOutput& log, const char* prefix, const uint8* data, const uint32 dataSize)
	{
		for (uint32 i = 0; i < dataSize; ++i)
		{
			const uint8 ch = data[i];
			if (ch == '\r')
			{
				if (!m_buffer.empty())
				{
					m_buffer.push_back(0);

					const char* line = (const char*)&m_buffer[0];

					if (m_stripErrors)
					{
						const char* errPtr = strstr(line, "): ");
						if (errPtr)
						{
							*(char*)errPtr = 0;

							const char* fileName = strrchr(line, '\\');
							if (fileName)
								line = fileName + 1;

							*(char*)errPtr = ')';
						}

						const char* projectPtr = strrchr(line, '[');
						if (projectPtr)
						{
							*(char*)projectPtr = 0;
						}
					}

					log.Log("%s: %s", prefix, line);
				}

				m_buffer.clear();
			}

			if (ch >= ' ')
				m_buffer.push_back((char)ch);
		}
	}

	TaskIOReader::TaskIOReader(ILogOutput& log, const char* prefix, const bool stripErrors)
		: m_exitThread(false)
		, m_hStdOutRead(NULL)
		, m_hStdOutWrite(NULL)
		, m_hThread(NULL)
		, m_stripErrors(stripErrors)
		, m_log(&log)
		, m_prefix(prefix)
	{
		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;

		CreatePipe(&m_hStdOutRead, &m_hStdOutWrite, &saAttr, 0);

		m_hThread = CreateThread(NULL, 64 << 10, &ThreadProc, this, 0, NULL);
	}

	TaskIOReader::~TaskIOReader()
	{
		m_exitThread = true;
		WaitForSingleObject(m_hThread, 2000);
		CloseHandle(m_hThread);

		CloseHandle(m_hStdOutRead);
		m_hStdOutRead = NULL;

		CloseHandle(m_hStdOutWrite);
		m_hStdOutWrite = NULL;
	}

	DWORD WINAPI TaskIOReader::ThreadProc(LPVOID pData)
	{
		TaskIOReader* log = (TaskIOReader*)pData;

		LineBuilder lineBuilder(log->m_stripErrors);

		while (1)
		{
			// buffer size
			BYTE readBuffer[128];

			// has data ?
			DWORD numAvaiable = 0;
			PeekNamedPipe(log->m_hStdOutRead, NULL, NULL, NULL, &numAvaiable, NULL);
			if (!numAvaiable)
			{
				if (log->m_exitThread)
					break;

				Sleep(10);
				continue;
			}

			// read data
			DWORD numRead = 0;
			DWORD numToRead = numAvaiable;
			if (numToRead > sizeof(readBuffer))
				numToRead = sizeof(readBuffer);

			if (!ReadFile(log->m_hStdOutRead, readBuffer, sizeof(readBuffer), &numRead, NULL))
				break;

			// append to line printer
			lineBuilder.Append(*log->m_log, log->m_prefix, readBuffer, numRead);
		}

		return 0;
	}

} // prv

uint32 RunApplication(ILogOutput& log, const std::wstring& executablePath, const std::wstring& commandLine)
{
	prv::TaskIOReader output(log, "Run", false);

	PROCESS_INFORMATION pinfo;
	memset(&pinfo, 0, sizeof(pinfo));

	STARTUPINFOW startupInfo;
	memset(&startupInfo, 0, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);
	startupInfo.hStdOutput = output.GetHandle();
	startupInfo.wShowWindow = SW_HIDE;
	startupInfo.hStdError = output.GetHandle();
	startupInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

	// prepare arguments
	std::wstring temp;
	temp += L"\"";
	temp += executablePath;
	temp += L"\" ";
	temp += commandLine;

	const uint32 argsSize = (uint32) temp.length();//m_arguments.length();
	auto* args = (wchar_t*) malloc(sizeof(wchar_t) * (argsSize+1));
	if (!args)
		return -1; // buffer was to small

	wcscpy_s(args, argsSize+1, temp.c_str());//m_arguments.c_str());

	// start process
	if (!CreateProcessW(
		NULL,//m_command.c_str(),		// lpApplicationName
		args,					// lpCommandLine
		NULL,					// lpProcessAttributes
		NULL,					// lpThreadAttributes
		TRUE,					// bInheritHandles
		0,						// dwCreationFlags
		NULL,					// lpEnvironment
		NULL,					// lpCurrentDirectory
		&startupInfo,			// lpStartupInfo
		&pinfo))
	{
		log.Error( "Run: Failed to start '%ls'", executablePath.c_str() );
		return -1;
	}

	// wait for finish
	while (1)
	{
		// wait for compilation task to end
		const int ret = WaitForSingleObject(pinfo.hProcess, 50);
		if (ret == WAIT_TIMEOUT)
			continue;

		// end
		if (ret == WAIT_OBJECT_0)
			break;

		// error
		log.Error( "Run: Process '%ls' did not terminate successfully: handle lost: %d", executablePath.c_str(), ret );
		CloseHandle(pinfo.hProcess);
		CloseHandle(pinfo.hThread);
		return -1;
	}

	// get exit code
	int exitCode = -1;
	GetExitCodeProcess(pinfo.hProcess, (DWORD*) &exitCode);
	if (exitCode != 0)
	{
		log.Error( "Run: Process '%ls' terminated with exit code %d", executablePath.c_str(), exitCode );
		Sleep(100);
		return exitCode;
	}

	// close local handles
	CloseHandle(pinfo.hProcess);
	CloseHandle(pinfo.hThread);

	// return error code
	return exitCode;
}

#else

uint32 RunApplication(ILogOutput& log, const std::wstring& executablePath, const std::wstring& commandLine)
{
	log.Error("Run: Running external applications was not implemented on this platform");
	return 0;
}

#endif