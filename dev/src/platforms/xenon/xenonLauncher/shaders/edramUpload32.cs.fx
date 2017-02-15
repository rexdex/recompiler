#include "edramCommon.h"

RWBuffer<uint> edram : register( u0 ); // EDRAM 
Texture2D<uint4> surface : register( t0 ); // 8_8_8_8 

[numthreads(8, 8, 1)] // all of the download/upload shaders work in 8x8 blocks
void main( uint3 groupId : SV_GroupID, uint2 groupThreadId : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	uint2 pixelXY = dispatchThreadId.xy + uint2( copySurfaceTargetX, copySurfaceTargetY );

	// get the value from surface
	uint4 data = surface.Load( int3(pixelXY, 0) );

	// unpack values
	float4 values = Unpack_Texture( data, copySurfaceFormat );

	// knowing the pixelXY and the edram placement 
	uint edramElement = CalcEdramOffsetForNormalTilingMSAA1( pixelXY, edramOffsetX, edramOffsetY, edramPitch ) + (edramBaseAddr * EDRAM_TILE_X * EDRAM_TILE_Y);

	// write new values to EDRAM (each pixel writes)
	values = float4(1,1,1,1);
	Encode_EDRAM_32( edram, edramElement, values, edramFormat );
}
