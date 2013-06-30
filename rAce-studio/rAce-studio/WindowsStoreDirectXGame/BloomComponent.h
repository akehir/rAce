#pragma once

#include "IGameResourcesComponent.h"
#include "IGameRenderComponent.h"
#include "BloomCBuffers.h"
#include "RenderTarget2D.h"

// Forward declaration of the Game class.
ref class Game;

// This is a simple class designed to demonstrate how to apply the bloom postprocessing technique to a rendered scene. In my testing, I get
// approximately 15 fps with a scale factor of 0.5f on my Surface RT and approximately 20 fps with a scale factor of 0.25f. The most likely 
// reason is fillrate or a low number of pixel shaders/unified shaders, which could be addressed by reducing the size of the fixed back buffer
// or implementing an alternate blur method which could be done in a single pass.
class BloomComponent sealed : public IGameResourcesComponent, public IGameRenderComponent
{
public:
	// Constructor.
	// renderTargetScaleFactor - The scale (relative to either the fixed back buffer or the window (if no fixed back buffer is used)) used to size the intermediate render targets. Should be between 1.0f and 0.25f for adequate results. 0.5f is the default though you should consider 0.25f if the game is running on a low power system to improve framerates. You can change this with SetRenderTargetScaleFactor.
	BloomComponent(
		_In_ float renderTargetScaleFactor = 0.5f
		);

	// Sets the uniform scale of the two intermediate render targets that are used by the BloomComponent. Must be called before
	// CreateDeviceResources in order to have any effect.
	// scaleFactor - The scale (relative to either the fixed back buffer or the window (if no fixed back buffer is used)) used to size the intermediate render targets. Should be between 1.0f and 0.25f for adequate results. 0.5f is the default though you should consider 0.25f if the game is running on a low power system to improve framerates.
	void SetRenderTargetScaleFactor(
		_In_ float renderTargetScaleFactor
		) { m_renderTargetScaleFactor = renderTargetScaleFactor; }

	// Creates resources that do not depend on the D3D device.
	virtual Windows::Foundation::IAsyncActionWithProgress<int>^ CreateDeviceIndependentResources(
		_In_ Game^ game
		);

	// Creates D3D device resources that do not depend on the windows size.
	virtual Windows::Foundation::IAsyncActionWithProgress<int>^ CreateDeviceResources(
		_In_ Game^ game
		);

	// Creates D3D device resources that do depend on the window size.
	virtual Windows::Foundation::IAsyncActionWithProgress<int>^ CreateWindowSizeDependentResources(
		_In_ Game^ game
		);

	// Applies the bloom component to the return value of Game::GetCurrentRenderTargetSRV and renders the result to the rendertarget set by Game::SetBackBuffer.
	virtual void Render(
		_In_ Game^ game,
		_In_ float timeTotal,
		_In_ float timeDelta
		);

	// The following are getters and setters for various bloom parameters.

	// The blur amount controls how far around a "bloomed" pixel the hazy glow effect that bloom can create should spread. A value of 1.0f means no hazy glow. A value of
	// 8.0f means a LOT of hazy glow.

	float GetBlurAmount() { return m_blurAmount; }
	void SetBlurAmount(float value) { m_blurAmount = value; }

	// Bloom threshold is the 0.0f to 1.0f threshold value of a pixel's RGB viewed as a unorm float. A value of 0.0f means to bloom every pixel in the scene, no matter
	// its brightness. The closer you get to 1.0f, the brighter a pixel needs to be in order to meet the threshold for having bloom applied to it. 1.0f means
	// just white (#FFFFFF) pixels will be bloomed. Alpha is disregarded in calculating whether a pixel meets the threshold.

	float GetBloomThreshold() { return m_extractCBufferChangesEveryFrame.BloomThreshold; }
	void SetBloomThreshold(float value) { m_extractCBufferChangesEveryFrame.BloomThreshold = value; }

	// Bloom intensity controls the intensity level of the color sampled from the resulting bloom texture. Typical values are between 1.0f and 2.0f. For more info,
	// have a look at the BloomCombinePixelShader.hlsl shader file.

	float GetBloomIntensity() { return m_combineCBufferChangesEveryFrame.CombineValues.x; }
	void SetBloomIntensity(float value) { m_combineCBufferChangesEveryFrame.CombineValues.x = value; }

	// Base intensity controls the intensity level of the color sampled from the pre-bloom (i.e. base) texture. Typically this value is 1.0f, but you might choose to 
	// lower it slightly to darken the scene in your game. For more info, have a look at the BloomCombinePixelShader.hlsl shader file.

	float GetBaseIntensity() { return m_combineCBufferChangesEveryFrame.CombineValues.y; }
	void SetBaseIntensity(float value) { m_combineCBufferChangesEveryFrame.CombineValues.y = value; }

	// Bloom saturation controls the saturation (grey to full original color value) of the color sampled from the resulting bloom texture. Typically this is 1.0f, but 
	// you might go higher (e.g. up to 2.0f) to really saturate the scene, or lower (e.g. down to 0.0f) to heavily desaturate the scene. For more info, have a look at 
	// the BloomCombinePixelShader.hlsl shader file.

	float GetBloomSaturation() { return m_combineCBufferChangesEveryFrame.CombineValues.z; }
	void SetBloomSaturation(float value) { m_combineCBufferChangesEveryFrame.CombineValues.z = value; }

	// Base saturation controls the saturation (grey to full original color value) of the color sampled from the base texture. Typically this is 1.0f, but you might
	// go lower (e.g. down to 0.0f) to heavily saturate the scene (provided you use a higher value for BloomSaturation). For more info, have a look at the
	// BloomCombinePixelShader.hlsl shader file.

	float GetBaseSaturation() { return m_combineCBufferChangesEveryFrame.CombineValues.w; }
	void SetBaseSaturation(float value) { m_combineCBufferChangesEveryFrame.CombineValues.w = value; }

	// Controls whether the component is enabled.

	bool GetBloomIsEnabled() { return m_bloomIsEnabled; }
	void SetBloomIsEnabled(bool value) { m_bloomIsEnabled = value; }

private:
	// How much m_renderTargetOne and m_renderTargetTwo should be scaled compared to the original scene's size. Typically 0.5f but the sample uses 0.25f as the default
	// when compiled for ARM since ARM chips in current tablets tend to have low powered GPUs (good for long battery life) which will limit the amount of overdraw in a scene.
	// Taking a 1366x768 buffer and using two 341x192 intermediate buffers rather than 683x381 results in a savings for the 3 passes which draw to these targets of
	// 584253 pixel shader invocations (((683 * 381) - (341 * 192)) * 3) and the corresponding memory writes, which helps resolve performance issues caused by a low fillrate or
	// by a low number of pixel shaders/unified shaders.
	float												m_renderTargetScaleFactor;

	// The scene is rendered to this target in its unmodified form so that it can be combined with the results of the bloom and drawn to the back buffer. This could be skipped
	// if your graphics pipeline was modified so that the scene was not rendered to the same render target that the bloomed scene would be drawn to. That is one potential avenue
	// for savings when considering ways to improve bloom performance on lower powered GPUs.
	RenderTarget2D										m_sceneRenderTarget;

	// This is an intermediate render target. It is used to create the brightness texture and in blurring the scene.
	RenderTarget2D										m_renderTargetOne;
	// This is an intermediate render target. It is used in blurring the scene.
	RenderTarget2D										m_renderTargetTwo;

	// This is the cbuffer used in the "extract" phase of the bloom process.
	Microsoft::WRL::ComPtr<ID3D11Buffer>				m_extractD3DBufferChangesEveryFrame;
	// This is the cbuffer used in the two-step "blur" phase of the bloom process.
	Microsoft::WRL::ComPtr<ID3D11Buffer>				m_blurD3DBufferChangesEveryFrame;
	// This is the cbuffer used in the "combine" phase of the bloom process.
	Microsoft::WRL::ComPtr<ID3D11Buffer>				m_combineD3DBufferChangesEveryFrame;

	// This is the pixel shader that extracts the pixels in the scene that exceed the specified brightness threshold (and thus that will be subject to the remaining parts of the
	// bloom process.
	Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_extractPixelShader;
	// This is the pixel shader that performs a gaussian blur on the brightness texture created in the extract pass. It runs twice: once to blur horizontally and once to blur
	// vertically (the order does not matter; in the sample we do horizontal then vertical but it could be vertical then horizontal with the same result).
	Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_blurPixelShader;
	// This is the pixel shader that combines the base scene and the bloom texture (the brightness texture after it has been blurred) into the final bloomed scene. Various
	// parameters control the saturation and intensity of both the base scene and the bloom texture in order to produce the desired effect. As the sample demonstrates, many
	// different looks can be achieved with bloom, not just the typical "saturated and glowing" look that is most commonly associated with bloom postprocessing.
	Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_combinePixelShader;

	// This is a CPU side representation of the extract cbuffer. Its value(s) can be modified and then be used to update the cbuffer on the GPU side before the scene is drawn.
	// In this sample it updates every frame, but you could change it to only update when the value(s) change on the CPU side.
	BloomExtractCBufferChangesEveryFrame				m_extractCBufferChangesEveryFrame;
	// This is a CPU side representation of the blur cbuffer. Its value(s) can be modified and then be used to update the cbuffer on the GPU side before the scene is drawn.
	// In this sample it updates twice every frame, once for the horizontal pass then again for the vertical pass. If a second cbuffer for blur were added and a second instance
	// of this struct, then you could change things to only update the horizontal/vertical buffer when the values change on the CPU side.
	BloomBlurCBufferChangesEveryFrame					m_blurCBufferChangesEveryFrame;
	// This is a CPU side representation of the combine cbuffer. Its value(s) can be modified and then be used to update the cbuffer on the GPU side before the scene is drawn.
	// In this sample it updates every frame, but you could change it to only update when the value(s) change on the CPU side.
	BloomCombineCBufferChangesEveryFrame				m_combineCBufferChangesEveryFrame;

	// This is the amount to blur the brightness texture. A value of 1.0f means don't blur it. Typical values are whole numbers between 2.0f and 8.0f.
	float												m_blurAmount;
	
	// Stores whether or not a fixed back buffer is being used. This prevents us from hitting an error if the Game class is changed to use a fixed back buffer but
	// only the window sized resources (rather than all graphics device resources) are recreated. This would be exceedingly rare but it's easy enough to prevent.
	bool												m_usingFixedBackBuffer;

	// Stores whether or not the bloom component is enabled.
	bool												m_bloomIsEnabled;
};
