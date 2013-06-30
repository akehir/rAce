#pragma once
#include <DirectXMath.h>

struct BloomExtractCBufferChangesEveryFrame
{
	BloomExtractCBufferChangesEveryFrame() :
		BloomThreshold()
	{ }

	BloomExtractCBufferChangesEveryFrame(
		_In_ float bloomThreshold
		) :	
	BloomThreshold(bloomThreshold)
	{ }

	BloomExtractCBufferChangesEveryFrame(const BloomExtractCBufferChangesEveryFrame& other) :
		BloomThreshold(other.BloomThreshold)
	{ }

	BloomExtractCBufferChangesEveryFrame& operator=(const BloomExtractCBufferChangesEveryFrame& other)
	{
		BloomThreshold = other.BloomThreshold;
		return *this;
	}

	// The minimum brightness of a pixel in order for bloom to apply to it on a 0.0f (all pixels) to 1.0f (only 100% white pixels) scale.
	float BloomThreshold;
};

// BLUR_SAMPLE_COUNT must match the value in BloomBlurPixelShader.hlsl.
const int BLUR_SAMPLE_COUNT = 15;

struct BloomBlurCBufferChangesEveryFrame
{
	DirectX::XMFLOAT4 SampleOffsetsAndWeights[BLUR_SAMPLE_COUNT];
};

struct BloomCombineCBufferChangesEveryFrame
{
	BloomCombineCBufferChangesEveryFrame() :
		CombineValues()
	{ }

	BloomCombineCBufferChangesEveryFrame(
		_In_ float bloomIntensity,
		_In_ float baseIntensity,
		_In_ float bloomSaturation,
		_In_ float baseSaturation) :
	CombineValues(bloomIntensity, baseIntensity, bloomSaturation, baseSaturation)
	{ }

	BloomCombineCBufferChangesEveryFrame(const BloomCombineCBufferChangesEveryFrame& other) :
		CombineValues(other.CombineValues)
	{ }

	BloomCombineCBufferChangesEveryFrame& operator=(const BloomCombineCBufferChangesEveryFrame& other)
	{
		CombineValues = other.CombineValues;
		return *this;
	}

	// x - BloomIntensity ; y - BaseIntensity ; z - BloomSaturation ; w - BaseSaturation
	DirectX::XMFLOAT4 CombineValues;
};

