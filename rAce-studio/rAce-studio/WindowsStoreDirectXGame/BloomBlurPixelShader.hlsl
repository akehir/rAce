// In this shader we implement a gaussian blur. The goal is that if we have an input pixel that is black with nearby pixels that are 
// bright, the nearby pixels will influence the output of this pixel such that the output pixel will have some brightness to it.
// This shader runs twice: once to blur horizontally and then again to blur vertically.

Texture2D g_texture : register(t0);
SamplerState g_samplerLinear : register(s0);

#define SAMPLE_COUNT 15

cbuffer cbPerFrame : register(b0)
{
    // x and y are the offset values ; z is the weight ; w is unused.
    float4 SampleOffsetsAndWeights[SAMPLE_COUNT];
}

float4 main(float4 color : COLOR0,
			float2 texCoord : TEXCOORD0) : SV_TARGET
{
    float4 outputBrightness = 0;

    // Combine a number of weighted image filter taps.
    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        outputBrightness += g_texture.Sample(g_samplerLinear, texCoord + SampleOffsetsAndWeights[i].xy) * SampleOffsetsAndWeights[i].z;
    }

    return outputBrightness;
}
