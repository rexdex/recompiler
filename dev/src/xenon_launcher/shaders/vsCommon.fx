//-----------------
// index buffer fake
//-----------------

Buffer<uint> IndexBuffer : register( t15 ); // hardcoded

//-----------------
// constants
//-----------------

cbuffer VertexConsts : register (b0)
{
	float4 FloatConsts[256];
};

cbuffer CommonBoolConsts: register (b1)
{
	uint BoolConsts[32]; // 32*32 bits	
}

bool GetBoolConst( int regIndex )
{
	return 0 != (BoolConsts[regIndex/32] & (1 << (regIndex % 32)));
}

float4 GetFloatConst( int regIndex )
{
	return FloatConsts[regIndex];
}

//float4 SUB_CONST_0( float4 a, float4 b ) { return a.xxxx-b.xxxx; }
//float4 SUB_CONST_1( float4 a, float4 b ) { return a.xxxx-b.xxxx; }

cbuffer ViewportState : register (b2)
{
	uint4  VertexTransfrom; // xyDiv, zDiv, wNotInv, -
	uint4  Coordinates;
	uint4  IndexingData; // isIndexed, baseVertex
	float4 ViewportSize; // size, invSize
}

uint GetVertexIndex( uint id )
{
	if ( IndexingData.x != 0 )
	{
		return IndexingData.y + IndexBuffer[ id ];
	}
	else
	{
		return IndexingData.y + id;
	}
}

float4 PostTransformVertex( uint vertexId, float4 pos )
{
	// w was already inverted
	if ( VertexTransfrom.z == 0 )
		pos.w = 1.0f / pos.w;

	// xy was already divided by W, fix it
	if ( VertexTransfrom.x != 0 )
		pos.xy /= pos.w;

	// z was already divided by W, fix it
	if ( VertexTransfrom.y != 0 )
		pos.z /= pos.w;

	// non-normalized coordinates
	if ( Coordinates.x == 0 )
	{
		pos.xy *= ViewportSize.zw;
		pos.w = 1;
	}

	//pos.xy /= 20.0f;
		//pos.z = 0.5;
		//pos.w = 1;


	return pos;
}

//-----------------
// predicate logic
//-----------------

void SetPredicate( out bool pred, bool newValue )
{
	pred = newValue;
}

//-----------------
// fetchers
//-----------------

// calculate fetch position
uint CalcFetchPosition( uint offset, uint stride, uint index )
{
	return offset + (stride*index);
}

// out format: unsigned 8bits
uint4 FetchVertex_8_8_8_8( Buffer<uint> b, uint offset, uint stride, uint index )
{
	uint data = b.Load( CalcFetchPosition( offset, stride, index ) );

	uint4 ret;
	ret.x = (data >> 0) & 0xFF;
	ret.y = (data >> 8) & 0xFF;
	ret.z = (data >> 16) & 0xFF;
	ret.w = (data >> 24) & 0xFF;

	return ret;
}

// out format: unsigned 16bits
uint4 FetchVertex_16_16( Buffer<uint> b, uint offset, uint stride, uint index )
{
	uint data = b.Load( CalcFetchPosition( offset, stride, index ) );

	uint4 ret;
	ret.x = (data >> 0) & 0xFFFF;
	ret.y = (data >> 16) & 0xFFFF;
	ret.z = 0;
	ret.w = 0;

	return ret;
}

uint4 FetchVertex_16_16_FLOAT(Buffer<uint> b, uint offset, uint stride, uint index)
{
	uint4 ret = FetchVertex_16_16(b, offset, stride, index);

	ret.x = asuint(f16tof32(ret.x));
	ret.y = asuint(f16tof32(ret.y));
	ret.z = 0;
	ret.w = 0;

	return ret;
}

// out format: unsigned 16bits
uint4 FetchVertex_16_16_16_16(Buffer<uint> b, uint offset, uint stride, uint index)
{
	uint dataA = b.Load(CalcFetchPosition(offset, stride, index));
	uint dataB = b.Load(CalcFetchPosition(offset, stride, index) + 4);

	uint4 ret;
	ret.x = (dataA >> 0) & 0xFFFF;
	ret.y = (dataA >> 16) & 0xFFFF;
	ret.z = (dataB >> 0) & 0xFFFF;
	ret.w = (dataB >> 16) & 0xFFFF;

	return ret;
}

uint4 FetchVertex_16_16_16_16_FLOAT(Buffer<uint> b, uint offset, uint stride, uint index)
{
	uint4 ret = FetchVertex_16_16_16_16(b, offset, stride, index);

	ret.x = asuint(f16tof32(ret.x));
	ret.y = asuint(f16tof32(ret.y));
	ret.z = asuint(f16tof32(ret.z));
	ret.w = asuint(f16tof32(ret.w));

	return ret;
}

// out format: unsigned 32 bit 
uint4 FetchVertex_32( Buffer<uint> b, uint offset, uint stride, uint index )
{
	uint4 ret;
	ret.x = b.Load( CalcFetchPosition( offset, stride, index ) + 0 );
	ret.y = 0;
	ret.z = 0;
	ret.w = 0;
	return ret;
}

// out format: unsigned 32 bit 
uint4 FetchVertex_32_32( Buffer<uint> b, uint offset, uint stride, uint index )
{
	uint4 ret;
	ret.x = b.Load( CalcFetchPosition( offset, stride, index ) + 0 );
	ret.y = b.Load( CalcFetchPosition( offset, stride, index ) + 1 );
	ret.z = 0;
	ret.w = 0;

	/*if (index == 0)
		ret.xy = asuint(float2(10,10));
	else if (index == 1 )
		ret.xy = asuint(float2(1270,10));
	else if (index == 2 )
		ret.xy = asuint(float2(1270,710));
	else if (index == 3)
		ret.xy = asuint(float2(10,10));
	else if (index == 4 )
		ret.xy = asuint(float2(1270,710));
	else if (index == 5 )
		ret.xy = asuint(float2(10,710));*/

	return ret;
}

// out format: unsigned 32 bit 
uint4 FetchVertex_2_10_10_10( Buffer<uint> b, uint offset, uint stride, uint index )
{
	uint4 ret;
	uint data = b.Load( CalcFetchPosition( offset, stride, index ) + 0 );
	//ret.x = ((data >> 22) & 0x3FF) << 6;
	//ret.y = ((data >> 12) & 0x3FF) << 6;
	//ret.z = ((data >> 2) & 0x3FF) << 6;
	//ret.w = ((data >> 0) & 0x3) << 14;

	ret.x = ((data >> 0) & 0x3FF) << 6;
	ret.y = ((data >> 10) & 0x3FF) << 6;
	ret.z = ((data >> 20) & 0x3FF) << 6;
	ret.w = ((data >> 30) & 0x3) << 14;

	//return uint4(0,0,0,0);
	//return ret.wwww;
	//return uint4( 0x8000, 0x8000, 0x8000, 0 );
	return ret;
}

// out format: unsigned 32 bit 
uint4 FetchVertex_32_32_32( Buffer<uint> b, uint offset, uint stride, uint index )
{
	uint4 ret;
	ret.x = b.Load( CalcFetchPosition( offset, stride, index ) + 0 );
	ret.y = b.Load( CalcFetchPosition( offset, stride, index ) + 1 );
	ret.z = b.Load( CalcFetchPosition( offset, stride, index ) + 2 );
	ret.w = 0;
	return ret;
}

// out format: unsigned 32 bit 
uint4 FetchVertex_32_32_32_32( Buffer<uint> b, uint offset, uint stride, uint index )
{
	uint4 ret;
	ret.x = b.Load( CalcFetchPosition( offset, stride, index ) + 0 );
	ret.y = b.Load( CalcFetchPosition( offset, stride, index ) + 1 );
	ret.z = b.Load( CalcFetchPosition( offset, stride, index ) + 2 );
	ret.w = b.Load( CalcFetchPosition( offset, stride, index ) + 3 );
	return ret;
}

//-----------------
// format conversion
//-----------------

int MakeSigned8( uint val )
{
	if ( val & 0x80 )
		val |= 0xFFFFFF00;

	return (int)val;// - (int)127;
}

int MakeSigned16( uint val )
{
	if ( val & 0x8000 )
		val |= 0xFFFF0000;

	return (int)val;// - (int)32767;
}

int MakeSigned32( uint val )
{
	return (int)val;
}

float NormalizeSinged8Val( int val )
{
	return (val < 0) ? ((float)val / 128.0f) : ((float)val / 127.0f);
}

float NormalizeSinged16Val( int val )
{
	return (val < 0) ? ((float)val / 32768.0f) : ((float)val / 32767.0f);
}

float NormalizeSinged32Val( int val )
{
	return (val < 0) ? ((float)val / 2147483647.0f) : ((float)val / 2147483647.0f); // is this used ?
}

float NormalizeUnsigned8Val( uint val )
{
	return (float)val / 255.0f;
}

float NormalizeUnsigned16Val( uint val )
{
	return (float)val / 65535.0f;
}

float NormalizeUnsigned32Val( uint val )
{
	return (float)val / 4294967295.0f; // is this used ?
}

// normalize 8bit value to -1, 1 range
float4 NormalizeSigned8( uint4 val )
{
	float4 ret;
	ret.x = NormalizeSinged8Val( MakeSigned8( val.x ) );
	ret.y = NormalizeSinged8Val( MakeSigned8( val.y ) );
	ret.z = NormalizeSinged8Val( MakeSigned8( val.z ) );
	ret.w = NormalizeSinged8Val( MakeSigned8( val.w ) );
	return ret;
}

// normalize 16bit value to -1, 1 range
float4 NormalizeSigned16( uint4 val )
{
	float4 ret;
	ret.x = NormalizeSinged16Val( MakeSigned16( val.x ) );
	ret.y = NormalizeSinged16Val( MakeSigned16( val.y ) );
	ret.z = NormalizeSinged16Val( MakeSigned16( val.z ) );
	ret.w = NormalizeSinged16Val( MakeSigned16( val.w ) );
	return ret;
}

// normalize 32bit value to -1, 1 range
float4 NormalizeSigned32( uint4 val )
{
	float4 ret;
	ret.x = NormalizeSinged32Val( MakeSigned32( val.x ) );
	ret.y = NormalizeSinged32Val( MakeSigned32( val.y ) );
	ret.z = NormalizeSinged32Val( MakeSigned32( val.z ) );
	ret.w = NormalizeSinged32Val( MakeSigned32( val.w ) );
	return ret;
}

// normalize unsigned 8bit value to 0, 1 range
float4 NormalizeUnsigned8( uint4 val )
{
	float4 ret;
	ret.x = NormalizeUnsigned8Val( val.x );
	ret.y = NormalizeUnsigned8Val( val.y );
	ret.z = NormalizeUnsigned8Val( val.z );
	ret.w = NormalizeUnsigned8Val( val.w );
	return ret;
}

// normalize unsigned 16bit value to 0, 1 range
float4 NormalizeUnsigned16( uint4 val )
{
	float4 ret;
	ret.x = NormalizeUnsigned16Val( val.x );
	ret.y = NormalizeUnsigned16Val( val.y );
	ret.z = NormalizeUnsigned16Val( val.z );
	ret.w = NormalizeUnsigned16Val( val.w );
	return ret;
}

// normalize unsigned 32bit value to 0, 1 range
float4 NormalizeUnsigned32( uint4 val )
{
	float4 ret;
	ret.x = NormalizeUnsigned32Val( val.x );
	ret.y = NormalizeUnsigned32Val( val.y );
	ret.z = NormalizeUnsigned32Val( val.z );
	ret.w = NormalizeUnsigned32Val( val.w );
	return ret;
}

// convert signed integer to a floating point value
float4 UnnormalizedSigned8( uint4 val )
{
	float4 ret;
	ret.x = MakeSigned8( val.x );
	ret.y = MakeSigned8( val.y );
	ret.z = MakeSigned8( val.z );
	ret.w = MakeSigned8( val.w );
	return ret;
}

// convert signed integer to a floating point value
float4 UnnormalizedSigned16( uint4 val )
{
	float4 ret;
	ret.x = MakeSigned16( val.x );
	ret.y = MakeSigned16( val.y );
	ret.z = MakeSigned16( val.z );
	ret.w = MakeSigned16( val.w );
	return ret;
}

// convert signed integer to a floating point value
float4 UnnormalizedSigned32( uint4 val )
{
	float4 ret;
	ret.x = MakeSigned32( val.x );
	ret.y = MakeSigned32( val.y );
	ret.z = MakeSigned32( val.z );
	ret.w = MakeSigned32( val.w );
	return ret;
}

// convert unsigned integer to a floating point value
float4 UnnormalizedUnsigned8( uint4 val )
{
	return (float4) val;
}

// convert unsigned integer to a floating point value
float4 UnnormalizedUnsigned16( uint4 val )
{
	return (float4) val;
}

// convert unsigned integer to a floating point value
float4 UnnormalizedUnsigned32( uint4 val )
{
	return (float4) val;
}
