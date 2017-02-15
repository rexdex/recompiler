#include "edramCommon.h"

RWBuffer<uint> edram : register( u0 ); // EDRAM 

[numthreads(8, 8, 1)] // all of the download/upload shaders work in 8x8 blocks
void main( uint3 groupId : SV_GroupID, uint2 groupThreadId : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	uint2 pixelXY = dispatchThreadId.xy + uint2( copySurfaceTargetX, copySurfaceTargetY );

	// unpack values
	float4 values = clearColor;

	// knowing the pixelXY and the edram placement 
	uint edramElement = CalcEdramOffsetForNormalTilingMSAA1( pixelXY, edramOffsetX, edramOffsetY, edramPitch ) + (edramBaseAddr * EDRAM_TILE_X * EDRAM_TILE_Y);

	// write new values to EDRAM (each pixel writes)
	Encode_EDRAM_32( edram, edramElement, values, edramFormat );
}
