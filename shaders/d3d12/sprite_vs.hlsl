const float2 resolution : register(b0);

struct VertexIn {
	float2 position : POSITION;
	float2 texCoord : TEXCOORD;
	float4 color : COLOR;
	uint textureId : TEXCOORD1;
};

struct VertexOut {
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float4 color : COLOR;
	uint textureId : TEXCOORD1;
};

VertexOut main(VertexIn vtx) {
	VertexOut vtxOut;
	vtxOut.position = float4((vtx.position / resolution) * 2.0 - 1.0, 0.0, 1.0);
	vtxOut.position.y = -vtxOut.position.y;
	vtxOut.texCoord = vtx.texCoord;
	vtxOut.color = vtx.color;
	vtxOut.textureId = vtx.textureId;
	return vtxOut;
}
