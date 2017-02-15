#include "edramCommon.h"

RWBuffer<uint> edram : register( u0 ); // EDRAM 

[numthreads(1024, 1, 1)]
void main( uint3 groupId : SV_GroupID, uint2 groupThreadId : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	// memory offset
	uint edramElement = dispatchThreadId.x;

	// restore tile index
	uint tileIndex = edramElement / (EDRAM_TILE_X*EDRAM_TILE_Y);
	uint tileX = tileIndex % (1280/EDRAM_TILE_X);
	uint tileY = tileIndex / (1280/EDRAM_TILE_X);

	uint localOffset = edramElement % (EDRAM_TILE_X*EDRAM_TILE_Y);
	uint localX = localOffset % EDRAM_TILE_X;
	uint localY = localOffset / EDRAM_TILE_X;

	// generate peudo random number
	uint randR = (dispatchThreadId.x * 3) & 0xFF;
	uint randG = (dispatchThreadId.x * 7) & 0xFF;
	uint randB = (dispatchThreadId.x * 17) & 0xFF;
	uint randA = (dispatchThreadId.x * 31) & 0xFF;
	uint color = (randR << 0) | (randG << 8) | (randB << 16) | (randA << 24);

	/*uint randR = localX*3;
	uint randG = localY*15;
	uint randB = ((tileX ^ tileY) & 1) ? 255 : 0;
	uint randA = 255;
	uint color = (randR << 0) | (randG << 8) | (randB << 16) | (randA << 24);*/

	// write peudo random number to mark untouched EDRAM areas
   edram[ edramElement ] = color;
}
