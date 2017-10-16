#include "build.h"
#include "dx11MicrocodeNodes.h"
#include "dx11MicrocodeHLSLWriter.h"

//#pragma optimize( "", off )

using namespace DX11Microcode;

//-----------------------------------------------------------------------------

static std::string PrintF( const char* txt, ... )
{
	char buf[ 8192 ];
	va_list args;

	va_start( args, txt );
	vsprintf_s( buf, ARRAYSIZE(buf), txt, args );
	va_end( args );

	return buf;
}

//-----------------------------------------------------------------------------

ExprNode::ExprNode()
	: m_refs( 1 )
{
	memset( &m_children, 0, sizeof(m_children) );
}

ExprNode::ExprNode( ExprNode* a )
	: m_refs( 1 )
{
	memset( &m_children, 0, sizeof(m_children) );
	m_children[0] = a->CopyWithType<ExprNode>();
}

ExprNode::ExprNode( ExprNode* a, ExprNode* b )
	: m_refs( 1 )
{
	memset( &m_children, 0, sizeof(m_children) );
	m_children[0] = a->CopyWithType<ExprNode>();
	m_children[1] = b->CopyWithType<ExprNode>();
}

ExprNode::ExprNode( ExprNode* a, ExprNode* b, ExprNode* c )
	: m_refs( 1 )
{
	memset( &m_children, 0, sizeof(m_children) );
	m_children[0] = a->CopyWithType<ExprNode>();
	m_children[1] = b->CopyWithType<ExprNode>();
	m_children[2] = c->CopyWithType<ExprNode>();
}

ICodeExpr* ExprNode::Copy()
{
	m_refs += 1;
	return this;
}

void ExprNode::Release()
{
	if ( 0 == --m_refs )
	{
		delete this;
	}
}

ExprNode::~ExprNode()
{
	DEBUG_CHECK( m_refs == 0 );

	for ( int32 i=0; i<ARRAYSIZE(m_children); ++i )
	{
		if ( m_children[i] )
			m_children[i]->Release();
	}
}

void ExprNode::Visit( IVisitor& v ) const
{
	v.OnExprStart( this );

	for ( int32 i=0; i<ARRAYSIZE(m_children); ++i )
	{
		if ( m_children[i] )
			m_children[i]->Visit( v );
	}

	v.OnExprEnd( this );
}

//-----------------------------------------------------------------------------

ExprReadReg::ExprReadReg( int regIndex )
	: ExprNode()
	, m_regIndex( regIndex )
{
}

std::string ExprReadReg::ToString() const
{
	return PrintF( "R%d", m_regIndex );
}

CodeChunk ExprReadReg::EmitHLSL( IHLSLWriter& writer ) const
{
	return writer.GetReg( m_regIndex );
}

//-----------------------------------------------------------------------------

ExprWriteReg::ExprWriteReg( int regIndex )
	: ExprNode()
	, m_regIndex( regIndex )
{
}

std::string ExprWriteReg::ToString() const
{
	return PrintF( "&R%d", m_regIndex ); // by convention the L-value regs are prefixed with &
}

CodeChunk ExprWriteReg::EmitHLSL( IHLSLWriter& writer ) const
{
	return writer.GetReg( m_regIndex );
}

//-----------------------------------------------------------------------------

ExprWriteExportReg::ExprWriteExportReg( EExportReg exportReg )
	: ExprNode()
	, m_exporReg( exportReg )
{
}

const char* ExprWriteExportReg::GetExporRegName( const EExportReg reg )
{
	switch ( reg )
	{
		case ExprWriteExportReg::EExportReg::POSITION: return "out_POSITION";
		case ExprWriteExportReg::EExportReg::POINTSIZE: return "out_POINTSIZE";

		case ExprWriteExportReg::EExportReg::COLOR0: return "out_COLOR0";
		case ExprWriteExportReg::EExportReg::COLOR1: return "out_COLOR1";
		case ExprWriteExportReg::EExportReg::COLOR2: return "out_COLOR2";
		case ExprWriteExportReg::EExportReg::COLOR3: return "out_COLOR3";

		case ExprWriteExportReg::EExportReg::INTERP0: return "out_INTERP0";
		case ExprWriteExportReg::EExportReg::INTERP1: return "out_INTERP1";
		case ExprWriteExportReg::EExportReg::INTERP2: return "out_INTERP2";
		case ExprWriteExportReg::EExportReg::INTERP3: return "out_INTERP3";
		case ExprWriteExportReg::EExportReg::INTERP4: return "out_INTERP4";
		case ExprWriteExportReg::EExportReg::INTERP5: return "out_INTERP5";
		case ExprWriteExportReg::EExportReg::INTERP6: return "out_INTERP6";
		case ExprWriteExportReg::EExportReg::INTERP7: return "out_INTERP7";
	}

	return "<unknown>";
}

const char* ExprWriteExportReg::GetExporSemanticName( const EExportReg reg )
{
	switch ( reg )
	{
		case ExprWriteExportReg::EExportReg::POSITION: return "SV_Position";
		case ExprWriteExportReg::EExportReg::POINTSIZE: return "PSIZE";

		case ExprWriteExportReg::EExportReg::COLOR0: return "SV_Target0";
		case ExprWriteExportReg::EExportReg::COLOR1: return "SV_Target1";
		case ExprWriteExportReg::EExportReg::COLOR2: return "SV_Target2";
		case ExprWriteExportReg::EExportReg::COLOR3: return "SV_Target3";

		case ExprWriteExportReg::EExportReg::INTERP0: return "TEXCOORD0";
		case ExprWriteExportReg::EExportReg::INTERP1: return "TEXCOORD1";
		case ExprWriteExportReg::EExportReg::INTERP2: return "TEXCOORD2";
		case ExprWriteExportReg::EExportReg::INTERP3: return "TEXCOORD3";
		case ExprWriteExportReg::EExportReg::INTERP4: return "TEXCOORD4";
		case ExprWriteExportReg::EExportReg::INTERP5: return "TEXCOORD5";
		case ExprWriteExportReg::EExportReg::INTERP6: return "TEXCOORD6";
		case ExprWriteExportReg::EExportReg::INTERP7: return "TEXCOORD7";
	}

	return "<unknown>";
}

int ExprWriteExportReg::GetExportSemanticIndex( const EExportReg reg )
{
	switch ( reg )
	{
		case ExprWriteExportReg::EExportReg::POSITION: return 0;
		case ExprWriteExportReg::EExportReg::POINTSIZE: return 1;

		case ExprWriteExportReg::EExportReg::COLOR0: return 2;
		case ExprWriteExportReg::EExportReg::COLOR1: return 3;
		case ExprWriteExportReg::EExportReg::COLOR2: return 4;
		case ExprWriteExportReg::EExportReg::COLOR3: return 5;

		case ExprWriteExportReg::EExportReg::INTERP0: return 6;
		case ExprWriteExportReg::EExportReg::INTERP1: return 7;
		case ExprWriteExportReg::EExportReg::INTERP2: return 8;
		case ExprWriteExportReg::EExportReg::INTERP3: return 9;
		case ExprWriteExportReg::EExportReg::INTERP4: return 10;
		case ExprWriteExportReg::EExportReg::INTERP5: return 11;
		case ExprWriteExportReg::EExportReg::INTERP6: return 12;
		case ExprWriteExportReg::EExportReg::INTERP7: return 13;
	}

	return 100;
}

int ExprWriteExportReg::GetExportInterpolatorIndex( const EExportReg reg )
{
	switch ( reg )
	{
		case ExprWriteExportReg::EExportReg::INTERP0: return 0; 
		case ExprWriteExportReg::EExportReg::INTERP1: return 1;
		case ExprWriteExportReg::EExportReg::INTERP2: return 2;
		case ExprWriteExportReg::EExportReg::INTERP3: return 3;
		case ExprWriteExportReg::EExportReg::INTERP4: return 4;
		case ExprWriteExportReg::EExportReg::INTERP5: return 5;
		case ExprWriteExportReg::EExportReg::INTERP6: return 6;
		case ExprWriteExportReg::EExportReg::INTERP7: return 7;
	}

	return -1;
}

std::string ExprWriteExportReg::ToString() const
{
	return GetExporRegName( m_exporReg );
}

CodeChunk ExprWriteExportReg::EmitHLSL( IHLSLWriter& writer ) const
{
	return writer.GetExportDest( m_exporReg );
}

//-----------------------------------------------------------------------------

ExprBoolConst::ExprBoolConst( bool pixelShader, int index )
	: ExprNode()
	, m_pixelShader( pixelShader )
	, m_index( index )
{
}

std::string ExprBoolConst::ToString() const
{
	return PrintF( "%sConsts.Bools[%d]", m_pixelShader ? "PS" : "VS", m_index );
}

CodeChunk ExprBoolConst::EmitHLSL( IHLSLWriter& writer ) const
{
	return writer.GetBoolVal( m_index );
}

//-----------------------------------------------------------------------------

ExprFloatConst::ExprFloatConst( bool pixelShader, int index )
	: ExprNode()
	, m_pixelShader( pixelShader )
	, m_index( index )
{
}

std::string ExprFloatConst::ToString() const
{
	return PrintF( "%sConsts.Floats[%d]", m_pixelShader ? "PS" : "VS", m_index );
}

CodeChunk ExprFloatConst::EmitHLSL( IHLSLWriter& writer ) const
{
	return writer.GetFloatVal( m_index );
}

//-----------------------------------------------------------------------------

ExprFloatRelativeConst::ExprFloatRelativeConst( bool pixelShader, int relativeOffset )
	: ExprNode()
	, m_pixelShader( pixelShader )
	, m_relativeOffset( relativeOffset )
{
}

std::string ExprFloatRelativeConst::ToString() const
{
	if ( m_relativeOffset )
		return PrintF( "%sConsts.Floats[a0 + %d]", m_pixelShader ? "PS" : "VS", m_relativeOffset );
	else
		return PrintF( "%sConsts.Floats[a0]", m_pixelShader ? "PS" : "VS" );
}

CodeChunk ExprFloatRelativeConst::EmitHLSL( IHLSLWriter& writer ) const
{
	return writer.GetFloatValRelative( m_relativeOffset );
}

//-----------------------------------------------------------------------------

ExprGetPredicate::ExprGetPredicate()
	: ExprNode()
{
}

std::string ExprGetPredicate::ToString() const
{
	return "__pred";
}

CodeChunk ExprGetPredicate::EmitHLSL( IHLSLWriter& writer ) const
{
	return writer.GetPredicate();
}

//-----------------------------------------------------------------------------

ExprAbs::ExprAbs( ExprNode* expr )
	: ExprNode( expr )
{
	DEBUG_CHECK( expr != nullptr )
}

std::string ExprAbs::ToString() const
{
	std::string ret = "abs(";
	ret += m_children[0]->ToString();
	ret += ")";
	return ret;
}

CodeChunk ExprAbs::EmitHLSL( IHLSLWriter& writer ) const
{
	CodeChunk ret;
	ret.Append( "abs(" );
	ret.Append( m_children[0]->EmitHLSL(writer) );
	ret.Append( ")" );
	return ret;
}

//-----------------------------------------------------------------------------

ExprNegate::ExprNegate( ExprNode* expr )
	: ExprNode( expr )
{
	DEBUG_CHECK( expr != nullptr )
}

std::string ExprNegate::ToString() const
{
	std::string ret = "-(";
	ret += m_children[0]->ToString();
	ret += ")";
	return ret;
}

CodeChunk ExprNegate::EmitHLSL( IHLSLWriter& writer ) const
{
	CodeChunk ret;
	ret.Append( "(-(" );
	ret.Append( m_children[0]->EmitHLSL(writer) );
	ret.Append( "))" );
	return ret;
}

//-----------------------------------------------------------------------------

ExprNot::ExprNot( ExprNode* expr )
	: ExprNode( expr )
{
	DEBUG_CHECK( expr != nullptr )
}

std::string ExprNot::ToString() const
{
	std::string ret = "!(";
	ret += m_children[0]->ToString();
	ret += ")";
	return ret;
}

CodeChunk ExprNot::EmitHLSL( IHLSLWriter& writer ) const
{
	CodeChunk ret;
	ret.Append( "(!(" );
	ret.Append( m_children[0]->EmitHLSL(writer) );
	ret.Append( "))" );
	return ret;
}

//-----------------------------------------------------------------------------

ExprSaturate::ExprSaturate( ExprNode* expr )
	: ExprNode( expr )
{
	DEBUG_CHECK( expr != nullptr )
}

std::string ExprSaturate::ToString() const
{
	std::string ret = "saturate(";
	ret += m_children[0]->ToString();
	ret += ")";
	return ret;
}

CodeChunk ExprSaturate::EmitHLSL( IHLSLWriter& writer ) const
{
	CodeChunk ret;
	ret.Append( "saturate(" );
	ret.Append( m_children[0]->EmitHLSL(writer) );
	ret.Append( ")" );
	return ret;
}

//-----------------------------------------------------------------------------

ExprReadSwizzle::ExprReadSwizzle( ExprNode* expr, ESwizzle x, ESwizzle y, ESwizzle z, ESwizzle w )
	: ExprNode( expr )
{
	DEBUG_CHECK( expr != nullptr )
	m_swizzles[0] = x;
	m_swizzles[1] = y;
	m_swizzles[2] = z;
	m_swizzles[3] = w;
}

std::string ExprReadSwizzle::ToString() const
{
	std::string ret = "(";
	ret += m_children[0]->ToString();
	ret += ").";

	const char* swizzles[] = {"x", "y", "z", "w"};
	ret += swizzles[ (uint8)m_swizzles[0] ];
	ret += swizzles[ (uint8)m_swizzles[1] ];
	ret += swizzles[ (uint8)m_swizzles[2] ];
	ret += swizzles[ (uint8)m_swizzles[3] ];

	return ret;
}

CodeChunk ExprReadSwizzle::EmitHLSL( IHLSLWriter& writer ) const
{
	CodeChunk ret;
	ret.Append( "((" );
	ret.Append( m_children[0]->EmitHLSL( writer ) );
	ret.Append( ")." );

	const char* swizzles[] = {"x", "y", "z", "w"};
	ret.Append( swizzles[ (uint8)m_swizzles[0] ] );
	ret.Append( swizzles[ (uint8)m_swizzles[1] ] );
	ret.Append( swizzles[ (uint8)m_swizzles[2] ] );
	ret.Append( swizzles[ (uint8)m_swizzles[3] ] );
	ret.Append( ")" );

	return ret;
}

//-----------------------------------------------------------------------------

ExprVertexFetch::ExprVertexFetch( ExprNode* src, uint32 fetchSlot, uint32 fetchOffset, uint32 fetchStride, EFetchFormat format, const bool isFloat, const bool isSigned, const bool isNormalized )
	: ExprNode( src )
	, m_format( format )
	, m_fetchSlot( fetchSlot )
	, m_fetchOffset( fetchOffset )
	, m_fetchStride( fetchStride )
	, m_isFloat( isFloat )
	, m_isSigned( isSigned )
	, m_isNormalized( isNormalized )
{
}

const char* ExprVertexFetch::GetFormatName( const EFetchFormat format )
{
	switch ( format )
	{
		case EFetchFormat::FMT_1_REVERSE: return "FMT_1_REVERSE";
		case EFetchFormat::FMT_8: return "FMT_8";
		case EFetchFormat::FMT_8_8_8_8: return "FMT_8_8_8_8";
		case EFetchFormat::FMT_2_10_10_10: return "FMT_2_10_10_10";
		case EFetchFormat::FMT_8_8: return "FMT_8_8";
		case EFetchFormat::FMT_16: return "FMT_16";
		case EFetchFormat::FMT_16_16: return "FMT_16_16";
		case EFetchFormat::FMT_16_16_16_16: return "FMT_16_16_16_16";
		case EFetchFormat::FMT_32: return "FMT_32";
		case EFetchFormat::FMT_32_32: return "FMT_32_32";
		case EFetchFormat::FMT_32_32_32_32: return "FMT_32_32_32_32";
		case EFetchFormat::FMT_32_FLOAT: return "FMT_32_FLOAT";
		case EFetchFormat::FMT_32_32_FLOAT: return "FMT_32_32_FLOAT";
		case EFetchFormat::FMT_32_32_32_32_FLOAT: return "FMT_32_32_32_32_FLOAT";
		case EFetchFormat::FMT_32_32_32_FLOAT: return "FMT_32_32_32_FLOAT";
	}

	return "unknown";
}

std::string ExprVertexFetch::ToString() const
{
	std::string ret = "VFETCH( ";

	ret += m_children[0]->ToString();
	ret += ",";

	ret += GetFormatName( m_format );
	ret += PrintF(", slot=%d, offset=%d, stride=%d, float=%d, signed=%d, normalized=%d )", 
		m_fetchSlot, m_fetchOffset, m_fetchStride,
		m_isFloat, m_isSigned, m_isNormalized );

	return ret;
}

CodeChunk ExprVertexFetch::EmitHLSL( IHLSLWriter& writer ) const
{
	CodeChunk src = m_children[0]->EmitHLSL( writer );
	CodeChunk data = writer.FetchVertex( src, *this );
	return data;
}

//-----------------------------------------------------------------------------

ExprTextureFetch::ExprTextureFetch( ExprNode* src, uint32 fetchSlot, TextureType textureType )
	: ExprNode( src )
	, m_fetchSlot( fetchSlot )
	, m_textureType( textureType )
{
}

std::string ExprTextureFetch::ToString() const
{
	std::string ret = "TFETCH( ";

	switch ( m_textureType )
	{
		case ExprTextureFetch::TextureType::Type1D: ret += "1D, ";
		case ExprTextureFetch::TextureType::Type2D: ret += "2D, ";
		case ExprTextureFetch::TextureType::Type2DArray: ret += "2DArray, ";
		case ExprTextureFetch::TextureType::Type3D: ret += "3D, ";
		case ExprTextureFetch::TextureType::TypeCube: ret += "CUBE, ";
	}

	ret += m_children[0]->ToString();
	ret += ",";

	ret += PrintF(", slot=%d )", 
		m_fetchSlot );

	return ret;
}

CodeChunk ExprTextureFetch::EmitHLSL( IHLSLWriter& writer ) const
{
	CodeChunk src = m_children[0]->EmitHLSL( writer );
	CodeChunk data = writer.FetchTexture( src, *this );
	return data;
}

//-----------------------------------------------------------------------------

ExprVectorFunc1::ExprVectorFunc1( const char* name, ExprNode* a )
	: ExprNode( a )
	, m_funcName( name )
{
	DEBUG_CHECK( a != nullptr );
}

std::string ExprVectorFunc1::ToString() const
{
	std::string ret;

	ret += m_funcName;
	ret += "(";
	ret += m_children[0]->ToString();
	ret += ")";

	return ret;
}

CodeChunk ExprVectorFunc1::EmitHLSL( IHLSLWriter& writer ) const
{
	CodeChunk ret;

	ret.Append( m_funcName );
	ret.Append( "(regs, " );
	ret.Append( m_children[0]->EmitHLSL( writer ) );
	ret.Append( ")" );

	return ret;
}

//-----------------------------------------------------------------------------

ExprVectorFunc2::ExprVectorFunc2( const char* name, ExprNode* a, ExprNode* b )
	: ExprNode( a, b )
	, m_funcName( name )
{
	DEBUG_CHECK( a != nullptr );
	DEBUG_CHECK( b != nullptr );
}

std::string ExprVectorFunc2::ToString() const
{
	std::string ret;

	ret += m_funcName;
	ret += "(regs, ";
	ret += m_children[0]->ToString();
	ret += ",";
	ret += m_children[1]->ToString();
	ret += ")";

	return ret;
}

CodeChunk ExprVectorFunc2::EmitHLSL( IHLSLWriter& writer ) const
{
	CodeChunk ret;

	ret.Append( m_funcName );
	ret.Append( "(regs, (" );
	ret.Append( m_children[0]->EmitHLSL( writer ) );
	ret.Append( "),(" );
	ret.Append( m_children[1]->EmitHLSL( writer ) );
	ret.Append( "))" );

	return ret;
}

//-----------------------------------------------------------------------------

ExprVectorFunc3::ExprVectorFunc3( const char* name, ExprNode* a, ExprNode* b, ExprNode* c )
	: ExprNode( a, b, c )
	, m_funcName( name )
{
	DEBUG_CHECK( a != nullptr );
	DEBUG_CHECK( b != nullptr );
	DEBUG_CHECK( c != nullptr );
}

std::string ExprVectorFunc3::ToString() const
{
	std::string ret;

	ret += m_funcName;
	ret += "(";
	ret += m_children[0]->ToString();
	ret += ",";
	ret += m_children[1]->ToString();
	ret += ",";
	ret += m_children[2]->ToString();
	ret += ")";

	return ret;
}

CodeChunk ExprVectorFunc3::EmitHLSL( IHLSLWriter& writer ) const
{
	CodeChunk ret;

	ret.Append( m_funcName );
	ret.Append( "(regs, (" );
	ret.Append( m_children[0]->EmitHLSL( writer ) );
	ret.Append( "),(" );
	ret.Append( m_children[1]->EmitHLSL( writer ) );
	ret.Append( "),(" );
	ret.Append( m_children[2]->EmitHLSL( writer ) );
	ret.Append( "))" );

	return ret;
}


//-----------------------------------------------------------------------------

ExprScalarFunc1::ExprScalarFunc1( const char* name, ExprNode* a )
	: ExprNode( a )
	, m_funcName( name )
{
	DEBUG_CHECK( a != nullptr );
}

std::string ExprScalarFunc1::ToString() const
{
	std::string ret;

	ret += m_funcName;
	ret += "(";
	ret += m_children[0]->ToString();
	ret += ")";

	return ret;
}

CodeChunk ExprScalarFunc1::EmitHLSL( IHLSLWriter& writer ) const
{
	CodeChunk ret;

	ret.Append( m_funcName );
	ret.Append( "(regs," );
	ret.Append( m_children[0]->EmitHLSL( writer ) );
	ret.Append( ")" );

	return ret;
}

//-----------------------------------------------------------------------------

ExprScalarFunc2::ExprScalarFunc2( const char* name, ExprNode* a, ExprNode* b )
	: ExprNode( a, b )
	, m_funcName( name )
{
	DEBUG_CHECK( a != nullptr );
	DEBUG_CHECK( b != nullptr );
}

std::string ExprScalarFunc2::ToString() const
{
	std::string ret;

	ret += m_funcName;
	ret += "(";
	ret += m_children[0]->ToString();
	ret += ",";
	ret += m_children[1]->ToString();
	ret += ")";

	return ret;
}

CodeChunk ExprScalarFunc2::EmitHLSL( IHLSLWriter& writer ) const
{
	CodeChunk ret;

	ret.Append( m_funcName );
	ret.Append( "(regs, (" );
	ret.Append( m_children[0]->EmitHLSL( writer ) );
	ret.Append( "),(" );
	ret.Append( m_children[1]->EmitHLSL( writer ) );
	ret.Append( "))" );

	return ret;
}

//-----------------------------------------------------------------------------

Statement::Statement()
	: m_refs( 1 )
{
}

Statement::~Statement()
{
	DEBUG_CHECK( m_refs == 0 );
}

ICodeStatement* Statement::Copy()
{
	m_refs += 1;
	return this;
}

void Statement::Release()
{
	if ( 0 == --m_refs )
		delete this;
}

//-----------------------------------------------------------------------------

ListStatement::ListStatement( Statement* a, Statement* b )
	: Statement()
{
	DEBUG_CHECK( a != nullptr );
	DEBUG_CHECK( b != nullptr );
	m_a = a->CopyWithType<Statement>();
	m_b = b->CopyWithType<Statement>();
}

std::string ListStatement::ToString() const
{
	std::string ret;
	
	ret += m_a->ToString();
	if ( ret.back() != '\n' )
		ret += "\n";

	ret += m_b->ToString();

	return ret;
}

ListStatement::~ListStatement()
{
	if ( m_a )
	{
		m_a->Release();
		m_a = nullptr;
	}

	if ( m_b )
	{
		m_b->Release();
		m_b = nullptr;
	}
}

void ListStatement::Visit( IVisitor& visitor ) const
{
	m_a->Visit( visitor );
	m_b->Visit( visitor );
}

void ListStatement::EmitHLSL( IHLSLWriter& writer ) const
{
	m_a->EmitHLSL( writer );
	m_b->EmitHLSL( writer );
}


//-----------------------------------------------------------------------------

SetPredicateStatement::SetPredicateStatement( ExprNode* expr )
	: Statement()
{
	DEBUG_CHECK( expr != nullptr );
	m_value = expr->CopyWithType<ExprNode>();
}

SetPredicateStatement::~SetPredicateStatement()
{
	m_value->Release();
}

void SetPredicateStatement::Visit( IVisitor& visitor ) const
{
}

std::string SetPredicateStatement::ToString() const
{
	std::string ret = "__pred = ";
	ret += m_value->ToString();
	ret += ";";
	return ret;
}

void SetPredicateStatement::EmitHLSL( IHLSLWriter& writer ) const
{
	CodeChunk val = m_value->EmitHLSL( writer );
	writer.SetPredicate( val );
}

//-----------------------------------------------------------------------------

ConditionalStatement::ConditionalStatement( Statement* a, ExprNode* condition )
	: Statement()
{
	DEBUG_CHECK( a != nullptr );
	DEBUG_CHECK( condition != nullptr );
	m_statement = a->CopyWithType<Statement>();
	m_condition = condition->CopyWithType<ExprNode>();
}

ConditionalStatement::~ConditionalStatement()
{
	if ( m_statement )
	{
		m_statement->Release();
		m_statement = nullptr;
	}

	if ( m_condition )
	{
		m_condition->Release();
		m_condition = nullptr;
	}
}

std::string ConditionalStatement::ToString() const
{
	std::string ret;
	
	ret += "if (";
	ret += m_condition->ToString();
	ret += ")\n{";
	ret += m_statement->ToString();
	ret += "}\n";

	return ret;
}

void ConditionalStatement::Visit( IVisitor& visitor ) const
{
	visitor.OnConditionPush( m_condition );
	m_statement->Visit( visitor );
	visitor.OnConditionPop();
}

void ConditionalStatement::EmitHLSL( IHLSLWriter& writer ) const
{
	if ( m_condition )
	{
		CodeChunk cond = writer.AllocLocalBool( m_condition->EmitHLSL( writer ) );
		writer.BeingCondition( cond );
	}

	m_statement->EmitHLSL( writer );

	if ( m_condition )
	{
		writer.EndCondition();
	}
}

//-----------------------------------------------------------------------------

WriteWithMaskStatement::WriteWithMaskStatement( ExprNode* target, ExprNode* source, ESwizzle x, ESwizzle y, ESwizzle z, ESwizzle w )
	: Statement()
	, m_target( target )
	, m_source( source )
{
	DEBUG_CHECK( (uint8)x < 8 );
	DEBUG_CHECK( (uint8)y < 8 );
	DEBUG_CHECK( (uint8)z < 8 );
	DEBUG_CHECK( (uint8)w < 8 );
	
	DEBUG_CHECK( m_target != nullptr );
	DEBUG_CHECK( m_source != nullptr );
	m_target = target->CopyWithType<ExprNode>();
	m_source = source->CopyWithType<ExprNode>();
	m_mask[0] = x;
	m_mask[1] = y;
	m_mask[2] = z;
	m_mask[3] = w;
}

WriteWithMaskStatement::~WriteWithMaskStatement()
{
}

std::string WriteWithMaskStatement::ToString() const
{
	std::string ret;

	ret = m_target->ToString();
	ret += ".";

	const char* masks[] = { "x", "y", "z", "w", "0", "1", "?", "_" };
	ret += masks[ (int)m_mask[0] ];
	ret += masks[ (int)m_mask[1] ];
	ret += masks[ (int)m_mask[2] ];
	ret += masks[ (int)m_mask[3] ];

	ret += " = ";

	ret += m_source->ToString();
	ret += ";";

	return ret;
}

void WriteWithMaskStatement::Visit( IVisitor& visitor ) const
{
	visitor.OnWrite( m_target, m_source, m_mask );
}

void WriteWithMaskStatement::EmitHLSL( IHLSLWriter& writer ) const
{
	// evaluate destination register
	CodeChunk dest = m_target->EmitHLSL( writer );

	// do we have any values copied from source ?
	ESwizzle sourceSwizzles[ 4 ];
	ESwizzle destMask[ 4 ];
	uint32 numSourceSwizzles = 0;
	uint32 numSpecialChannels = 0;
	for ( uint32 i=0; i<ARRAYSIZE(m_mask); ++i )
	{
		if ( m_mask[i] == ESwizzle::X || m_mask[i] == ESwizzle::Y || m_mask[i] == ESwizzle::Z || m_mask[i] == ESwizzle::W )
		{
			sourceSwizzles[ numSourceSwizzles ] = m_mask[i];
			destMask[ numSourceSwizzles ] = (ESwizzle)i;
			numSourceSwizzles += 1;
		}
		else if ( m_mask[i] == ESwizzle::ZERO || m_mask[i] == ESwizzle::ONE )
		{
			numSpecialChannels += 1;
		}
		else if ( m_mask[i] == ESwizzle::NOTUSED )
		{
			// neither emitted or set to 0/1
		}
		else
		{
			DEBUG_CHECK( !"Unknown swizzle type" );
		}
	}

	// nothing to output
	if (numSpecialChannels == 0 && numSourceSwizzles == 0)
	{
		// we still need to evaluate the source
		CodeChunk src = m_source->EmitHLSL(writer);
		writer.Emit(src);
		return;
	}

	// evaluate source and copy the (masked) channels
	if ( numSourceSwizzles > 0 )
	{
		CodeChunk src = m_source->EmitHLSL( writer );

		// HLSL compiler bug workaround: wrap source with float4( ). structure with repeated swizzles
		CodeChunk swizzledSrc( "float4(" );
		swizzledSrc.Append( src );
		swizzledSrc.Append( ")." );

		// prepare the swizzle
		CodeChunk swizzledDest = dest;
		swizzledDest.Append( "." );
		for ( uint32 i=0; i<numSourceSwizzles; ++i )
		{
			const char* masks[] = { "x", "y", "z", "w" };
			DEBUG_CHECK( (uint32)sourceSwizzles[i] < ARRAYSIZE(masks) );
			swizzledDest.Appendf( masks[ (uint32)destMask[i] ] );

			// HLSL compiler bug workaround: use the same swizzles on source
			swizzledSrc.Appendf( masks[ (uint32)sourceSwizzles[i] ] );
		}

		// emit assignment
		writer.Assign( swizzledDest, swizzledSrc );
	}

	// special zero/one writeouts
	if ( numSpecialChannels > 0 )
	{
		CodeChunk swizzledSrc;
		CodeChunk swizzledDest;

		swizzledSrc.Append( "float2( 0.0f, 1.0f )." );

		swizzledDest = dest;
		swizzledDest.Append( "." );

		uint32 indexSpecial = 0;
		for ( uint32 i=0; i<ARRAYSIZE(m_mask); ++i )
		{
			const char* masks[] = { "x", "y", "z", "w" };
			if ( m_mask[i] == ESwizzle::ZERO )
			{
				swizzledSrc.Append( "x" );
				swizzledDest.Append( masks[i] );
				indexSpecial += 1;
			}
			else if ( m_mask[i] == ESwizzle::ONE )
			{
				swizzledSrc.Append( "y" );
				swizzledDest.Append( masks[i] );
				indexSpecial += 1;
			}
		}

		// emit assignment
		writer.Assign( swizzledDest, swizzledSrc );
	}
}

//-----------------------------------------------------------------------------
	
