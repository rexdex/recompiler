#pragma once

#include "dx11Shader.h"

/// Cache for shaders
class CDX11ShaderCache
{
public:
	CDX11ShaderCache( ID3D11Device* dev );
	~CDX11ShaderCache();

	/// Set shader dump path
	void SetDumpPath( const std::wstring& absPath );

	/// Get renderable vertex shader for given microcode and drawing context  (can return cached one)
	CDX11VertexShader* GetVertexShader( CDX11MicrocodeShader* sourceMicrocode, const CDX11VertexShader::DrawingContext& context );

	/// Get renderable pixel shader for given microcode and drawing context (can return cached one)
	CDX11PixelShader* GetPixelShader( CDX11MicrocodeShader* sourceMicrocode, const CDX11PixelShader::DrawingContext& context );

private:
	// cached device
	ID3D11Device*		m_device;

	// cache key
	struct Key
	{
		uint64 m_shaderHash;
		uint32 m_contextHash;

		inline Key( const uint64 shaderHash, const uint32 contextHash )
			: m_shaderHash( shaderHash )
			, m_contextHash( contextHash )
		{}

		inline Key( const Key& other )
			: m_shaderHash( other.m_shaderHash )
			, m_contextHash( other.m_contextHash )
		{}

		inline const bool operator==( const Key& other ) const
		{
			return (other.m_contextHash == m_contextHash) && (other.m_shaderHash == other.m_shaderHash);
		}

		inline const bool operator!=( const Key& other ) const
		{
			return (other.m_contextHash != m_contextHash) || (other.m_shaderHash != other.m_shaderHash);
		}

		inline const bool operator<( const Key& other ) const
		{
			if ( other.m_shaderHash < m_shaderHash )
				return true;
			if (other.m_shaderHash > m_shaderHash)
				return false;

			return other.m_contextHash < m_contextHash;
		}
	};
	
	// pixel/vertex caches
	std::map< Key, CDX11PixelShader* >		m_pixelShaders;
	std::map< Key, CDX11VertexShader* >		m_vertexShaders;

	// dump path for shaders
	std::wstring							m_dumpPath;
};