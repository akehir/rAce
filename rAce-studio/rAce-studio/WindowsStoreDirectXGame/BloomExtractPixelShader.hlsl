// Pixel shader extracts the brighter areas of an image.
// This is the first step in applying a bloom postprocess.

Texture2D g_texture : register(t0);
SamplerState g_samplerLinear : register(s0);

// The cbuffer used to specify the bloom threshold value.
cbuffer cbPerFrame : register(b0)
{
    float BloomThreshold : packoffset(c0);
}

float4 main(float4 color : COLOR0,
			float2 texCoord : TEXCOORD0) : SV_TARGET
{
    // Look up the original image color.
    float4 sampledColor = g_texture.Sample(g_samplerLinear, texCoord);
    
    // Adjust it to keep only values brighter than the specified threshold. Use saturate to clamp the values
	// within unorm range.
    return float4(saturate((sampledColor.rgb - BloomThreshold) / (1 - BloomThreshold)), sampledColor.a);
}
