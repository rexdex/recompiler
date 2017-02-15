#include "build.h"
#include "xenonGPUUtils.h"
#include "dx11Shader.h"
#include "dx11MicrocodeShader.h"
#include "dx11ShaderBuilder.h"
#include "dx11FetchLayout.h"

//---------------------------------------------------------------------------

CDX11VertexShader::CDX11VertexShader()
	: m_source( nullptr )
	, m_vertexShader( nullptr )
	, m_fetchLayout( nullptr )
{
}

CDX11VertexShader::~CDX11VertexShader()
{
	// release compiled vertex shader
	if ( m_vertexShader )
	{
		m_vertexShader->Release();
		m_vertexShader = nullptr;
	}

	// release compiled vertex format
	if ( m_fetchLayout )
	{
		delete m_fetchLayout;
		m_fetchLayout = nullptr;
	}

	// release input layout
	if ( m_vertexInputLayout )
	{
		m_vertexInputLayout->Release();
		m_vertexInputLayout = nullptr;
	}
}

CDX11VertexShader::DrawingContext::DrawingContext()
	: m_bTest( false )
	, m_psRegs( 0 )
	, m_vsRegs( 0 )
{
}

uint32 CDX11VertexShader::DrawingContext::GetHash() const
{
	uint32 ret = 0;
	ret |= (m_bTest ? 1 : 0) << 0;
	ret |= (m_psRegs & 0x3F) << 1;
	ret |= (m_vsRegs & 0x3F) << 7;
	return ret;
}

CDX11VertexShader* CDX11VertexShader::Compile( ID3D11Device* device, CDX11MicrocodeShader* sourceMicrocode, const DrawingContext& context, const std::wstring& dumpDir )
{
	DEBUG_CHECK( device != nullptr );

	// no source microcode
	if ( !sourceMicrocode )
		return nullptr;

	// microcode must be a vertex shader
	if ( sourceMicrocode->IsPixelShader() )
	{
		GLog.Err( "D3D: Cannot build vertex shader from microcode for pixel shader" );
		return nullptr;
	}

	// generate source code
	std::string code;
	CDX11ShaderHLSLGenerator codeGenerator;
	CDX11ShaderHLSLGenerator::Context codeContext;
	codeContext.m_numIncomingInputs = 0;
	if ( !codeGenerator.GenerateHLSL( *sourceMicrocode, codeContext, code ) )
	{
		GLog.Err( "D3D: Error converting microcode shader hash 0x%016llX to HLSL", sourceMicrocode->GetHash() );
		return nullptr;
	}

	// generate the input layout
	CDX11FetchLayout* fetchLayout = CDX11FetchLayout::ExtractFromMicrocode( sourceMicrocode );
	if ( !fetchLayout )
	{
		GLog.Err( "D3D: Error extracting vertex layout from microcode shader hash 0x%016llX", sourceMicrocode->GetHash() );
		return nullptr;
	}

	// dump generated code
	if ( !dumpDir.empty() )
	{
		wchar_t fileName[512];
		swprintf_s( fileName, ARRAYSIZE(fileName), L"%sshader_%08llX_%d_vertex_fx.txt", dumpDir.c_str(), sourceMicrocode->GetHash(), context.GetHash() );

		FILE* f = NULL;
		_wfopen_s( &f, fileName, L"w" );
		if ( f )
		{
			fwrite( code.c_str(), code.length(), 1, f );
			fclose(f);
		}
	}

	// compile the shader
	DWORD flags = 0;
	DWORD flags2 = 0;
	ID3DBlob* codeBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hRet = D3DCompile( code.c_str(), code.length(), "microcode", NULL, NULL, "main", "vs_5_0", flags, flags2, &codeBlob, &errorBlob );
	if ( FAILED( hRet ) )
	{
		// report errors
		const char* errorText = errorBlob ? (const char*) errorBlob->GetBufferPointer() : "";
		GLog.Err( "D3D: Failed to compile vertex shader: %hs", errorText );

		// dump compilation error
		if ( !dumpDir.empty() )
		{
			wchar_t fileName[512];
			swprintf_s( fileName, ARRAYSIZE(fileName), L"%sshader_%08llX_%d_vertex_fx_error.txt", dumpDir.c_str(), sourceMicrocode->GetHash(), context.GetHash() );

			FILE* f = NULL;
			_wfopen_s( &f, fileName, L"w" );
			if ( f )
			{
				fwrite( errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), 1, f );
				fclose(f);
			}
		}
	}

	// cleanup warning buffer
	if ( errorBlob )
	{
		errorBlob->Release();
		errorBlob = nullptr;
	}

	// no code generated
	if ( !codeBlob)
		return nullptr;

	// create vertex shader
	ID3D11VertexShader* vs = nullptr;
	hRet = device->CreateVertexShader( codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), NULL, &vs );
	if ( FAILED(hRet) )
	{
		GLog.Err( "D3D: Failed to create vertex shader, error 0x%08X", hRet );
		codeBlob->Release();
		return nullptr;
	}

	// decompile
	if ( !dumpDir.empty() )
	{
		ID3DBlob* dissassembledBlob = nullptr;
		D3DDisassemble( codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), 0, NULL, &dissassembledBlob );
		if ( dissassembledBlob )
		{
			wchar_t fileName[512];
			swprintf_s( fileName, ARRAYSIZE(fileName), L"%sshader_%08llX_%d_vertex_diss.txt", dumpDir.c_str(), sourceMicrocode->GetHash(), context.GetHash() );

			FILE* f = NULL;
			_wfopen_s( &f, fileName, L"w" );
			if ( f )
			{
				fwrite( dissassembledBlob->GetBufferPointer(), dissassembledBlob->GetBufferSize(), 1, f );
				fclose(f);
			}

			dissassembledBlob->Release();
		}
	}

	// create empty (no buffers) input layout descriptor for the IA stage
	ID3D11InputLayout* inputLayout = 0;
	D3D11_INPUT_ELEMENT_DESC desc;
	memset( &desc, 0, sizeof(desc) );
	hRet = device->CreateInputLayout( &desc, 0, codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), &inputLayout );
	if ( FAILED(hRet) || !inputLayout )
	{
		GLog.Err( "D3D: Failed to create vertex input layout, error 0x%08X", hRet );
		codeBlob->Release();
		vs->Release();
		return nullptr;
	}

	// code no longer needed
	codeBlob->Release();

	// return wrapper
	CDX11VertexShader* ret = new CDX11VertexShader();
	ret->m_source = sourceMicrocode;
	ret->m_fetchLayout = fetchLayout;
	ret->m_vertexInputLayout = inputLayout;
	ret->m_sourceCodeHLSL = code;
	ret->m_vertexShader = vs;
	return ret;
}

//---------------------------------------------------------------------------

CDX11PixelShader::CDX11PixelShader()
	: m_source( nullptr )
	, m_pixelShader( nullptr )
{
}

CDX11PixelShader::~CDX11PixelShader()
{
	// release compiled pixel shader
	if ( m_pixelShader )
	{
		m_pixelShader->Release();
		m_pixelShader = nullptr;
	}
}

CDX11PixelShader::DrawingContext::DrawingContext()
	: m_bSRGBWrite( false )
	, m_bAlphaTestEnabled( false )
	, m_psRegs( 0 )
	, m_vsRegs( 0 )
	, m_interp( 0 )
{
}

uint32 CDX11PixelShader::DrawingContext::GetHash() const
{
	uint32 ret = 0;
	ret |= (m_bAlphaTestEnabled ? 1 : 0) << 0;
	ret |= (m_bSRGBWrite ? 1 : 0) << 1;
	ret |= (m_psRegs & 0x3F) << 2;
	ret |= (m_vsRegs & 0x3F) << 8;
	ret |= (m_interp & 0xF) << 14;

	return ret;
}

CDX11PixelShader* CDX11PixelShader::Compile( ID3D11Device* device, CDX11MicrocodeShader* sourceMicrocode, const DrawingContext& context, const std::wstring& dumpDir )
{
	DEBUG_CHECK( device != nullptr );

	// no source microcode
	if ( !sourceMicrocode )
		return nullptr;

	// microcode must be a vertex shader
	if ( !sourceMicrocode->IsPixelShader() )
	{
		GLog.Err( "D3D: Cannot build pixel shader from microcode for vertex shader" );
		return nullptr;
	}

	// generate source code
	std::string code;
	CDX11ShaderHLSLGenerator codeGenerator;
	CDX11ShaderHLSLGenerator::Context codeContext;
	codeContext.m_numIncomingInputs = context.m_interp;
	if ( !codeGenerator.GenerateHLSL( *sourceMicrocode, codeContext, code ) )
	{
		GLog.Err( "D3D: Error converting microcode shader hash 0x%016llX to HLSL", sourceMicrocode->GetHash() );
		return nullptr;
	}

	// dump generated code
	if ( !dumpDir.empty() )
	{
		wchar_t fileName[512];
		swprintf_s( fileName, ARRAYSIZE(fileName), L"%sshader_%08llX_%d_pixel_fx.txt", dumpDir.c_str(), sourceMicrocode->GetHash(), context.GetHash() );

		FILE* f = NULL;
		_wfopen_s( &f, fileName, L"w" );
		if ( f )
		{
			fwrite( code.c_str(), code.length(), 1, f );
			fclose(f);
		}
	}

	// compile the shader
	DWORD flags = 0;
	DWORD flags2 = 0;
	ID3DBlob* codeBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hRet = D3DCompile( code.c_str(), code.length(), "microcode", NULL, NULL, "main", "ps_5_0", flags, flags2, &codeBlob, &errorBlob );
	if ( FAILED( hRet ) )
	{
		// report errors
		const char* errorText = errorBlob ? (const char*) errorBlob->GetBufferPointer() : "";
		GLog.Err( "D3D: Failed to compile pixel shader: %hs", errorText );

		// dump compilation error
		if ( !dumpDir.empty() )
		{
			wchar_t fileName[512];
			swprintf_s( fileName, ARRAYSIZE(fileName), L"%sshader_%08llX_%d_pixel_fx_error.txt", dumpDir.c_str(), sourceMicrocode->GetHash(), context.GetHash() );

			FILE* f = NULL;
			_wfopen_s( &f, fileName, L"w" );
			if ( f )
			{
				fwrite( errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), 1, f );
				fclose(f);
			}
		}
	}

	// cleanup warning buffer
	if ( errorBlob )
	{
		errorBlob->Release();
		errorBlob = nullptr;
	}

	// no code generated
	if ( !codeBlob)
		return nullptr;

	// create vertex shader
	ID3D11PixelShader* ps = nullptr;
	hRet = device->CreatePixelShader( codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), NULL, &ps );
	if ( FAILED(hRet) )
	{
		GLog.Err( "D3D: Failed to create pixel shader, error 0x%08X", hRet );
		codeBlob->Release();
		return nullptr;
	}


	// decompile
	if ( !dumpDir.empty() )
	{
		ID3DBlob* dissassembledBlob = nullptr;
		D3DDisassemble( codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), 0, NULL, &dissassembledBlob );
		if ( dissassembledBlob )
		{
			wchar_t fileName[512];
			swprintf_s( fileName, ARRAYSIZE(fileName), L"%sshader_%08llX_%d_pixel_diss.txt", dumpDir.c_str(), sourceMicrocode->GetHash(), context.GetHash() );

			FILE* f = NULL;
			_wfopen_s( &f, fileName, L"w" );
			if ( f )
			{
				fwrite( dissassembledBlob->GetBufferPointer(), dissassembledBlob->GetBufferSize(), 1, f );
				fclose(f);
			}

			dissassembledBlob->Release();
		}
	}

	// code no longer needed
	codeBlob->Release();

	// return wrapper
	CDX11PixelShader* ret = new CDX11PixelShader();
	ret->m_source = sourceMicrocode;
	ret->m_pixelShader = ps;
	ret->m_sourceCodeHLSL = code;
	return ret;
}

//---------------------------------------------------------------------------
