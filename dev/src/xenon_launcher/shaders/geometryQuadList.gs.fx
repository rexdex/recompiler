struct VERT
{
	float4 Pos : SV_Position;
	float4 T0 : TEXCOORD0;
	float4 T1 : TEXCOORD1;
	float4 T2 : TEXCOORD2;
	float4 T3 : TEXCOORD3;
	float4 T4 : TEXCOORD4;
	float4 T5 : TEXCOORD5;
	float4 T6 : TEXCOORD6;
	float4 T7 : TEXCOORD7;
};

[maxvertexcount(6)]
void main(triangle VERT tri[3], inout TriangleStream<VERT> triStream)
{
	triStream.Append(tri[0]);
	triStream.Append(tri[1]);
	triStream.Append(tri[2]);
}