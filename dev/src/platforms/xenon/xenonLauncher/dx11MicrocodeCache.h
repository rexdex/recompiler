#pragma once

//----------------------------------------------------------------------------

/// Cache for decompiled microcode
class CDX11MicrocodeCache
{
public:
	CDX11MicrocodeCache();
	~CDX11MicrocodeCache();

	/// Set shader dump path
	void SetDumpPath( const std::wstring& absPath );

	/// Decompile pixel shader (can use cached version)
	CDX11MicrocodeShader* GetCachedPixelShader( const void* shaderCode, const uint32 shaderCodeSize );

	/// Decompile vertex shader (can use cached version)
	CDX11MicrocodeShader* GetCachedVertexShader( const void* shaderCode, const uint32 shaderCodeSize );

private:
	typedef uint64 TMicrocodeHash;
	typedef std::map< TMicrocodeHash, CDX11MicrocodeShader* >	TMicrocodeMap;

	static TMicrocodeHash ComputeHash( const void* shaderCode, const uint32 shaderCodeSize );

	TMicrocodeMap		m_cachedVertexShaders;
	TMicrocodeMap		m_cachedPixelShaders;

	std::wstring		m_dumpPath;
};