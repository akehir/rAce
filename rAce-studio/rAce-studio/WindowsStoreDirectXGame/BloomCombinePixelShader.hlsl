// Pixel shader combines the bloom image with the original
// scene, using tweakable intensity levels and saturation.
// This is the final step in applying a bloom postprocess.

Texture2D g_bloomTexture : register(t0);
Texture2D g_baseTexture : register(t1);

SamplerState g_samplerLinear : register(s0);

cbuffer cbPerFrame : register(b0)
{
    float BloomIntensity :  packoffset(c0.x);
    float BaseIntensity :   packoffset(c0.y);

    float BloomSaturation : packoffset(c0.z);
    float BaseSaturation :  packoffset(c0.w);
}

// Helper for modifying the saturation of a color.
float4 AdjustSaturation(float4 color, float saturation)
{
    // The constants 0.3, 0.59, and 0.11 are chosen because the
    // human eye is more sensitive to green light, and less to blue.
    float grey = dot((float3)color, float3(0.3, 0.59, 0.11));

    return lerp(grey, color, saturation);
}


float4 main(float4 color : COLOR0,
			float2 texCoord : TEXCOORD0) : SV_TARGET
{
    // Look up the bloom and original base image colors.
    float4 bloom = g_bloomTexture.Sample(g_samplerLinear, texCoord);

    float4 base = g_baseTexture.Sample(g_samplerLinear, texCoord);

    // Adjust color saturation and intensity.
    bloom = AdjustSaturation(bloom, BloomSaturation) * BloomIntensity;
    base = AdjustSaturation(base, BaseSaturation) * BaseIntensity;

    // Darken down the base image in areas where there is a lot of bloom,
    // to prevent things looking excessively burned-out.
    base *= (1 - saturate(bloom));

    // Combine the two images.
    return base + bloom;
}
