#include "build.h"
#include "xenonGPUTraceWriter.h"

CXenonGPUTraceWriter::CXenonGPUTraceWriter( FILE* f )
	: m_file( f )
	, m_indent( 0 )
{
}

CXenonGPUTraceWriter::~CXenonGPUTraceWriter()
{
	if ( m_file )
	{
		fclose( m_file );
		m_file = nullptr;
	}
}

CXenonGPUTraceWriter* CXenonGPUTraceWriter::Create(const std::wstring& path)
{
	// open file
	FILE* file = nullptr;
	_wfopen_s(&file, path.c_str(), L"w");
	if (!file)
		return nullptr;

	// create wrapper
	return new CXenonGPUTraceWriter(file);
}

void CXenonGPUTraceWriter::Indent( int delta )
{
	DEBUG_CHECK( m_indent >= 0 );
	m_indent += delta;
	DEBUG_CHECK( m_indent >= 0 );
}

void CXenonGPUTraceWriter::Write( const char* txt )
{
	if ( !m_file )
		return;

	for ( int32 i=0; i<m_indent; ++i )
		fprintf( m_file, "    " );

	fprintf( m_file, txt );
	fprintf( m_file, "\n" );
	fflush(m_file);
}

void CXenonGPUTraceWriter::Writef( const char* txt, ... )
{
	if ( !m_file )
		return;

	va_list args;
	va_start( args, txt );
	vsprintf_s( m_buffer, ARRAYSIZE(m_buffer), txt, args );
	va_end( args );

	for ( int32 i=0; i<m_indent; ++i )
		fprintf( m_file, "    " );

	fprintf( m_file, m_buffer );
	fprintf( m_file, "\n" );
	fflush(m_file);
}