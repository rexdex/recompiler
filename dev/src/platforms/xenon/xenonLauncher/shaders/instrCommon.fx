// Common instruction implementation file for autogenerted shaders
// (C) 2014-2015 by Dex

float4 ADDv( float4 a, float4 b ) {	return a + b; }
float4 MULv( float4 a, float4 b ) {	return a * b; }
float4 MAXv( float4 a, float4 b ) {	return max(a,b); }
float4 MINv( float4 a, float4 b ) {	return min(a,b); }
float4 MULLADDv( float4 c0, float4 c1, float4 a ) {	return a + (c0*c1); }
float4 DOT4v( float4 a, float4 b ) { return dot(a,b); }
float4 DOT3v( float4 a, float4 b ) { return dot(a.xyz,b.xyz); }
float4 FRACv( float4 a ) { return frac(a); }

float4 MUL_CONST_0( float4 a, float4 b ) { return a.xxxx*b.xxxx; }
float4 MUL_CONST_1( float4 a, float4 b ) { return a.xxxx*b.xxxx; }
float4 ADD_CONST_0( float4 a, float4 b ) { return a.xxxx+b.xxxx; }
float4 ADD_CONST_1( float4 a, float4 b ) { return a.xxxx+b.xxxx; }

float4 SUB_CONST_0( float4 a, float4 b ) { return a.xxxx+b.xxxx; }
float4 SUB_CONST_1( float4 a, float4 b ) { return a.xxxx-b.xxxx; }

float4 FRACs( float a ) { return frac(a.xxxx); }
float4 MAXs( float4 a ) { return max(a.x,a.y).xxxx; }
float4 MINs( float4 a ) { return min(a.x,a.y).xxxx; }
float4 ADDs( float4 a ) { return (a.x + a.z).xxxx; } // watch out!
float4 SUBs( float4 a ) { return (a.z - a.x).xxxx; } // watch out!

float4 SETGTv( float4 a, float4 b ) { return float4( a.x>b.x ? 1.0 : 0.0, a.y>b.y ? 1.0 : 0.0, a.z>b.z ? 1.0 : 0.0, a.w>b.w ? 1.0 : 0.0 ); }   
float4 SETEv( float4 a, float4 b ) { return float4( a.x==b.x ? 1.0 : 0.0, a.y==b.y ? 1.0 : 0.0, a.z==b.z ? 1.0 : 0.0, a.w==b.w ? 1.0 : 0.0 ); }   
float4 SETGTEv( float4 a, float4 b ) { return float4( a.x>=b.x ? 1.0 : 0.0, a.y>=b.y ? 1.0 : 0.0, a.z>=b.z ? 1.0 : 0.0, a.w>=b.w ? 1.0 : 0.0 ); }   
float4 SETNEv( float4 a, float4 b ) { return float4( a.x!=b.x ? 1.0 : 0.0, a.y!=b.y ? 1.0 : 0.0, a.z!=b.z ? 1.0 : 0.0, a.w!=b.w ? 1.0 : 0.0 ); }   

float4 SETGTEs( float a ) { return ((a >= 0.0f) ? 1.0f : 0.0f).xxxx; }
float4 SETGTs( float a ) { return ((a > 0.0f) ? 1.0f : 0.0f).xxxx; }
float4 SETEs( float a ) { return ((a == 0.0f) ? 1.0f : 0.0f).xxxx; }
float4 SETNEs( float a ) { return ((a != 0.0f) ? 1.0f : 0.0f).xxxx; }

float4 CNDGTv( float4 a, float4 b, float4 c ) { return a>0 ? b : c; }
float4 CNDEv( float4 a, float4 b, float4 c ) { return a==0 ? b : c; }
float4 CNDGTEv( float4 a, float4 b, float4 c ) { return a>=0 ? b : c; }
float4 CNDNEv( float4 a, float4 b, float4 c ) { return a!=0 ? b : c; }

bool PRED_SETNEs( float4 a ) { return a != 0.0f; }
bool PRED_SETEs( float4 a ) { return a == 0.0f; }
bool PRED_SETGTs( float4 a ) { return a > 0.0f; }
bool PRED_SETGTEs( float4 a ) { return a >= 0.0f; }