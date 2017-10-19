// Common instruction implementation file for autogenerted shaders
// (C) 2014-2017 by Dex

///------------------------------
/// VECTOR OPERATIONS
///------------------------------

#define FLT_MAX          3.402823466e+38F        // max value

float4 ADDv(inout REGS regs, float4 a, float4 b)
{
	regs.PV = a + b;
	return regs.PV;
}

float4 MULv(inout REGS regs, float4 a, float4 b)
{
	regs.PV = a * b;
	return regs.PV;
}

float4 MAXv(inout REGS regs, float4 a, float4 b)
{
	regs.PV = max(a, b);
	return regs.PV;
}

float4 MINv(inout REGS regs, float4 a, float4 b)
{
	regs.PV = min(a, b);
	return regs.PV;
}

float4 MULLADDv(inout REGS regs, float4 c0, float4 c1, float4 a)
{
	regs.PV = a + (c0*c1);
	return regs.PV;
}

float4 DOT4v(inout REGS regs, float4 a, float4 b)
{
	regs.PV = dot(a, b);
	return regs.PV;
}

float4 DOT3v(inout REGS regs, float4 a, float4 b)
{
	regs.PV = dot(a.xyz, b.xyz);
	return regs.PV;
}

float4 DOT2ADDv(inout REGS regs, float4 a, float4 b, float4 c)
{
	regs.PV = (a.x*b.x + a.y*b.y + c.x).xxxx;
	return regs.PV;
}

float4 FRACv(inout REGS regs, float4 a)
{
	regs.PV = frac(a);
	return regs.PV;
}

float4 TRUNCv(inout REGS regs, float4 a)
{
	regs.PV = trunc(a);
	return regs.PV;
}

float4 MUL_CONST_0(inout REGS regs, float4 a, float4 b)
{
	regs.PV = a.xxxx*b.xxxx;
	return regs.PV;
}

float4 MUL_CONST_1(inout REGS regs, float4 a, float4 b)
{
	regs.PV = a.xxxx*b.xxxx;
	return regs.PV;
}

float4 ADD_CONST_0(inout REGS regs, float4 a, float4 b)
{
	regs.PV = a.xxxx + b.xxxx;
	return regs.PV;
}

float4 ADD_CONST_1(inout REGS regs, float4 a, float4 b)
{
	regs.PV = a.xxxx + b.xxxx;
	return regs.PV;
}

float4 SUB_CONST_0(inout REGS regs, float4 a, float4 b)
{
	regs.PV = a.xxxx + b.xxxx;
	return regs.PV;
}

float4 SUB_CONST_1(inout REGS regs, float4 a, float4 b)
{
	regs.PV = a.xxxx - b.xxxx;
	return regs.PV;
}

float4 SETGTv(inout REGS regs, float4 a, float4 b)
{
	regs.PV = float4(a.x>b.x ? 1.0 : 0.0, a.y>b.y ? 1.0 : 0.0, a.z>b.z ? 1.0 : 0.0, a.w>b.w ? 1.0 : 0.0);
	return regs.PV;
}

float4 SETEv(inout REGS regs, float4 a, float4 b)
{
	regs.PV = float4(a.x == b.x ? 1.0 : 0.0, a.y == b.y ? 1.0 : 0.0, a.z == b.z ? 1.0 : 0.0, a.w == b.w ? 1.0 : 0.0);
	return regs.PV;
}

float4 SETGTEv(inout REGS regs, float4 a, float4 b)
{
	regs.PV = float4(a.x >= b.x ? 1.0 : 0.0, a.y >= b.y ? 1.0 : 0.0, a.z >= b.z ? 1.0 : 0.0, a.w >= b.w ? 1.0 : 0.0);
	return regs.PV;
}

float4 SETNEv(inout REGS regs, float4 a, float4 b)
{
	regs.PV = float4(a.x != b.x ? 1.0 : 0.0, a.y != b.y ? 1.0 : 0.0, a.z != b.z ? 1.0 : 0.0, a.w != b.w ? 1.0 : 0.0);
	return regs.PV;
}

float4 CNDGTv(inout REGS regs, float4 a, float4 b, float4 c)
{
	regs.PV = a>0 ? b : c;
	return regs.PV;
}

float4 CNDEv(inout REGS regs, float4 a, float4 b, float4 c)
{
	regs.PV = a == 0 ? b : c;
	return regs.PV;
}

float4 CNDGTEv(inout REGS regs, float4 a, float4 b, float4 c)
{
	regs.PV = a >= 0 ? b : c;
	return regs.PV;
}

float4 CNDNEv(inout REGS regs, float4 a, float4 b, float4 c)
{
	regs.PV = a != 0 ? b : c;
	return regs.PV;
}

float4 PRED_SETE_PUSHv(inout REGS regs, float4 a, float4 b)
{
	regs.PRED = (a.w == 0.0 && b.w == 0.0) ? true : false;
	regs.PV = (a.x == 0.0 && b.x == 0.0) ? 0.0 : a.x + 1.0f;
	return regs.PV;
}

float4 PRED_SETNE_PUSHv(inout REGS regs, float4 a, float4 b)
{
	regs.PRED = (a.w == 0.0 && b.w != 0.0) ? true : false;
	regs.PV = (a.x == 0.0 && b.x != 0.0) ? 0.0 : a.x + 1.0f;
	return regs.PV;
}

float4 PRED_SETGT_PUSHv(inout REGS regs, float4 a, float4 b)
{
	regs.PRED = (a.w == 0.0 && b.w > 0.0) ? true : false;
	regs.PV = (a.x == 0.0 && b.x > 0.0) ? 0.0 : a.x + 1.0f;
	return regs.PV;
}

float4 PRED_SETGTE_PUSHv(inout REGS regs, float4 a, float4 b)
{
	regs.PRED = (a.w == 0.0 && b.w >= 0.0) ? true : false;
	regs.PV = (a.x == 0.0 && b.x >= 0.0) ? 0.0 : a.x + 1.0f;
	return regs.PV;
}

// http://www.nvidia.com/object/cube_map_ogl_tutorial.html
// http://developer.amd.com/wordpress/media/2012/10/R600_Instruction_Set_Architecture.pdf
// a = Rn.zzxy, b = Rn.yxzz
// dst.W = FaceId;
// dst.Z = 2.0f * MajorAxis;
// dst.Y = S cube coordinate;
// dst.X = T cube coordinate;
/*
major axis
direction     target                                sc     tc    ma
----------   ------------------------------------   ---    ---   ---
+rx          GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT=0   -rz    -ry   rx
-rx          GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT=1   +rz    -ry   rx
+ry          GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT=2   +rx    +rz   ry
-ry          GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT=3   +rx    -rz   ry
+rz          GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT=4   +rx    -ry   rz
-rz          GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT=5   -rx    -ry   rz
*/
float4 CUBEv(inout REGS regs, float4 src0, float4 src1)
{
	float3 src = float3(src1.y, src1.x, src1.z);
	float3 abs_src = abs(src);
	int face_id;
	float sc;
	float tc;
	float ma;
	if (abs_src.x > abs_src.y && abs_src.x > abs_src.z)
	{
		if (src.x > 0.0)
		{
			face_id = 0;
			sc = -abs_src.z;
			tc = -abs_src.y;
			ma = abs_src.x;
		}
		else
		{
			face_id = 1;
			sc = abs_src.z;
			tc = -abs_src.y;
			ma = abs_src.x;
		}
	}
	else if (abs_src.y > abs_src.x && abs_src.y > abs_src.z)
	{
		if (src.y > 0.0)
		{
			face_id = 2;
			sc = abs_src.x;
			tc = abs_src.z;
			ma = abs_src.y;
		}
		else
		{
			face_id = 3;
			sc = abs_src.x;
			tc = -abs_src.z;
			ma = abs_src.y;
		}
	}
	else
	{
		if (src.z > 0.0)
		{
			face_id = 4;
			sc = abs_src.x;
			tc = -abs_src.y;
			ma = abs_src.z;
		}
		else
		{
			face_id = 5;
			sc = -abs_src.x;
			tc = -abs_src.y;
			ma = abs_src.z;
		}
	}
	float s = (sc / ma + 1.0) / 2.0;
	float t = (tc / ma + 1.0) / 2.0;

	regs.PV = float4(t, s, 2.0 * ma, float(face_id));
	return regs.PV;
}

///------------------------------
/// SCALAR OPERATIONS
///------------------------------

float4 FRACs(inout REGS regs, float a)
{ 
	regs.PS = frac(a.x).xxxx;
	return regs.PS;
}
    
float4 TRUNCs(inout REGS regs, float a)
{
	regs.PS = trunc(a.x).xxxx;
	return regs.PS;
}

float4 FLOORs(inout REGS regs, float a)
{
	regs.PS = floor(a.x).xxxx;
	return regs.PS;
}

float4 EXP_IEEE(inout REGS regs, float a)
{
	regs.PS = exp(a.x).xxxx;
	return regs.PS;
}

float4 LOG_CLAMP(inout REGS regs, float a)
{
	float v = log2(a.x); 
	regs.PS = isinf(v) ? -FLT_MAX : v;
	return regs.PS;
}

float4 LOG_IEEE(inout REGS regs, float a)
{
	regs.PS = log2(a.x).xxxx;
	return regs.PS;
}

float4 MAXs(inout REGS regs, float4 a)
{
	regs.PS = max(a.x, a.y).xxxx;
	return regs.PS;
}

float4 MINs(inout REGS regs, float4 a)
{
	regs.PS = min(a.x, a.y).xxxx;
	return regs.PS;
}

float4 ADDs(inout REGS regs, float4 a)
{ 
	regs.PS = (a.x + a.y).xxxx;// watch out!
	return regs.PS;
}

float4 ADD_PREVs(inout REGS regs, float a)
{
	regs.PS += a.x;
	return regs.PS;
}

float4 SUBs(inout REGS regs, float4 a)
{
	regs.PS = (a.x - a.y).xxxx;
	return regs.PS;
}

float4 MULs(inout REGS regs, float4 a)
{
	regs.PS = a.x * a.y;
	return regs.PS;
}

float4 MUL_PREVs(inout REGS regs, float4 a)
{
	regs.PS = a.x * regs.PS;
	return regs.PS;
}

float4 MUL_PREV2s(inout REGS regs, float4 a)
{
	regs.PS = ((regs.PS == -FLT_MAX) || isinf(regs.PS) || isnan(regs.PS) || isnan(a.y) || a.y <= 0.0) ? -FLT_MAX : a.x * regs.PS;
	return regs.PS;
}

float4 MOVAs(inout REGS regs, float4 a)
{
	regs.A0 = clamp(int(floor(a.x + 0.5f)), -256.0f, 255.0f);
	regs.PS = max(a.x, a.y);
	return regs.PS;
}

float4 SETGTEs(inout REGS regs, float a)
{
	regs.PS = ((a >= 0.0f) ? 1.0f : 0.0f).xxxx;
	return regs.PS;
}

float4 SETGTs(inout REGS regs, float a)
{
	regs.PS = ((a > 0.0f) ? 1.0f : 0.0f).xxxx;
	return regs.PS;
}

float4 SETEs(inout REGS regs, float a)
{
	regs.PS = ((a == 0.0f) ? 1.0f : 0.0f).xxxx;
	return regs.PS;
}

float4 SETNEs(inout REGS regs, float a)
{ 
	regs.PS = ((a != 0.0f) ? 1.0f : 0.0f).xxxx;
	return regs.PS;
}

float4 RECIP_CLAMP(inout REGS regs, float4 x)
{
	float a = 1.0f / x.x; 
	regs.PS = isinf(a) ? FLT_MAX : a;
	return regs.PS;
}

float4 RECIP_FF(inout REGS regs, float4 x)
{
	float a = 1.0f / x.x; regs.PS = isinf(a) ? 0.0f : a;
	return regs.PS;
}

float4 RECIP_IEEE(inout REGS regs, float4 x)
{
	regs.PS = (x.x != 0.0) ? (1.0f / x.x) : 0.0f;
	return regs.PS;
}

float4 RECIPSQ_CLAMP(inout REGS regs, float4 x)
{
	float a = rsqrt(x.x); 
	regs.PS = isinf(a) ? FLT_MAX : a;
	return regs.PS;
}

float4 RECIPSQ_FF(inout REGS regs, float4 x)
{
	float a = rsqrt(x.x); 
	regs.PS = isinf(a) ? 0.0f : a;
	return regs.PS;
}

float4 RECIPSQ_IEEE(inout REGS regs, float4 x)
{
	regs.PS = (x.x > 0.0f) ? rsqrt(x.x) : 0.0f;
	return regs.PS;
}

float4 PRED_SETNEs(inout REGS regs, float4 a)
{
	regs.PRED = (a.x != 0.0f);
	regs.PS = a.x != 0.0f ? 0.0f : 1.0f;
	return regs.PS;
}

float4 PRED_SETEs(inout REGS regs, float4 a)
{
	regs.PRED = (a.x == 0.0f);
	regs.PS = a.x == 0.0f ? 0.0f : 1.0f;
	return regs.PS;
}

float4 PRED_SETGTs(inout REGS regs, float4 a)
{
	regs.PRED = (a.x > 0.0f);
	regs.PS = a.x > 0.0f ? 0.0f : 1.0f;
	return regs.PS;
}

float4 PRED_SETGTEs(inout REGS regs, float4 a)
{
	regs.PRED = (a.x >= 0.0f);
	regs.PS = a.x >= 0.0f ? 0.0f : 1.0f;
	return regs.PS;
}

float4 PRED_SET_INVs(inout REGS regs, float4 a)
{
	regs.PRED = (a.x == 1.0f) ? true : false;
	regs.PS = a.x == 1.0f ? 0.0f : (a.x == 0.0f ? 1.0f : a.x);
	return regs.PS;
}

float4 PRED_SET_POPs(inout REGS regs, float4 a)
{
	regs.PRED = ((a.x - 1.0f) < 0.0f) ? true : false;
	regs.PS = ((a.x - 1.0f) < 0.0f) ? 0.0 : 1.0f - a.x;
	return regs.PS;
}

float4 PRED_SET_CLRs(inout REGS regs, float4 a)
{
	regs.PRED = false;
	regs.PS = FLT_MAX;
	return regs.PS;
}

float4 PRED_SET_RESTOREs(inout REGS regs, float4 a)
{
	regs.PRED = (a.x == 0.0f) ? true : false;
	regs.PS = a.x;
	return regs.PS;
}

float RETAIN_PREV(inout REGS regs, float4 a)
{
	return regs.PS;
}

