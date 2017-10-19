#include "build.h"
#include "dx11ShaderHLSLWriter.h"

CDX11ShaderHLSLWriter::CDX11ShaderHLSLWriter()
	: m_codeWritePtr( 0 )
	, m_indent( 0 )
{
	m_code[ m_codeWritePtr ] = 0;
}

CDX11ShaderHLSLWriter::~CDX11ShaderHLSLWriter()
{
}

void CDX11ShaderHLSLWriter::Clear()
{
	m_codeWritePtr = 0;
	m_code[ m_codeWritePtr ] = 0;
	m_indent = 0;
}

void CDX11ShaderHLSLWriter::Indent( const int delta )
{
	m_indent += delta;
	DEBUG_CHECK( m_indent >= 0 );
}

void CDX11ShaderHLSLWriter::Append( const char* txt )
{
	if ( txt )
	{
		const uint32 len = (const uint32) strlen(txt);
		if ( m_codeWritePtr + len < MAX_CODE )
		{
			memcpy( &m_code[ m_codeWritePtr ], txt, sizeof(char) * len );
			m_codeWritePtr += len;
			m_code[ m_codeWritePtr ] = 0;
		}
	}
}

void CDX11ShaderHLSLWriter::Appendf( const char* txt, ... )
{
	static const uint32 MAX_LEN = 8192;

	char buf[ MAX_LEN+1 ];

	va_list args;

	va_start( args, txt );
	size_t len = _vscprintf( txt, args );
	if ( len < MAX_LEN )
	{
		vsprintf_s( buf, ARRAYSIZE(buf), txt, args );
		Append( buf );
	}
	else
	{
		char* temp = (char*) malloc( len+1 );
		vsprintf_s( temp, len, txt, args );
		Append( temp );
		free( temp );
	}

	va_end( args );
}

const char* CDX11ShaderHLSLWriter::c_str() const
{
	DEBUG_CHECK( m_code[ m_codeWritePtr ] == 0 ); // make sure string is NULL terminated
	return m_code;
}