#include "build.h"
#include "xenonGPUUtils.h"
#include "dx11MicrocodeShader.h"
#include "dx11MicrocodeDecompiler.h"
#include "dx11MicrocodeCache.h"

CDX11MicrocodeCache::CDX11MicrocodeCache()
{
}

CDX11MicrocodeCache::~CDX11MicrocodeCache()
{
	for ( auto it : m_cachedVertexShaders )
		delete it.second;

	for ( auto it : m_cachedPixelShaders )
		delete it.second;
}

void CDX11MicrocodeCache::SetDumpPath( const std::wstring& absPath )
{
	m_dumpPath = absPath;
}

CDX11MicrocodeCache::TMicrocodeHash CDX11MicrocodeCache::ComputeHash( const void* shaderCode, const uint32 shaderCodeSize )
{
	return XenonGPUCalcCRC64( shaderCode, shaderCodeSize );
}

CDX11MicrocodeShader* CDX11MicrocodeCache::GetCachedPixelShader( const void* shaderCode, const uint32 shaderCodeSize )
{
	// invalid data
	if ( !shaderCode || !shaderCodeSize )
		return nullptr;

	// compute data hash
	const TMicrocodeHash hash = ComputeHash( shaderCode, shaderCodeSize );
	const auto it = m_cachedPixelShaders.find( hash );
	if ( it != m_cachedPixelShaders.end() )
		return it->second;

	// decompile
	CDX11MicrocodeShader* newShader = CDX11MicrocodeShader::ExtractPixelShader( shaderCode, shaderCodeSize, hash );
	if ( !newShader )
	{
		GLog.Err( "D3D: Failed to extract pixel shader from microcode hash 0x%016llX. Will not retry.", hash );
		m_cachedPixelShaders[ hash ] = nullptr;
		return nullptr;
	}

	// dump
	if ( !m_dumpPath.empty() )
	{
		// dump micorcode
		{
			wchar_t fileName[512];
			swprintf_s( fileName, ARRAYSIZE(fileName), L"%sshader_%08llX_%d_pixel.ucode", m_dumpPath.c_str(), hash, shaderCodeSize );

			FILE* f = NULL;
			_wfopen_s( &f, fileName, L"wb" );
			if ( f )
			{
				fwrite( shaderCode, shaderCodeSize, 1, f );
				fclose(f);
			}
		}

		// dump decompiled structure
		{
			wchar_t fileName[512];
			swprintf_s( fileName, ARRAYSIZE(fileName), L"%sshader_%08llX_%d_pixel.txt", m_dumpPath.c_str(), hash, shaderCodeSize ) ;
			newShader->DumpShader( fileName );
		}

		// dump original code
		{
			wchar_t fileName[512];
			swprintf_s( fileName, ARRAYSIZE(fileName), L"%sshader_%08llX_%d_pixel_raw.txt", m_dumpPath.c_str(), hash, shaderCodeSize ) ;

			FILE* f = NULL;
			_wfopen_s( &f, fileName, L"w" );
			if ( f )
			{
				class TextFileWriter : public CDX11MicrocodeDecompiler::ICodeWriter
				{
				public:
					TextFileWriter( FILE* f )
						: m_file( f ) 
					{};

					virtual void Append( const char* txt )
					{
						fprintf( m_file, txt );
					}

					virtual void Appendf( const char* txt, ... )
					{
						char buf[ 1024 ];
						va_list args;

						va_start( args, txt );
						vsprintf_s( buf, ARRAYSIZE(buf), txt, args );
						va_end( args );

						fprintf( m_file, buf );
					}

				private:
					FILE*		m_file;
				};

				CDX11MicrocodeDecompiler decompiler( XenonShaderType::ShaderPixel );
				decompiler.DisassembleShader( TextFileWriter( f ), (const uint32*) shaderCode, shaderCodeSize/4 );

				fclose(f);
			}
		}
	}

	// store in cache
	DEBUG_CHECK( newShader->GetHash() == hash );
	m_cachedPixelShaders[ hash ] = newShader;
	return newShader;
}

CDX11MicrocodeShader* CDX11MicrocodeCache::GetCachedVertexShader( const void* shaderCode, const uint32 shaderCodeSize )
{
	// invalid data
	if ( !shaderCode || !shaderCodeSize )
		return nullptr;

	// compute data hash
	const TMicrocodeHash hash = ComputeHash( shaderCode, shaderCodeSize );
	const auto it = m_cachedVertexShaders.find( hash );
	if ( it != m_cachedVertexShaders.end() )
		return it->second;

	// decompile
	CDX11MicrocodeShader* newShader = CDX11MicrocodeShader::ExtractVertexShader( shaderCode, shaderCodeSize, hash );
	if ( !newShader )
	{
		GLog.Err( "D3D: Failed to extract vertex shader from microcode hash 0x%016llX. Will not retry.", hash );
		m_cachedVertexShaders[ hash ] = nullptr;
		return nullptr;
	}

	// dump
	if ( !m_dumpPath.empty() )
	{
		// dump micorcode
		{
			wchar_t fileName[512];
			swprintf_s( fileName, ARRAYSIZE(fileName), L"%sshader_%08llX_%d_vertex.ucode", m_dumpPath.c_str(), hash, shaderCodeSize );

			FILE* f = NULL;
			_wfopen_s( &f, fileName, L"wb" );
			if ( f )
			{
				fwrite( shaderCode, shaderCodeSize, 1, f );
				fclose(f);
			}
		}

		// dump decompiled structure
		{
			wchar_t fileName[512];
			swprintf_s( fileName, ARRAYSIZE(fileName), L"%sshader_%08llX_%d_vertex.txt", m_dumpPath.c_str(), hash, shaderCodeSize ) ;
			newShader->DumpShader( fileName );
		}

		// dump original code
		{
			wchar_t fileName[512];
			swprintf_s( fileName, ARRAYSIZE(fileName), L"%sshader_%08llX_%d_vertex_raw.txt", m_dumpPath.c_str(), hash, shaderCodeSize ) ;

			FILE* f = NULL;
			_wfopen_s( &f, fileName, L"w" );
			if ( f )
			{
				class TextFileWriter : public CDX11MicrocodeDecompiler::ICodeWriter
				{
				public:
					TextFileWriter( FILE* f )
						: m_file( f ) 
					{};

					virtual void Append( const char* txt )
					{
						fprintf( m_file, txt );
					}

					virtual void Appendf( const char* txt, ... )
					{
						char buf[ 1024 ];
						va_list args;

						va_start( args, txt );
						vsprintf_s( buf, ARRAYSIZE(buf), txt, args );
						va_end( args );

						fprintf( m_file, buf );
					}

				private:
					FILE*		m_file;
				};

				CDX11MicrocodeDecompiler decompiler( XenonShaderType::ShaderVertex );
				decompiler.DisassembleShader( TextFileWriter( f ), (const uint32*) shaderCode, shaderCodeSize/4 );

				fclose(f);
			}
		}
	}

	// store in cache
	DEBUG_CHECK( newShader->GetHash() == hash );
	m_cachedVertexShaders[ hash ] = newShader;
	return newShader;
}
