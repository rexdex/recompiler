#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

class CDX11FetchLayout;
class CDX11MicrocodeShader;

/// Rendering vertex shader (includes the vertex format information)
class CDX11VertexShader
{
public:
	CDX11VertexShader();
	~CDX11VertexShader();

	// drawing context (may affect outcome of actual shader compilation)
	struct DrawingContext
	{
		bool	m_bTest;
		uint32	m_vsRegs:6;				// number of vertex shader regs
		uint32	m_psRegs:6;				// number of pixel shader regs

		DrawingContext();

		uint32 GetHash() const;
	};

	// Get fetch format
	inline const CDX11FetchLayout* GetFetchLayout() const { return m_fetchLayout; }

	// Get the debug source code
	inline const std::string& GetSourceCode() const { return m_sourceCodeHLSL; }

	// DX11 resources
	inline ID3D11InputLayout* GetBindableInputLayout() const { return m_vertexInputLayout; }
	inline ID3D11VertexShader* GetBindableShader() const { return m_vertexShader; }

	// Create vertex shader out of decompiled microcode	
	// NOTE: the decompiled microcode can be shared between multiple actual shaders
	static CDX11VertexShader* Compile( ID3D11Device* device, CDX11MicrocodeShader* sourceMicrocode, const DrawingContext& context, const std::wstring& dumpDir );

private:
	// source shader
	CDX11MicrocodeShader*		m_source;

	// compiled source code (debug)
	std::string					m_sourceCodeHLSL;

	// fetch format for this vertex shader
	CDX11FetchLayout*			m_fetchLayout;

	// compiled vertex shader
	ID3D11VertexShader*			m_vertexShader;
	ID3D11InputLayout*			m_vertexInputLayout;
};

/// Rendering pixel shader (includes the vertex format information)
class CDX11PixelShader
{
public:
	CDX11PixelShader();
	~CDX11PixelShader();

	// drawing context (may affect outcome of actual shader compilation)
	struct DrawingContext
	{
		bool	m_bSRGBWrite;			// srgb write to render targets
		bool	m_bAlphaTestEnabled;	// alpha test is enabled
		uint32	m_vsRegs:6;				// number of vertex shader regs
		uint32	m_psRegs:6;				// number of pixel shader regs
		uint32  m_interp:4;				// number of used vertex shader interpolators

		DrawingContext();

		uint32 GetHash() const;
	};

	// D3D resources
	inline ID3D11PixelShader* GetBindableShader() const { return m_pixelShader; }

	// Get list of texture slots to bind
	inline class CDX11MicrocodeShader* GetMicrocode() const { return m_source; }

	// Get the debug source code
	inline const std::string& GetSourceCode() const { return m_sourceCodeHLSL; }

	// Create pixel shader out of decompiled microcode	
	// NOTE: the decompiled microcode can be shared between multiple actual shaders
	static CDX11PixelShader* Compile( ID3D11Device* device, CDX11MicrocodeShader* sourceMicrocode, const DrawingContext& context, const std::wstring& dumpDir );

private:
	// source shader
	CDX11MicrocodeShader*		m_source;

	// compiled source code (debug)
	std::string					m_sourceCodeHLSL;

	// compiled pixel shader
	ID3D11PixelShader*			m_pixelShader;
};