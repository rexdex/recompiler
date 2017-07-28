void main(uint idx : SV_VertexID, float4 pos : SV_Position)
{
	pos.x = (idx & 1) ? -1.0f : 1.0f;
	pos.y = (idx & 2) ? -1.0f : 1.0f;
	pos.z = 0.5f;
	pos.w = 1.0f;
}