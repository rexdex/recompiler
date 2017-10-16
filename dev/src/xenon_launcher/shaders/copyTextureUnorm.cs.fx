cbuffer SETTINGS : register (b0)
{
	int copySrcOffsetX : packoffset(c0.x); // in pixels
	int copySrcOffsetY : packoffset(c0.y); // in pixels
	int copyDestOffsetX : packoffset(c0.z); // in pixels
	int copyDestOffsetY : packoffset(c0.w); // in pixels
	int copyDestSizeX : packoffset(c1.x); // in pixels
	int copyDestSizeY : packoffset(c1.y); // in pixels
};

Texture2D SrcTexture : register(t0);
RWTexture2D<unorm float4> DestTexture : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 groupId : SV_GroupID, uint2 groupThreadId : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	// get raw position
	uint2 pixelXY = dispatchThreadId.xy;

	// write only in the target area
	uint2 destPos;
	destPos.x = (uint)(pixelXY.x + copyDestOffsetX);
	destPos.y = (uint)(pixelXY.y + copyDestOffsetY);

	// select source pixel
	uint3 srcPos;
	srcPos.x = (uint)(pixelXY.x + copySrcOffsetX);
	srcPos.y = (uint)(pixelXY.y + copySrcOffsetX);
	srcPos.z = 0;

	// copy data
	DestTexture[destPos] = SrcTexture.Load(srcPos);
}