SamplerState samplerLinear : register(s0);
SamplerState samplerPoint  : register(s1);

Texture2D<float4> mainTexture[(1<<16)] : register(t0);
struct VertexOut {
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float4 color : COLOR;
	uint textureId : TEXCOORD1;
};

float4 main(VertexOut vtx) : SV_TARGET {
	return mainTexture[vtx.textureId].SampleLevel(samplerPoint, vtx.texCoord, 0) * vtx.color;
}
