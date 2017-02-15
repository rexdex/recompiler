#pragma

struct ProjectTraceCallHistory;
struct ProjectTraceAddressHistoryInfo;

/// HTML Helper
class CHTMLBuilder
{
private:
	wxString*					m_out;
	std::vector<const char*>	m_stack;
	bool						m_blockStart;

public:
	CHTMLBuilder(wxString& out)
		: m_out(&out)
		, m_blockStart(false)
	{}

	void Open(const char* block)
	{
		StartValue();

		*m_out += "<";
		*m_out += block;
		*m_out += " ";

		m_stack.push_back(block);
		m_blockStart = true;
	}

	void TableColumn( const char* name, int width )
	{
		Open( "th" );
		Attr( "width", "%d", width );
		Print( name );
		Close();
	}

	void Attr(const char* name, const char* valueBuf, ...)
	{
		if ( m_blockStart )
		{
			char buf[ 4096 ];
			va_list args;

			va_start(args,valueBuf);
			vsprintf_s(buf, valueBuf, args);
			va_end(args);
			
			*m_out += name;
			*m_out += "=";
			*m_out += buf;
			*m_out += " ";
		}
	}

	void Print(const char* valueBuf, ...)
	{
		char buf[ 4096 ];
		va_list args;

		va_start(args,valueBuf);
		vsprintf_s(buf, valueBuf, args);
		va_end(args);

		StartValue();

		*m_out += buf;
	}
	
	void PrintBlock(const char* blockName, const char* valueBuf, ...)
	{
		char buf[ 4096 ];
		va_list args;

		va_start(args,valueBuf);
		vsprintf_s(buf, valueBuf, args);
		va_end(args);

		Open(blockName);
		StartValue();
		*m_out += buf;
		Close();
	}

	void Close()
	{
		if ( !m_stack.empty() )
		{
			StartValue();

			const char* topName = m_stack.back();
			*m_out += "</";
			*m_out +=  topName;
			*m_out += ">";
			m_stack.pop_back();
		}
	}

private:
	void StartValue()
	{
		if ( m_blockStart )
		{
			*m_out += " >";
			m_blockStart = false;
		}
	}
};

namespace Parse
{
	extern bool ParseUrlKeyword(const char*& stream, const char* match);
	extern bool ParseUrlPart(const char*& stream, char* outData, const uint32 outDataMax );
	extern bool ParseUrlInteger(const char*& stream, int& outInteger);
	extern bool ParseUrlAddress(const char*& stream, uint32& outAddress);
	extern void PrintCallHistory( class ILogOutput& log, const uint32 level, const ProjectTraceCallHistory& history );
	extern void PrintValueHistory( class ILogOutput& log, const std::vector< ProjectTraceAddressHistoryInfo >& history );
} // Parse

//---------------------------------------------------------------------------

extern void ExtractDirW( const wchar_t* filePath, std::wstring& outDirectory );

//---------------------------------------------------------------------------
