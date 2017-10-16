#pragma once

/// Generate HLSL code for shader
class CDX11ShaderHLSLGenerator
{
public:
	CDX11ShaderHLSLGenerator();

	/// General generation context
	struct Context
	{
		uint32		m_numIncomingInputs;

		Context()
			: m_numIncomingInputs( 0 )
		{};
	};

	/// Generate shader code
	bool GenerateHLSL( const class CDX11MicrocodeShader& microcode, const Context& context, std::string& outCode );

private:
	void GeneratePixelShaderStructures( class CDX11ShaderHLSLWriter& writer, const Context& context, const class CDX11MicrocodeShader& microcode );
	void GenerateVertexShaderStructures( class CDX11ShaderHLSLWriter& writer, const Context& context, const class CDX11MicrocodeShader& microcode );
	void GenerateRegsStructure( class CDX11ShaderHLSLWriter& writer, const class CDX11MicrocodeShader& microcode );
};