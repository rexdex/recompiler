// Common include file for autogenerted pixel shaders
// (C) 2014-2015 by Dex

cbuffer PixelConsts : register (b0)
{
	float4 FloatConsts[256];
};

cbuffer CommonBoolConsts: register (b1)
{
	uint BoolConsts[32]; // 32*32 bits	
}

void SetPredicate( out bool pred, bool newValue )
{
	pred = newValue;
}

bool GetBoolConst( int regIndex )
{
	return 0 != (BoolConsts[regIndex/32] & (1 << (regIndex % 32)));
}

float4 GetFloatConst( int regIndex )
{
	return FloatConsts[regIndex];
}
