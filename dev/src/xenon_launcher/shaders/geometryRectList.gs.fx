struct VERT
{
	float4 Pos : SV_Position;
	float4 T0 : TEXCOORD0;
	//float4 T1 : TEXCOORD1;
	//float4 T2 : TEXCOORD2;
	/*float4 T3 : TEXCOORD3;
	float4 T4 : TEXCOORD4;
	float4 T5 : TEXCOORD5;
	float4 T6 : TEXCOORD6;
	float4 T7 : TEXCOORD7;*/
};

VERT ComputeCorner(const VERT a, const VERT b, const VERT c)
{
	VERT ret;
	ret.Pos = -a.Pos + b.Pos + c.Pos;
	ret.T0 = -a.T0 + b.T0 + c.T0;
	//ret.T1 = -a.T1 + b.T1 + c.T1;
	//ret.T2 = -a.T2 + b.T2 + c.T2;
	return ret;
}

[maxvertexcount(6)]
void main(triangle VERT tri[3], inout TriangleStream<VERT> triStream)
{
	bool pos = tri[0].Pos.x == tri[2].Pos.x;

	// first three vertices are always outputed the same
	triStream.Append(tri[0]);
	triStream.Append(tri[1]);
	triStream.Append(tri[2]);

	//  0 ------ 1
	//  |      - |
	//  |   //   |
	//  | -      |
	//  2 ----- [3]
	if (pos)
	{
		// second triangle
		triStream.RestartStrip();
		triStream.Append(tri[2]);
		triStream.Append(tri[1]);

		// last vertex is computed
		triStream.Append(ComputeCorner(tri[0], tri[1], tri[2]));
	}

	//  0 ------ 1
	//  | -      |
	//  |   \\   |
	//  |      - |
	// [3] ----- 2
	else
	{
		// second triangle
		triStream.RestartStrip();
		triStream.Append(tri[0]);
		triStream.Append(tri[2]);

		// last vertex is computed
		triStream.Append(ComputeCorner(tri[1], tri[0], tri[2]));
	}
}