#pragma once

/// Simple trace writer for GPU
class CXenonGPUTraceWriter
{
public:
	~CXenonGPUTraceWriter();

	/// Indentation
	void Indent( int delta );

	/// Write
	void Write( const char* txt );
	void Writef( const char* txt, ... );

	// create writer
	static CXenonGPUTraceWriter* Create(const std::wstring& path);

private:
	CXenonGPUTraceWriter(FILE* f);

	FILE*		m_file;
	int			m_indent;

	char		m_buffer[ 4096 ];
};