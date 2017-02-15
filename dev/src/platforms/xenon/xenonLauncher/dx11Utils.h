#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

//-----------------

// helper class for compute shaders
class CDX11ComputeShader
{
public:
	CDX11ComputeShader( const char* name );
	~CDX11ComputeShader();

	// get shader object
	inline ID3D11ComputeShader* GetShader() const { return m_shader; }

	// get name of the shader
	inline const char* GetName() const { return m_name; }

	// load shader from provided data buffer
	bool Load( ID3D11Device* dev, const void* data, const uint32 dataSize );

private:
	ID3D11ComputeShader*	m_shader;
	const char*				m_name;
};

//-----------------

// helper class with parameters
class CDX11ConstantBuffer
{
public:
	CDX11ConstantBuffer( void* dataPtr, const uint32 dataSize );
	~CDX11ConstantBuffer();

	// create the buffer
	bool Create( ID3D11Device* dev );

	// get bindable buffer, will upload content if buffer content changed
	ID3D11Buffer** GetBufferForBinding( ID3D11DeviceContext* context );

protected:
	bool					m_modified;

private:
	ID3D11Buffer*			m_buffer;

	void*					m_dataPtr;
	uint32					m_dataSize;

	bool FlushChanges( ID3D11DeviceContext* context );
};

//-----------------

// helper template that wraps CPU side structure with constant buffer support
template< typename T >
class TDX11ConstantBuffer : public CDX11ConstantBuffer
{
public:
	TDX11ConstantBuffer()
		: CDX11ConstantBuffer( &m_data, sizeof(m_data) )
	{
	}

	inline T& Get()
	{
		m_modified = true;
		return m_data;
	}

	inline const T& Get() const 
	{
		return m_data;
	}

private:
	T	m_data;
};

//-----------------

/// Ring buffer for index data
class CDX11BufferRing
{
public:
	CDX11BufferRing( ID3D11Device* dev, ID3D11DeviceContext* context, const uint32 size );
	~CDX11BufferRing();

	struct AllocInfo
	{
		uint32	m_generation;
		uint32	m_offset;
		uint32	m_size;
	};

	// acquire memory 
	void* Acquire( uint32 size, uint32 alignment, AllocInfo& outAlloc );

	// commit acquired memory
	void Commit( void* memory );

	// get buffer
	inline ID3D11Buffer* GetBuffer() const { return m_buffer; }

private:
	ID3D11Device*			m_device;
	ID3D11DeviceContext*	m_context;

	ID3D11Buffer*			m_buffer;
	uint32					m_pos;
	uint32					m_size;

	uint32					m_generation;
	bool					m_isMapped;
};

//-----------------
