#pragma once

#include <atomic>

#include "xenonGPUConstants.h"
#include "xenonGPUMicrocodeTransformer.h"

//----------------------------------------------------------------------------

namespace DX11Microcode
{

	class IHLSLWriter;

	//----------------------------------------------------------------------------

	// general expression type
	enum class EExprType : uint8
	{
		ALU,
		VFETCH,
		TFETCH,
		EXPORT,
	};

	// expression node base
	class ExprNode : public ICodeExpr
	{
	public:
		ExprNode();
		ExprNode( ExprNode* a ); // note: no ref added
		ExprNode( ExprNode* a, ExprNode* b ); // note: no ref added
		ExprNode( ExprNode* a, ExprNode* b, ExprNode* c ); // note: no ref added

		virtual ICodeExpr* Copy() override final;
		virtual void Release()  override final;
		virtual EExprType GetType() const { return EExprType::ALU; }
		virtual int GetRegisterIndex() const { return -1; }

		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const = 0;

		class IVisitor
		{
		public:
			virtual ~IVisitor() {};

			virtual void OnExprStart( const ExprNode* n ) = 0;
			virtual void OnExprEnd( const ExprNode* n ) = 0;
		};

		virtual void Visit( IVisitor& v ) const;

	protected:
		virtual ~ExprNode();

		std::atomic<int>	m_refs;
		ExprNode*			m_children[4];
	};

	// reading of register
	class ExprReadReg : public ExprNode
	{
	public:
		ExprReadReg( int regIndex );
		virtual std::string ToString() const override final; // debug only
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;
		virtual int GetRegisterIndex() const override final { return m_regIndex; }

	private:
		int m_regIndex;
	};

	// reference to non-exported output register
	class ExprWriteReg : public ExprNode
	{
	public:
		ExprWriteReg( int regIndex );
		virtual std::string ToString() const override final; // debug only
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;
		virtual int GetRegisterIndex() const override final { return m_regIndex; }

	private:
		int m_regIndex;
	};

	// reference to exported output register
	class ExprWriteExportReg : public ExprNode
	{
	public:
		enum class EExportReg
		{
			POSITION,
			POINTSIZE,
			COLOR0,
			COLOR1,
			COLOR2,
			COLOR3,
			INTERP0,
			INTERP1,
			INTERP2,
			INTERP3,
			INTERP4,
			INTERP5,
			INTERP6,
			INTERP7,
			INTERP8,
		};

		ExprWriteExportReg( EExportReg exportReg );
		virtual std::string ToString() const override final; // debug only
		virtual EExprType GetType() const { return EExprType::EXPORT; }

		inline EExportReg GetExportReg() const { return m_exporReg; }
		static const char* GetExporRegName( const EExportReg reg );
		static const char* GetExporSemanticName( const EExportReg reg );
		static int GetExportSemanticIndex( const EExportReg reg );
		static int GetExportInterpolatorIndex( const EExportReg reg );
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;

	private:
		EExportReg	m_exporReg;
	};

	// boolean constant
	class ExprBoolConst : public ExprNode
	{
	public:
		ExprBoolConst( bool pixelShader, int index );
		virtual std::string ToString() const override final; // debug only
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;

	private:
		bool m_pixelShader;
		int m_index;
	};

	// numerical constant
	class ExprFloatConst : public ExprNode
	{
	public:
		ExprFloatConst( bool pixelShader, int index );
		virtual std::string ToString() const override final; // debug only
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;

	private:
		bool m_pixelShader;
		int m_index;
	};

	// relative numerical constant
	class ExprFloatRelativeConst : public ExprNode
	{
	public:
		ExprFloatRelativeConst( bool pixelShader, int relativeOffset );
		virtual std::string ToString() const override final; // debug only
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;

	private:
		bool m_pixelShader;
		int m_relativeOffset;
	};

	// predicate access expression
	class ExprGetPredicate : public ExprNode
	{
	public:
		ExprGetPredicate();
		virtual std::string ToString() const override final; // debug only
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;
	};

	// absolute value of expression
	class ExprAbs : public ExprNode
	{
	public:
		ExprAbs( ExprNode* expr );
		virtual std::string ToString() const override final; // debug only
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;
	};

	// negate expression
	class ExprNegate : public ExprNode
	{
	public:
		ExprNegate( ExprNode* expr );
		virtual std::string ToString() const override final; // debug only
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;
	};

	// logic negation
	class ExprNot : public ExprNode
	{
	public:
		ExprNot( ExprNode* expr );
		virtual std::string ToString() const override final; // debug only
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;
	};

	// saturate expression (clamp to 0-1 range)
	class ExprSaturate : public ExprNode
	{
	public:
		ExprSaturate( ExprNode* expr );
		virtual std::string ToString() const override final; // debug only
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;
	};

	// read swizzle
	class ExprReadSwizzle : public ExprNode
	{
	public:
		typedef CXenonGPUMicrocodeTransformer::ESwizzle ESwizzle;

		ExprReadSwizzle( ExprNode* expr, ESwizzle x, ESwizzle y, ESwizzle z, ESwizzle w );
		virtual std::string ToString() const override final; // debug only
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;

	private:
		ESwizzle	m_swizzles[4];
	};

	// vertex fetch expression
	class ExprVertexFetch : public ExprNode
	{
	public:
		typedef CXenonGPUMicrocodeTransformer::EFetchFormat EFetchFormat;

		ExprVertexFetch( ExprNode* src, uint32 fetchSlot, uint32 fetchOffset, uint32 stride, EFetchFormat format, const bool isFloat, const bool isSigned, const bool isNormalized );

		virtual std::string ToString() const override final; // debug only
		virtual EExprType GetType() const { return EExprType::VFETCH; }
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;

		inline const EFetchFormat GetFormat() const { return m_format; }
		inline const uint32	GetSlot() const { return m_fetchSlot; }
		inline const uint32	GetOffset() const { return m_fetchOffset; }

		inline const uint32 GetStride() const { return m_fetchStride; }

		inline const bool IsSigned() const { return m_isSigned; }
		inline const bool IsNormalized() const { return m_isNormalized; }
		inline const bool IsFloat() const { return m_isFloat; }

		static const char* GetFormatName( const EFetchFormat format );

	private:
		EFetchFormat		m_format;
		uint32				m_fetchSlot;
		uint32				m_fetchOffset;
		uint32				m_fetchStride;

		bool				m_isFloat;
		bool				m_isSigned;
		bool				m_isNormalized;
	};

	//----------------------------------------------------------------------------

	// texture fetch expression
	class ExprTextureFetch : public ExprNode
	{
	public:
		enum class TextureType
		{
			Type1D,
			Type2D,
			Type2DArray,
			Type3D,
			TypeCube,
		};

		ExprTextureFetch( ExprNode* src, uint32 fetchSlot, TextureType textureType );

		virtual std::string ToString() const override final; // debug only
		virtual EExprType GetType() const { return EExprType::TFETCH; }
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;

		inline const uint32	GetSlot() const { return m_fetchSlot; }
		inline const TextureType GetTextureType() const { return m_textureType; }

	private:
		uint32				m_fetchSlot;
		TextureType			m_textureType;
	};

	//----------------------------------------------------------------------------

	// one argument vector function
	class ExprVectorFunc1 : public ExprNode
	{
	public:
		ExprVectorFunc1( const char* name, ExprNode* a );
		virtual std::string ToString() const override final; // debug only
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;

	private:
		const char*		m_funcName;
	};

	// two arguments vector function
	class ExprVectorFunc2 : public ExprNode
	{
	public:
		ExprVectorFunc2( const char* name, ExprNode* a, ExprNode* b );
		virtual std::string ToString() const override final; // debug only
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;

	private:
		const char*		m_funcName;
	};

	// three arguments vector function
	class ExprVectorFunc3 : public ExprNode
	{
	public:
		ExprVectorFunc3( const char* name, ExprNode* a, ExprNode* b, ExprNode* c );
		virtual std::string ToString() const override final; // debug only
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;

	private:
		const char*		m_funcName;
	};

	// one argument scalar function
	class ExprScalarFunc1 : public ExprNode
	{
	public:
		ExprScalarFunc1( const char* name, ExprNode* a );
		virtual std::string ToString() const override final; // debug only
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;

	private:
		const char*		m_funcName;
	};

	// two arguments scalar function
	class ExprScalarFunc2 : public ExprNode
	{
	public:
		ExprScalarFunc2( const char* name, ExprNode* a, ExprNode* b );
		virtual std::string ToString() const override final; // debug only
		virtual CodeChunk EmitHLSL( IHLSLWriter& writer ) const override final;

	private:
		const char*		m_funcName;
	};

	//----------------------------------------------------------------------------

	// statement node base
	class Statement : public ICodeStatement
	{
	public:
		Statement();

		enum EType
		{
			LIST,
			CONDITIONAL,
			WRITE,
		};

		class IVisitor
		{
		public:
			virtual ~IVisitor() {};

			typedef CXenonGPUMicrocodeTransformer::ESwizzle ESwizzle;

			virtual void OnWrite( const ExprNode* dest, const ExprNode* src, const ESwizzle* mask ) = 0;
			virtual void OnConditionPush( const ExprNode* condition ) = 0;
			virtual void OnConditionPop() = 0;
		};

		virtual ICodeStatement* Copy() override final;
		virtual void Release()  override final;

		virtual EType GetType() const = 0;
		virtual void Visit( IVisitor& visitor ) const = 0;		
		virtual void EmitHLSL( IHLSLWriter& writer ) const = 0; // note: no result

	protected:
		virtual ~Statement();

		std::atomic<int>	m_refs;
	};

	// list element statement
	class ListStatement : public Statement
	{
	public:
		ListStatement( Statement* a, Statement* b );
		virtual std::string ToString() const override final; // debug only
		virtual EType GetType() const override final { return EType::LIST; }
		virtual void Visit( IVisitor& visitor ) const override final;
		virtual void EmitHLSL( IHLSLWriter& writer ) const override final;

	protected:
		virtual ~ListStatement();

		Statement* m_a;
		Statement* m_b;
	};

	// conditional statement
	class ConditionalStatement : public Statement
	{
	public:
		ConditionalStatement( Statement* a, ExprNode* condition );
		virtual std::string ToString() const override final; // debug only
		virtual EType GetType() const override final { return EType::CONDITIONAL; }
		virtual void Visit( IVisitor& visitor ) const override final;
		virtual void EmitHLSL( IHLSLWriter& writer ) const override final;

	protected:
		virtual ~ConditionalStatement();

		Statement*	m_statement;
		ExprNode*	m_condition;
	};

	// predicate change statement
	class SetPredicateStatement : public Statement
	{
	public:
		SetPredicateStatement( ExprNode* expr );
		virtual std::string ToString() const override final; // debug only
		virtual EType GetType() const override final { return EType::WRITE; }
		virtual void Visit( IVisitor& visitor ) const override final;
		virtual void EmitHLSL( IHLSLWriter& writer ) const override final;

	protected:
		virtual ~SetPredicateStatement();

		ExprNode*	m_value;
	};

	// masked write statement
	class WriteWithMaskStatement : public Statement
	{
	public:
		typedef CXenonGPUMicrocodeTransformer::ESwizzle ESwizzle;

		WriteWithMaskStatement( ExprNode* target, ExprNode* source, ESwizzle x, ESwizzle y, ESwizzle z, ESwizzle w );
		virtual std::string ToString() const override final; // debug only
		virtual EType GetType() const override final { return EType::WRITE; }
		virtual void Visit( IVisitor& visitor ) const override final;
		virtual void EmitHLSL( IHLSLWriter& writer ) const override final;

	protected:
		virtual ~WriteWithMaskStatement();

		ExprNode*					m_target; // l-vaue
		ExprNode*					m_source;
		ESwizzle					m_mask[4];
	};

} // DX11Microcode

//----------------------------------------------------------------------------
