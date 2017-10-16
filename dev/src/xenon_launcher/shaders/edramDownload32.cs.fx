#include "edramCommon.h"

RWBuffer<uint> edram : register( u0 ); // EDRAM 
RWTexture2D<uint4> surface : register( u1 ); // 8_8_8_8 

[numthreads(8, 8, 1)] // all of the download/upload shaders work in 8x8 blocks
void main( uint3 groupId : SV_GroupID, uint2 groupThreadId : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	uint2 pixelXY = dispatchThreadId.xy + uint2( copySurfaceTargetX, copySurfaceTargetY );

	// knowing the pixelXY and the edram placement 
	// load edram content
	uint edramElement = CalcEdramOffsetForNormalTilingMSAA1( pixelXY, edramOffsetX, edramOffsetY, edramPitch ) + (edramBaseAddr * EDRAM_TILE_X * EDRAM_TILE_Y);

	// load memory from EDRAM
	float4 values = Decode_EDRAM_32( edram, edramElement, edramFormat );

	// store in surface
	//surface[ pixelXY ] = uint4( pixelXY.xy, 0, 0 );
	surface[ pixelXY ] = Pack_Texture( values, copySurfaceFormat );
}
