#include "pch.h"
#include "BloomComponent.h"
#include "Game.h"

using namespace DirectX;
using namespace WindowsStoreDirectXGame;

BloomComponent::BloomComponent(
	_In_ float renderTargetScaleFactor
	) :
	m_renderTargetScaleFactor(renderTargetScaleFactor),
	m_sceneRenderTarget(),
	m_renderTargetOne(),
	m_renderTargetTwo(),
	m_extractD3DBufferChangesEveryFrame(),
	m_blurD3DBufferChangesEveryFrame(),
	m_combineD3DBufferChangesEveryFrame(),
	m_extractPixelShader(),
	m_blurPixelShader(),
	m_combinePixelShader(),
	m_extractCBufferChangesEveryFrame(0.25f),
	m_blurCBufferChangesEveryFrame(),
	m_combineCBufferChangesEveryFrame(1.25f, 1.0f, 1.0f, 1.0f),
	m_blurAmount(4.0f),
	m_usingFixedBackBuffer()
{
#if defined(_M_ARM)
	m_renderTargetScaleFactor = 0.25f;
#endif
}

Windows::Foundation::IAsyncActionWithProgress<int>^ BloomComponent::CreateDeviceIndependentResources(
	_In_ Game^ game
	)
{
	// The bloom component only has resources that depend on the graphics device, so we have nothing to do here other than return an (empty) task.
	return concurrency::create_async([](concurrency::progress_reporter<int> progressReporter, concurrency::cancellation_token cancellationToken)
	{
		// Do nothing.
		return;
	});
}

Windows::Foundation::IAsyncActionWithProgress<int>^ BloomComponent::CreateDeviceResources(
	_In_ Game^ game
	)
{
	// Create an IAsyncActionWithProgress<int> by using create_async and passing in a progress_reporter<int> and ensuring that we don't try to return any
	// value (otherwise it'd be a an IAsyncOperationWithProgress<TResult, TProgress>.
	return concurrency::create_async([this, game](concurrency::progress_reporter<int> progressReporter, concurrency::cancellation_token cancellationToken)
	{
		// This is a simple progress counter. The returned numbers could be cached and each time an increment happens, an indefinite progress indicator
		// could be updated. See the notes for IGameResourcesComponent for more about (real) ways to use progress reporting.
		int progress = 0;

		// We need to create several cbuffers. These first two parameters will be the same for all of them, with only the size changing for each.
		D3D11_BUFFER_DESC cbufferDesc = {};
		cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbufferDesc.Usage = D3D11_USAGE_DEFAULT;

		auto device = game->GetD3DDevice();

		// ID3D11Buffer objects must be sized in increments of 16 bytes. The (size + 15) / 16 * 16 math here uses the truncation "rounding" of integer math to
		// ensure that we wind up with a buffer that is large enough to contain all of the data without being excessively large.
		cbufferDesc.ByteWidth = (sizeof(BloomExtractCBufferChangesEveryFrame) + 15) / 16 * 16;
		
		// Create the buffer.
		DX::ThrowIfFailed(
			device->CreateBuffer(&cbufferDesc, nullptr, &m_extractD3DBufferChangesEveryFrame), __FILEW__, __LINE__
			);

		// Report some progress. As this is our first report and we're using the ++ prefix operator, the returned value will be 1.
		progressReporter.report(++progress);

		// Check to see if cancellation is requested. If so, cancel the task. 
		if (cancellationToken.is_canceled())
		{
			// Note: cancel_current_task takes care of returning properly so all you need to do is call it if cancellation is needed and it does the rest.
			concurrency::cancel_current_task();
		}

		// Determine the proper size and then create the next buffer.
		cbufferDesc.ByteWidth = (sizeof(BloomBlurCBufferChangesEveryFrame) + 15) / 16 * 16;
		DX::ThrowIfFailed(
			device->CreateBuffer(&cbufferDesc, nullptr, &m_blurD3DBufferChangesEveryFrame), __FILEW__, __LINE__
			);

		// Update progress then cancel if requested.
		progressReporter.report(++progress);

		if (cancellationToken.is_canceled())
		{
			concurrency::cancel_current_task();
		}

		// Determine the proper size and then create the next buffer.
		cbufferDesc.ByteWidth = (sizeof(BloomCombineCBufferChangesEveryFrame) + 15) / 16 * 16;
		DX::ThrowIfFailed(
			device->CreateBuffer(&cbufferDesc, nullptr, &m_combineD3DBufferChangesEveryFrame), __FILEW__, __LINE__
			);

		// Update progress then cancel if requested.
		progressReporter.report(++progress);

		if (cancellationToken.is_canceled())
		{
			concurrency::cancel_current_task();
		}

		// BasicLoader provides a simple way to load pre-compiled shaders.
		auto loader = ref new BasicLoader(device);

		// Load the bloom extract pixel shader.
		loader->LoadShader("BloomExtractPixelShader.cso", &m_extractPixelShader);
		progressReporter.report(++progress);

		if (cancellationToken.is_canceled())
		{
			concurrency::cancel_current_task();
		}

		// Load the bloom blur pixel shader.
		loader->LoadShader("BloomBlurPixelShader.cso", &m_blurPixelShader);
		progressReporter.report(++progress);

		if (cancellationToken.is_canceled())
		{
			concurrency::cancel_current_task();
		}

		// Load the bloom combine pixel shader.
		loader->LoadShader("BloomCombinePixelShader.cso", &m_combinePixelShader);

		progressReporter.report(++progress);

		if (cancellationToken.is_canceled())
		{
			concurrency::cancel_current_task();
		}

		if (game->IsUsingFixedBackBuffer())
		{
			// If the game is using a fixed back buffer we want to create the bloom render targets here to avoid needing to recreate them whenever the
			// window size changes.
			m_usingFixedBackBuffer = true;

			auto fixedBackBufferSize = game->GetFixedBackBufferSize();

			auto width = static_cast<UINT>(fixedBackBufferSize.Width);
			auto height = static_cast<UINT>(fixedBackBufferSize.Height);

			// Create the scene render target, which is used to cache the pre-bloomed (base) scene since we'd likely be rendering to the same resource
			// that the scene currently exists in when we're ready to draw the result of the bloom operation.
			m_sceneRenderTarget.CreateRenderTarget(device, width, height);

			progressReporter.report(++progress);

			if (cancellationToken.is_canceled())
			{
				concurrency::cancel_current_task();
			}

			// Scale down the width and height for the two intermediate render targets.
			width = static_cast<UINT>(width * m_renderTargetScaleFactor);
			height = static_cast<UINT>(height * m_renderTargetScaleFactor);

			m_renderTargetOne.CreateRenderTarget(device, width, height);

			progressReporter.report(++progress);

			if (cancellationToken.is_canceled())
			{
				concurrency::cancel_current_task();
			}

			m_renderTargetTwo.CreateRenderTarget(device, width, height);
		}
		else
		{
			m_usingFixedBackBuffer = false;

			// Assume we aren't using a fixed back buffer. Destroy any existing bloom render targets since every call to CreateDeviceResources should be followed
			// by a call to CreateWindowSizeDependentResources.
			m_sceneRenderTarget.Reset();
			progressReporter.report(++progress);

			m_renderTargetOne.Reset();
			progressReporter.report(++progress);

			m_renderTargetTwo.Reset();
		}
	});
}

Windows::Foundation::IAsyncActionWithProgress<int>^ BloomComponent::CreateWindowSizeDependentResources(
	_In_ Game^ game
	)
{
	return concurrency::create_async([this, game](concurrency::progress_reporter<int> progressReporter, concurrency::cancellation_token cancellationToken)
	{
		// Simple, indefinite progress reporter via an incrementing int.
		int progress = 0;

		// We only have something to do if the last call to CreateDeviceResources said that we aren't using a fixed back buffer.
		if (!m_usingFixedBackBuffer)
		{
			auto windowSize = game->GetWindowSize();

			auto device = game->GetD3DDevice();
			auto width = static_cast<UINT>(windowSize.Width);
			auto height = static_cast<UINT>(windowSize.Height);

			// Create the scene render target.
			m_sceneRenderTarget.CreateRenderTarget(device, width, height);

			progressReporter.report(++progress);

			if (cancellationToken.is_canceled())
			{
				concurrency::cancel_current_task();
			}

			width = static_cast<UINT>(width * m_renderTargetScaleFactor);
			height = static_cast<UINT>(height * m_renderTargetScaleFactor);

			// Create the intermediate render targets.
			m_renderTargetOne.CreateRenderTarget(device, width, height);

			progressReporter.report(++progress);

			if (cancellationToken.is_canceled())
			{
				concurrency::cancel_current_task();
			}

			m_renderTargetTwo.CreateRenderTarget(device, width, height);

			progressReporter.report(++progress);

			if (cancellationToken.is_canceled())
			{
				concurrency::cancel_current_task();
			}
		}
		else
		{
			// Do nothing. If you were using a definite progress tracking in, you'd need to make an equal
			// number of progress reports here to the number made in the if branch above.
		}
	});
}

void BloomComponent::Render(
	_In_ Game^ game,
	_In_ float timeTotal,
	_In_ float timeDelta
	)
{
	// Render doesn't use the timeTotal or timeDelta so used UNREFERENCED_PARAMETER to avoid warnings since this is an interface implementation and must have them.
	UNREFERENCED_PARAMETER(timeTotal);
	UNREFERENCED_PARAMETER(timeDelta);

	// If the game isn't currently using bloom, there's nothing for us to do here.
	if (m_bloomIsEnabled)
	{
		return;
	}

	// This creates an array of ID3D11ShaderResourceView pointers with the sole member being a nullptr. This lets us clear the resources we bound to the pixel shader
	// before using them as render targets since if they were still bound to the graphics pipeline it would generate a D3D warning (and then would unbind them for us).
	ID3D11ShaderResourceView* const nullSRV[] = { nullptr };

	auto context = game->GetImmediateContext();
	auto spriteBatch = game->GetSpriteBatch();
	auto commonStates = game->GetCommonStates();

	// Duplicate the contents of renderTargetSRV to a holding texture.
	context->OMSetRenderTargets(1, m_sceneRenderTarget.GetRTV(), nullptr);

	// We use an opaque blend state and tell SpriteBatch to disregard depth so that we can avoid clearing the rendertarget and depth stencil buffer. This saves us
	// quite a bit of rendering time when working with GPUs that have a low fillrate. Even if there are transparent areas of the existing scene, using opaque will
	// just copy those transparent values over such that they'd still be transparent for anything that followed bloom.
	spriteBatch->Begin(DirectX::SpriteSortMode_Deferred, commonStates->Opaque(), nullptr, commonStates->DepthNone());
	spriteBatch->Draw(game->GetCurrentRenderTargetSRV(), DirectX::XMFLOAT2(0.0f, 0.0f), nullptr);
	spriteBatch->End();

	// EXTRACT BRIGHTNESS
	{
		// We begin by updating the extract cbuffer using the latest values in the CPU side representation of the cbuffer. UpdateSubresource can also be used to
		// update textures and other resources, which is why it has all the extra parameters that we, updating a cbuffer, pass null and 0 to (whichever is appropriate).
		context->UpdateSubresource(m_extractD3DBufferChangesEveryFrame.Get(), 0, nullptr, &m_extractCBufferChangesEveryFrame, 0, 0);

		// Set the appropriate render target.
		context->OMSetRenderTargets(1, m_renderTargetOne.GetRTV(), nullptr);
		
		// Get the pixel shader and buffer in variables that can be easily captured by a lambda.
		auto pixelShader = m_extractPixelShader.Get();
		auto buffer = m_extractD3DBufferChangesEveryFrame.GetAddressOf();
		
		// Our call to SpriteBatch::Begin uses a lambda to set a custom pixel shader and the cbuffer that the shader requires. Since SpriteBatch works by batching up
		// all draw calls then submitting them in as few draw calls as possible, the lambda lets SpriteBatch set the right shaders when it is ready to draw rather than
		// using its defaults. Since we don't set a vertex shader, we just default to SpriteBatch's default vertex shader.
		spriteBatch->Begin(DirectX::SpriteSortMode_Deferred, commonStates->Opaque(), nullptr, commonStates->DepthNone(), nullptr, [context, pixelShader, buffer]()
		{
			context->PSSetShader(pixelShader, nullptr, 0);
			context->PSSetConstantBuffers(0, 1, buffer);
		});

		// We'll be scaling down the scene to fit in our (likely) smaller intermediate render target. The easiest way to do this is with a destination RECT.
		RECT rect = { 0L, 0L, static_cast<LONG>(m_renderTargetOne.GetWidth()), static_cast<LONG>(m_renderTargetOne.GetHeight()) };
		spriteBatch->Draw(m_sceneRenderTarget.GetSRV(), rect, nullptr);
		spriteBatch->End();
	}

	// BLUR HORIZONTALLY
	{
		float gaussian;
		float offset = 0.0f;
		float deltaX;
		float deltaY;

		deltaX = 1.0f / m_renderTargetOne.GetWidth();
		deltaY = 0.0f;

		// sigma is the blur amount.
		auto sigmaSquared = m_blurAmount * m_blurAmount;

		// Calculate the one dimensional gaussian blur weight value for the given offset. (See: http://en.wikipedia.org/wiki/Gaussian_blur ).
		gaussian = static_cast<float>((1.0 / sqrtf(2.0f * XM_PI * sigmaSquared)) *
			expf(-(offset * offset) / (2 * sigmaSquared)));

		m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[0].x = 0.0f;
		m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[0].y = 0.0f;
		m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[0].z = gaussian;

		// The number of samples used in blurring each pixel.
		int sampleCount = BLUR_SAMPLE_COUNT;

		// Running total of weights.
		float totalWeights = gaussian;

		// Add pairs of additional sample taps, positioned
		// along a line in both directions from the center.
		for (int i = 0; i < sampleCount / 2; i++)
		{
			// Store weights for the positive and negative taps.
			offset = static_cast<float>(i + 1);

			// Calculate the one dimensional gaussian blur weight value for the given offset.
			gaussian = static_cast<float>((1.0 / sqrtf(2.0f * XM_PI * sigmaSquared)) *
				expf(-(offset * offset) / (2 * sigmaSquared)));

			float weight = gaussian;

			m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[i * 2 + 1].z = weight;
			m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[i * 2 + 2].z = weight;

			totalWeights += weight * 2;

			// To get the maximum amount of blurring from a limited number of
			// pixel shader samples, we take advantage of the bilinear filtering
			// hardware inside the texture fetch unit. If we position our texture
			// coordinates exactly halfway between two texels, the filtering unit
			// will average them for us, giving two samples for the price of one.
			// This allows us to step in units of two texels per sample, rather
			// than just one at a time. The 1.5 offset kicks things off by
			// positioning us nicely in between two texels.
			float sampleOffset = i * 2 + 1.5f;

			XMFLOAT2 delta(deltaX * sampleOffset, deltaY * sampleOffset);

			// Store texture coordinate offsets for the positive and negative taps.
			m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[i * 2 + 1].x = delta.x;
			m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[i * 2 + 1].y = delta.y;
			m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[i * 2 + 2].x = -delta.x;
			m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[i * 2 + 2].y = -delta.y;
		}

		// Normalize the list of sample weightings, so they will always sum to one.
		for (int i = 0; i < BLUR_SAMPLE_COUNT; i++)
		{
			m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[i].z /= totalWeights;
		}

		// Update the blur cbuffer with the horizontal blur values.
		context->UpdateSubresource(m_blurD3DBufferChangesEveryFrame.Get(), 0, nullptr, &m_blurCBufferChangesEveryFrame, 0, 0);

		context->OMSetRenderTargets(1, m_renderTargetTwo.GetRTV(), nullptr);
		auto pixelShader = m_blurPixelShader.Get();
		auto buffer = m_blurD3DBufferChangesEveryFrame.GetAddressOf();
		spriteBatch->Begin(DirectX::SpriteSortMode_Deferred, commonStates->Opaque(), nullptr, commonStates->DepthNone(), nullptr, [context, pixelShader, buffer]()
		{
			context->PSSetShader(pixelShader, nullptr, 0);
			context->PSSetConstantBuffers(0, 1, buffer);
		});
		spriteBatch->Draw(m_renderTargetOne.GetSRV(), DirectX::XMFLOAT2(0.0f, 0.0f), nullptr);
		spriteBatch->End();

		// Unbind m_renderTargetOne from the graphics pipeline so we can bind it as a render target in the next pass without any warnings.
		context->PSSetShaderResources(0, 1, nullSRV);
	}

	// BLUR VERTICALLY
	{
		float gaussian;
		float offset = 0.0f;
		float deltaX;
		float deltaY;

		deltaX = 0.0f;
		deltaY = 1.0f / m_renderTargetOne.GetHeight();

		// sigma is the blur amount.
		auto sigmaSquared = m_blurAmount * m_blurAmount;

		gaussian = static_cast<float>((1.0 / sqrtf(2.0f * XM_PI * sigmaSquared)) *
			expf(-(offset * offset) / (2 * sigmaSquared)));

		m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[0].x = 0.0f;
		m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[0].y = 0.0f;
		m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[0].z = gaussian;
		int sampleCount = BLUR_SAMPLE_COUNT;

		// Running total of weights.
		float totalWeights = gaussian;

		// Add pairs of additional sample taps, positioned
		// along a line in both directions from the center.
		for (int i = 0; i < sampleCount / 2; i++)
		{
			// Store weights for the positive and negative taps.
			offset = static_cast<float>(i + 1);

			gaussian = static_cast<float>((1.0 / sqrtf(2.0f * XM_PI * sigmaSquared)) *
				expf(-(offset * offset) / (2 * sigmaSquared)));

			float weight = gaussian;

			m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[i * 2 + 1].z = weight;
			m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[i * 2 + 2].z = weight;

			totalWeights += weight * 2;

			// To get the maximum amount of blurring from a limited number of
			// pixel shader samples, we take advantage of the bilinear filtering
			// hardware inside the texture fetch unit. If we position our texture
			// coordinates exactly halfway between two texels, the filtering unit
			// will average them for us, giving two samples for the price of one.
			// This allows us to step in units of two texels per sample, rather
			// than just one at a time. The 1.5 offset kicks things off by
			// positioning us nicely in between two texels.
			float sampleOffset = i * 2 + 1.5f;

			XMFLOAT2 delta(deltaX * sampleOffset, deltaY * sampleOffset);

			// Store texture coordinate offsets for the positive and negative taps.
			m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[i * 2 + 1].x = delta.x;
			m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[i * 2 + 1].y = delta.y;
			m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[i * 2 + 2].x = -delta.x;
			m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[i * 2 + 2].y = -delta.y;
		}

		// Normalize the list of sample weightings, so they will always sum to one.
		for (int i = 0; i < BLUR_SAMPLE_COUNT; i++)
		{
			m_blurCBufferChangesEveryFrame.SampleOffsetsAndWeights[i].z /= totalWeights;
		}

		context->UpdateSubresource(m_blurD3DBufferChangesEveryFrame.Get(), 0, nullptr, &m_blurCBufferChangesEveryFrame, 0, 0);

		context->OMSetRenderTargets(1, m_renderTargetOne.GetRTV(), nullptr);

		auto pixelShader = m_blurPixelShader.Get();
		auto buffer = m_blurD3DBufferChangesEveryFrame.GetAddressOf();
		spriteBatch->Begin(DirectX::SpriteSortMode_Deferred, commonStates->Opaque(), nullptr, commonStates->DepthNone(), nullptr, [context, pixelShader, buffer]()
		{
			context->PSSetShader(pixelShader, nullptr, 0);
			context->PSSetConstantBuffers(0, 1, buffer);
		});
		spriteBatch->Draw(m_renderTargetTwo.GetSRV(), DirectX::XMFLOAT2(0.0f, 0.0f), nullptr);
		spriteBatch->End();
		context->PSSetShaderResources(0, 1, nullSRV);
	}

	// The final stage of bloom is to combine the bloom effect results with the original scene and draw it onto the back buffer(s).
	// Here we do that, while also allowing for a simple pass-through in case we want to disable bloom for any reason. 
	{
		game->SetBackBuffer();

		// COMBINE

		context->UpdateSubresource(m_combineD3DBufferChangesEveryFrame.Get(), 0, nullptr, &m_combineCBufferChangesEveryFrame, 0, 0);

		auto pixelShader = m_combinePixelShader.Get();
		auto buffer = m_combineD3DBufferChangesEveryFrame.GetAddressOf();
		auto secondTexture = m_sceneRenderTarget.GetSRV();

		// The combine operation uses a second texture, the base scene, and performs all blending between the two within the pixel shader itself. So we can still use an opaque
		// state and we just need to bind the base scene (stored in m_sceneRenderTarget) as a second texture (SpriteBatch binds the texture it is drawing as the first shader resource).
		spriteBatch->Begin(DirectX::SpriteSortMode_Deferred, commonStates->Opaque(), nullptr, commonStates->DepthNone(), nullptr, [context, pixelShader, buffer, secondTexture]()
		{
			context->PSSetShader(pixelShader, nullptr, 0);
			context->PSSetConstantBuffers(0, 1, buffer);
			context->PSSetShaderResources(1, 1, &secondTexture);
		});

		// Make sure to scale everything to fit the back buffer.
		auto backBufferSize = game->IsUsingFixedBackBuffer() ? game->GetFixedBackBufferSize() : game->GetWindowSize();
		RECT rect = { 0L, 0L, static_cast<LONG>(backBufferSize.Width), static_cast<LONG>(backBufferSize.Height) };
		spriteBatch->Draw(m_renderTargetOne.GetSRV(), rect, nullptr);
		spriteBatch->End();
		context->PSSetShaderResources(0, 1, nullSRV);
		context->PSSetShaderResources(1, 1, nullSRV);
	}
}
