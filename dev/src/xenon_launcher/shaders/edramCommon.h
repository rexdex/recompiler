#ifndef _EDRAM_COMMON_H_
#define _EDRAM_COMMON_H_

// #define DRAW_EDRAM_BORDER

// gamma settings
#define GAMMA_DECODE 2.2f
#define GAMMA_ENCODE (1.0f/(GAMMA_DECODE))

// use "full" (picewise-linear) gamma emulation from the Xbox360 - note, it's slow
//#define FULL_GAMMA_EMULATION

// no msaa
#define EDRAM_TILE_X		80
#define EDRAM_TILE_Y		16

// generalized 32-bit format info
#define FORMAT_8_8_8_8				1
#define FORMAT_8_8_8_8_GAMMA		2
#define FORMAT_10_10_10_2			3
#define FORMAT_10_10_10_2_GAMMA		4

cbuffer SETTINGS : register (b0)
{
	// flat addressing
	uint		flatOffset : packoffset(c0.x);
	uint		flatSize : packoffset(c0.y);

	// format
	uint		edramFormat : packoffset(c0.z);
	uint		copySurfaceFormat : packoffset(c0.w);
	
	// pixel dispatch
	uint		pixelDispatchWidth : packoffset(c1.x);
	uint		pixelDispatchHeight : packoffset(c1.y);
	uint		pixelDispatcBlocksX : packoffset(c1.z);
	uint		pixelDispatcBlocksY : packoffset(c1.w);

	// EDRAM surface binding
	uint		edramBaseAddr : packoffset(c2.x);
	uint		edramPitch : packoffset(c2.y);
	uint		edramOffsetX : packoffset(c2.z);
	uint		edramOffsetY : packoffset(c2.w);

	// copy settings
	uint		copySurfaceTargetX : packoffset(c3.x); // in pixels
	uint		copySurfaceTargetY : packoffset(c3.y); // in pixels
	uint		copySurfaceTotalWidth : packoffset(c3.z); // in pixels
	uint		copySurfaceTotalHeight : packoffset(c3.w); // in pixels
	uint		copySurfaceWidth : packoffset(c4.x);; // width of the copy region
	uint		copySurfaceHeight : packoffset(c4.y);; // height of the copy region

	// clear settings
	float4      clearColor : packoffset(c5);
}

// calcualte EDRAM coordinates for NO MSSA, non tiled surface
uint CalcEdramOffsetForNormalTilingMSAA1( uint2 pixelXY, uint offsetX, uint offsetY, uint surfacePitch )
{
	// apply local offset
	pixelXY.x += offsetX;
	pixelXY.y += offsetY;

	// get the edram tile coordinates
	uint tileX = pixelXY.x / EDRAM_TILE_X;
	uint tileY = pixelXY.y / EDRAM_TILE_Y;

	// number of tiles per fow
	uint tilesPerRow = surfacePitch / EDRAM_TILE_X;

	// calculate the edram tile offset
	uint tileSize = EDRAM_TILE_X * EDRAM_TILE_Y; // size of tile in the memory
	uint tileRowSize = tileSize * tilesPerRow; // size of tile in the memory

	// now, put it all back together to get the base address of the tile
	uint tileBase = (tileX*tileSize) + (tileY*tileRowSize);

	// compute memory address WITHIN the tile
	uint localX = pixelXY.x % EDRAM_TILE_X;
	uint localY = pixelXY.y % EDRAM_TILE_Y;

	// compute local offset
	uint localOffset = (localX) + (localY*EDRAM_TILE_X);

	// assemble local & global offset
	return tileBase + localOffset;
}

//-----------------------------------------------------------------------------

// copied from: Programmatically Applying and Removing Gamma Correction 
float XBoxPWLGamma(float Clin)
{
	float Cgam;

	if (Clin < 64.0f)
	{
		Cgam = Clin;
	}
	else if (Clin < 128.0f)
	{
		Cgam = 64.0f + (Clin-64.0f)/2.0f;
	}
	else if (Clin < 513)
	{
		Cgam = 96.0f + (Clin-128.0f)/4.0f;
	}
	else
	{
		Cgam = 192.0f + (Clin-512.0f)/8.0f;
	}

	return Cgam;
}

// copied from: Programmatically Applying and Removing Gamma Correction 
float XBoxPWLDegamma(float C)
{
	float Clin;

	if (C < 64.0f)
	{
		Clin = C;
	}
	else if (C < 96.0f)
	{
		Clin = 64.0f + (C-64.0f)*2.0f;
	}
	else if (C < 192.0f)
	{
		Clin = 128.0f + (C-96.0f)*4.0f;
	}
	else
	{
		Clin = 513.0f + (C-192.0f)*8.0f;
	}
	return Clin;
}

// encode gamma
float4 EncodeGamma( float4 val )
{
	float4 ret = val;
#ifdef FULL_GAMMA_EMULATION
	ret.x = XBoxPWLGamma(saturate(val.x) * 1023.0f) / 255.0f;
	ret.y = XBoxPWLGamma(saturate(val.y) * 1023.0f) / 255.0f;
	ret.z = XBoxPWLGamma(saturate(val.z) * 1023.0f) / 255.0f;
#else
	ret.x = pow( abs(val.x), GAMMA_ENCODE );
	ret.y = pow( abs(val.y), GAMMA_ENCODE );
	ret.z = pow( abs(val.z), GAMMA_ENCODE );
#endif
	return ret;
}

// decode gamma
float4 DecodeGamma( float4 val )
{
	float4 ret = val;
#ifdef FULL_GAMMA_EMULATION
	ret.x = XBoxPWLDegamma(saturate(val.x) * 255.0f) / 1023.0f;
	ret.y = XBoxPWLDegamma(saturate(val.y) * 255.0f) / 1023.0f;
	ret.z = XBoxPWLDegamma(saturate(val.z) * 255.0f) / 1023.0f;
#else
	ret.x = pow(abs(val.x), GAMMA_DECODE );
	ret.y = pow(abs(val.y), GAMMA_DECODE );
	ret.z = pow(abs(val.z), GAMMA_DECODE );
#endif
	return ret;
}

//-----------------------------------------------------------------------------

// decode EDRAM as 8_8_8_8 color, range returned 0-1
float4 Decode_EDRAM_8_8_8_8( RWBuffer<uint> edram, uint edramOffset )
{
	uint edramValue = edram[ edramOffset ];
	uint dataR = (edramValue >> 0) & 0xFF;
	uint dataG = (edramValue >> 8) & 0xFF;
	uint dataB = (edramValue >> 16) & 0xFF;
	uint dataA = (edramValue >> 24) & 0xFF;

	return float4( dataR / 255.0f, dataG / 255.0f, dataB / 255.0f, dataA / 255.0f );
}

// decode EDRAM from custom 32-bit format
float4 Decode_EDRAM_32( RWBuffer<uint> edram, uint edramOffset, uint edramFormat )
{
	if ( edramFormat == FORMAT_8_8_8_8 )
		return Decode_EDRAM_8_8_8_8( edram, edramOffset );
	else if ( edramFormat == FORMAT_8_8_8_8_GAMMA )
		return DecodeGamma( Decode_EDRAM_8_8_8_8( edram, edramOffset ) );
	
	return float4(1.0f, 0.0f, 0.5f, 1.0f);
}

//-----------------------------------------------------------------------------

// encode 0-1 values in EDRAM compatible 8_8_8_8 format
void Encode_EDRAM_8_8_8_8( RWBuffer<uint> edram, uint edramOffset, float4 values )
{
	uint dataR = saturate(values.x) * 255.0f;
	uint dataG = saturate(values.y) * 255.0f;
	uint dataB = saturate(values.z) * 255.0f;
	uint dataA = saturate(values.w) * 255.0f;

	// write new values to EDRAM (each pixel writes)
	uint edramValue = ((dataR & 0xFF) << 0) | ((dataG & 0xFF) << 8) | ((dataB & 0xFF) << 16) | ((dataA & 0xFF) << 24);
	edram[ edramOffset ] = edramValue;
}

// encode values into 32-bit EDRAM compatible format
void Encode_EDRAM_32( RWBuffer<uint> edram, uint edramOffset, float4 values, uint edramFormat )
{
	if ( edramFormat == FORMAT_8_8_8_8 )
		Encode_EDRAM_8_8_8_8( edram, edramOffset, values );
	else if ( edramFormat == FORMAT_8_8_8_8_GAMMA )
		Encode_EDRAM_8_8_8_8( edram, edramOffset, EncodeGamma( values ) );
}

//-----------------------------------------------------------------------------

// store value to 32-bit texture
uint4 Pack_Texture_8_8_8_8( float4 values )
{
	return uint4( saturate(values) * 255.0f );
}

// pack based on format
uint4 Pack_Texture( float4 values, uint textureFormat )
{
	if ( textureFormat == FORMAT_8_8_8_8 )
		return Pack_Texture_8_8_8_8( values );
	else if ( textureFormat == FORMAT_8_8_8_8_GAMMA )
		return Pack_Texture_8_8_8_8( EncodeGamma( values ) );

	return uint4(255,0,0,255);
}

//-----------------------------------------------------------------------------

// unpack value from 32-bit 8_8_8_8 texture
float4 Unpack_Texture_8_8_8_8( uint4 values )
{
	return float4( values / 255.0f );
}

// unpack texture based on format
float4 Unpack_Texture( float4 values, uint textureFormat )
{
	if ( textureFormat == FORMAT_8_8_8_8 )
		return Unpack_Texture_8_8_8_8( values );
	else if ( textureFormat == FORMAT_8_8_8_8_GAMMA )
		return DecodeGamma( Unpack_Texture_8_8_8_8( values ) );

	return float4(0.5f, 0.0f, 1.0f, 1.0f);
}

#endif