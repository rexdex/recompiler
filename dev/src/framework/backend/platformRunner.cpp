#include "build.h"
#include "platformLibrary.h"
#include "xmlReader.h"
#include "internalUtils.h"
#include "internalFile.h"
#include "platformRunner.h"

namespace platform
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
			LineBuilder( const bool stripErrors );
			void Append(ILogOutput& log, const char* prefix, const uint8* data, const uint32 dataSize);

		private:
			std::vector<uint8>	m_buffer;
			bool				m_stripErrors;
		};

		static DWORD WINAPI ThreadProc(LPVOID pData);
	};

	TaskIOReader::LineBuilder::LineBuilder( const bool stripErrors )
		: m_stripErrors( stripErrors )
	{
	}

	void TaskIOReader::LineBuilder::Append(ILogOutput& log, const char* prefix, const uint8* data, const uint32 dataSize)
	{
		for (uint32 i=0; i<dataSize; ++i)
		{
			const uint8 ch = data[i];
			if (ch == '\r')
			{
				if (!m_buffer.empty())
				{
					m_buffer.push_back(0);

					const char* line = (const char*)&m_buffer[0];

					if ( m_stripErrors )
					{
						const char* errPtr = strstr(line, "): ");
						if (errPtr)
						{
							*(char*)errPtr = 0;

							const char* fileName = strrchr(line, '\\' );
							if (fileName)
								line = fileName+1;

							*(char*)errPtr = ')';
						}

						const char* projectPtr = strrchr(line, '[');
						if (projectPtr)
						{
							*(char*)projectPtr = 0;
						}
					}

					log.Log( "%s: %s", prefix, line );
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

		CreatePipe( &m_hStdOutRead, &m_hStdOutWrite, &saAttr, 0);

		m_hThread = CreateThread( NULL, 64<<10, &ThreadProc, this, 0, NULL );
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
		TaskIOReader* log = (TaskIOReader*) pData;

		LineBuilder lineBuilder( log->m_stripErrors );

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
			if ( numToRead > sizeof(readBuffer) )
				numToRead = sizeof(readBuffer);

			if (!ReadFile(log->m_hStdOutRead, readBuffer, sizeof(readBuffer), &numRead, NULL))
				break;

			// append to line printer
			lineBuilder.Append(*log->m_log, log->m_prefix, readBuffer, numRead);
		}

		return 0;
	}

	//--------------------------------------------------------------------------

	ExternalAppEnvironment::ExternalAppEnvironment( class ILogOutput& log, const char* prefix, const bool stripErrors )
		: m_log( &log )
		, m_prefix( prefix )
	{
		m_output = new class TaskIOReader( log, prefix, stripErrors );
	}

	ExternalAppEnvironment::~ExternalAppEnvironment()
	{
		if ( m_output )
		{
			delete m_output;
			m_output = nullptr;
		}
	}

	void ExternalAppEnvironment::SetDirectory( const std::wstring& dir )
	{
	}

	int ExternalAppEnvironment::Execute( const std::wstring& command, const std::wstring& arguments )
	{
		PROCESS_INFORMATION pinfo;
		memset(&pinfo, 0, sizeof(pinfo));

		STARTUPINFOW startupInfo;
		memset(&startupInfo, 0, sizeof(startupInfo));
		startupInfo.cb = sizeof(startupInfo);
		startupInfo.hStdOutput = m_output->GetHandle();
		startupInfo.wShowWindow = SW_HIDE;
		startupInfo.hStdError = m_output->GetHandle();
		startupInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

		// short process name
		const wchar_t* processName = wcsrchr(command.c_str(), L'\\');
		if (!processName) processName = wcsrchr(command.c_str(), L'/');
		if (processName) processName += 1;
		m_log->Log("%hs: Executing '%ls'", m_prefix, processName);

		// get temp directory path
		const std::wstring tempPath = GetCurDirectoryPath();

		// prepare arguments
		std::wstring temp;
		temp += L"\"";
		temp += command;
		temp += L"\" ";
		temp += arguments;

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
			tempPath.c_str(),			// lpCurrentDirectory
			&startupInfo,			// lpStartupInfo
			&pinfo))
		{
			m_log->Error( "%hs: Failed to start '%ls'", m_prefix, processName );
			return -1;
		}

		// wait for finish
		while (1)
		{
			// update log output
			m_log->Flush();

			// wait for compilation task to end
			const int ret = WaitForSingleObject(pinfo.hProcess, 50);
			if (ret == WAIT_TIMEOUT)
				continue;

			// end
			if (ret == WAIT_OBJECT_0)
			{
				m_log->Flush();
				break;
			}

			// error
			m_log->Error( "%hs: Process '%ls' did not terminate successfully: handle lost: %d", m_prefix, processName, ret );
			CloseHandle(pinfo.hProcess);
			CloseHandle(pinfo.hThread);
			return -1;
		}

		// get exit code
		int exitCode = -1;
		GetExitCodeProcess(pinfo.hProcess, (DWORD*) &exitCode);
		if (exitCode != 0)
		{
			m_log->Error( "%hs: Process '%ls' terminated with exit code %d", m_prefix, processName, exitCode );
			Sleep(100);
			m_log->Flush();
			return exitCode;
		}

		// close local handles
		CloseHandle(pinfo.hProcess);
		CloseHandle(pinfo.hThread);

		// return error code
		return exitCode;
	}

} // platform