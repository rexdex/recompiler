#pragma once

/// Code writer for HLSL shader
class CDX11ShaderHLSLWriter
{
public:
	CDX11ShaderHLSLWriter();
	~CDX11ShaderHLSLWriter();

	void Clear();

	void Indent( const int delta );

	void Append( const char* txt );
	void Appendf( const char* txt, ... );

	const char* c_str() const;

private:
	static const uint32 MAX_CODE			= (64 * 1024) - 1;

	char		m_code[ MAX_CODE+1 ];
	uint32		m_codeWritePtr;

	int			m_indent;
};