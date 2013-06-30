#include "pch.h"
#include "DirectXBase.h" 
#include <windows.ui.xaml.media.dxinterop.h>
#include <math.h>

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;

// Constructor.
DirectXBase::DirectXBase() :
	m_window(),
	m_panel(),
	m_device(),
	m_context(),
	m_swapChain(),
	m_renderTargetView(),
	m_renderTargetSRV(),
	m_depthStencilView(),
	m_currentRenderTargetView(),
	m_currentDepthStencilView(),
	m_featureLevel(),
	m_renderTargetSize(),
	m_windowBounds(),
	m_dpi(-1.0f),
	m_orientation(),
	m_windowIsDeactivated(),
	m_windowIsDeactivatedRefCounter(),
	m_gamePaused(),
	m_usesFixedBackBuffer(),
	m_fixedBackBuffer(),
	m_fixedBackBufferMultisampled(),
	m_fixedBackBufferFormat(),
	m_fixedBackBufferDepthStencilFormat(),
	m_usesMultisampledFixedBackBuffer(),
	m_fixedBackBufferMultisamplePreferredCount(1),
	m_fixedBackBufferMultisamplePreferredQuality(0),
	m_spriteBatch(),
	m_commonStates(),
	m_orientationTransform(),
	m_deviceIndependentResourcesLoaded(),
	m_deviceResourcesLoaded(),
	m_windowSizeResourcesLoaded()
{
}

// Initialize the DirectX resources required to run.
void DirectXBase::Initialize(CoreWindow^ window, SwapChainBackgroundPanel^ panel, float dpi)
{
	m_window = window;
	m_panel = panel;

	// Create/Load the resources that don't rely on the graphics device (e.g. audio resources, game data).
	CreateDeviceIndependentResources();

	// Create/Load the resources that rely on the graphics device but not on the window size (e.g. most graphics assets and shaders).
	CreateDeviceResources();

	// Set the dpi. This calls CreateWindowSizeDependentResources, which loads resources that rely on the graphics device and on the
	// window size. This is generally limited to things like window-sized render targets.
	SetDpi(dpi);
}

// Recreate all device resources and set them back to the current state.
void DirectXBase::HandleDeviceLost()
{
	// Get the current system DPI.
	float dpi = DisplayProperties::LogicalDpi;

	// Reset these member variables to ensure that SetDpi recreates all resources.
	m_dpi = -1.0f;
	m_windowBounds.Width = 0;
	m_windowBounds.Height = 0;
	m_swapChain = nullptr;

	// Recreate all the device-dependent resources.
	CreateDeviceResources();

	// SetDpi will recreate all window-size dependent resources while ensuring that we have the correct DPI.
	SetDpi(dpi);
}

// These are the resources required independent of the device.
void DirectXBase::CreateDeviceIndependentResources()
{
	// The sample does nothing here since DirectXBase handles graphics only, with Game handling all other
	// aspects. If you were to use Direct2D or DirectWrite, some of their resources would be created here.
	// We aren't using either here since between DirectXTK and XAML we have all of that functionality covered.
}

// These are the resources that depend on the device.
void DirectXBase::CreateDeviceResources()
{
	// Reset SpriteBatch and CommonStates.
	m_spriteBatch.reset();
	m_commonStates.reset();

	// This flag adds support for surfaces with a different color channel ordering
	// than the API default. It is required for compatibility with Direct2D.
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
	// If the project is in a debug build, enable debugging via SDK Layers with this flag.
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// This array defines the set of DirectX hardware feature levels this app will support.
	// Note the ordering should be preserved.
	// Don't forget to declare your application's minimum required feature level in its
	// description.  All applications are assumed to support 9.1 unless otherwise stated.
	// See the App Certification Requirements for more detail about feature level support.
	// If you want to have your game run on ARM devices, you must support 9.1.
	D3D_FEATURE_LEVEL featureLevels[] = 
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// Create the Direct3D 11 API device object and a corresponding context.
	ComPtr<ID3D11Device> d3dDevice;
	ComPtr<ID3D11DeviceContext> d3dContext;
	DX::ThrowIfFailed(
		D3D11CreateDevice(
		nullptr, // Specify nullptr to use the default adapter.
		D3D_DRIVER_TYPE_HARDWARE,
		0,
		creationFlags, // Set debug and Direct2D compatibility flags.
		featureLevels, // List of feature levels this app can support.
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION, // Always set this to D3D11_SDK_VERSION for Windows Store apps.
		&d3dDevice, // Returns the Direct3D device created.
		&m_featureLevel, // Returns feature level of device created.
		&d3dContext // Returns the device immediate context.
		), __FILEW__, __LINE__
		);

	// Get the Direct3D 11.1 API device and context interfaces.
	DX::ThrowIfFailed(
		d3dDevice.As(&m_device), __FILEW__, __LINE__
		);
	DX::ThrowIfFailed(
		d3dContext.As(&m_context), __FILEW__, __LINE__
		);

	// Create a SpriteBatch instance and a CommonStates instance.
	m_spriteBatch.reset(new DirectX::SpriteBatch(m_context.Get()));
	m_commonStates.reset(new CommonStates(m_device.Get()));
}

// Allocate all memory resources that change on a window SizeChanged event.
void DirectXBase::CreateWindowSizeDependentResources()
{
	// Store the window bounds so the next time we get a SizeChanged event we can
	// avoid rebuilding everything if the size is identical.
	m_windowBounds = m_window->Bounds;

	// Calculate the necessary swap chain and render target size in pixels.
	float windowWidth = ConvertDipsToPixels(m_windowBounds.Width);
	float windowHeight = ConvertDipsToPixels(m_windowBounds.Height);

	// The width and height of the swap chain must be based on the window's
	// landscape-oriented width and height. If the window is in a portrait
	// orientation, the dimensions must be reversed.
	m_orientation = DisplayProperties::CurrentOrientation;
	bool swapDimensions =
		m_orientation == DisplayOrientations::Portrait ||
		m_orientation == DisplayOrientations::PortraitFlipped;
	m_renderTargetSize.Width = swapDimensions ? windowHeight : windowWidth;
	m_renderTargetSize.Height = swapDimensions ? windowWidth : windowHeight;

	if (m_swapChain != nullptr)
	{
		// Clear all references to the existing back buffer to free it.
		ID3D11RenderTargetView* nullViews[] = {nullptr};
		m_context->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
		m_renderTargetSRV = nullptr;
		m_renderTargetView = nullptr;
		m_depthStencilView = nullptr;
		m_context->ClearState();

		// If the swap chain already exists, resize it.
		HRESULT hr = m_swapChain->ResizeBuffers(
			2, // Double-buffered swap chain.
			static_cast<UINT>(m_renderTargetSize.Width),
			static_cast<UINT>(m_renderTargetSize.Height),
			DXGI_FORMAT_B8G8R8A8_UNORM,
			0
			);

		// We can safely attempt to recover from either of these two failure HRESULTS so we 
		// call HandleDeviceLost to make that attempt.
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			HandleDeviceLost();
			return;
		}
		else
		{
			// Call DX::ThrowIfFailed, which will do nothing if all's well (e.g. S_OK) but will
			// throw if there was a failure that we did not already handle.
			DX::ThrowIfFailed(hr, __FILEW__, __LINE__);
		}
	}
	else
	{
		// If the swap chain doesn't exist, create a new one using the same adapter as the existing Direct3D device.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
		swapChainDesc.Width = static_cast<UINT>(m_renderTargetSize.Width); // Match the size of the window.
		swapChainDesc.Height = static_cast<UINT>(m_renderTargetSize.Height);
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // This is the most common swap chain format.
		swapChainDesc.Stereo = false; 
		swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
		swapChainDesc.BufferCount = 2; // Use double-buffering to minimize latency. Must be at least 2 for FLIP_SEQUENTIAL to do what is intended.
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH; // CreateSwapChainForComposition (which we use) requires DXGI_SCALING_STRETCH.
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
		swapChainDesc.Flags = 0;

		// We need the IDXGIDevice1 to get the IDXGIAdapter.
		ComPtr<IDXGIDevice1> dxgiDevice;
		DX::ThrowIfFailed(
			m_device.As(&dxgiDevice), __FILEW__, __LINE__
			);

		// We need the IDXGIAdapter to get the IDXGIFactory2 which created it.
		ComPtr<IDXGIAdapter> dxgiAdapter;
		DX::ThrowIfFailed(
			dxgiDevice->GetAdapter(&dxgiAdapter), __FILEW__, __LINE__
			);

		// We need the IDXGIFactory2 to create the swap chain.
		ComPtr<IDXGIFactory2> dxgiFactory;
		DX::ThrowIfFailed(
			dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)), __FILEW__, __LINE__
			);

		// Create the swap chain. Note that for interaction with XAML we need to use CreateSwapChainForComposition.
		DX::ThrowIfFailed(
			dxgiFactory->CreateSwapChainForComposition(
			m_device.Get(),
			&swapChainDesc,
			nullptr,
			&m_swapChain
			), __FILEW__, __LINE__
			);

		// Associate the new swap chain with the SwapChainBackgroundPanel element. The SwapChainBackgroundPanel has
		// a COM interface, ISwapChainBackgroundPanelNative, which we need to use to make this association. That is
		// why we make the QI call here (and since SwapChainBackgroundPanel isn't a ComPtr, we can't use ComPtr::As
		// like we normally would to do a QI; instead we need to do it manually).
		ComPtr<ISwapChainBackgroundPanelNative> panelNative;
		DX::ThrowIfFailed(
			reinterpret_cast<IUnknown*>(m_panel)->QueryInterface(IID_PPV_ARGS(&panelNative)), __FILEW__, __LINE__
			);
		DX::ThrowIfFailed(
			panelNative->SetSwapChain(m_swapChain.Get()), __FILEW__, __LINE__
			);

		// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
		// ensures that the application will only render after each VSync, minimizing power consumption.
		DX::ThrowIfFailed(
			dxgiDevice->SetMaximumFrameLatency(1), __FILEW__, __LINE__
			);
	}

	// Set the proper orientation for the swap chain, and generate the
	// 3D matrix transformation for rendering to the rotated swap chain.
	// The 3D matrix is specified explicitly to avoid rounding errors.
	DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;
	switch (m_orientation)
	{
	case DisplayOrientations::Landscape:
		rotation = DXGI_MODE_ROTATION_IDENTITY;
		m_orientationTransform = XMFLOAT4X4( // 0-degree Z-rotation
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
			);
		break;

	case DisplayOrientations::Portrait:
		rotation = DXGI_MODE_ROTATION_ROTATE270;
		m_orientationTransform = XMFLOAT4X4( // 90-degree Z-rotation
			0.0f, 1.0f, 0.0f, 0.0f,
			-1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
			);
		break;

	case DisplayOrientations::LandscapeFlipped:
		rotation = DXGI_MODE_ROTATION_ROTATE180;
		m_orientationTransform = XMFLOAT4X4( // 180-degree Z-rotation
			-1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, -1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
			);
		break;

	case DisplayOrientations::PortraitFlipped:
		rotation = DXGI_MODE_ROTATION_ROTATE90;
		m_orientationTransform = XMFLOAT4X4( // 270-degree Z-rotation
			0.0f, -1.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
			);
		break;

	default:
		// Some unknown display orientation. We currently handle everything except None, which
		// isn't a real orientation, so this could only occur if new orientations were added
		// in a future update to Windows. If so, this switch block should be modified to take
		// into account any new orientations.
		throw ref new Platform::FailureException("Unknown Display Orientation.");
	}
	// Set the rotation on the swap chain.
	DX::ThrowIfFailed(
		m_swapChain->SetRotation(rotation), __FILEW__, __LINE__
		);

	// Create a Direct3D render target view of the swap chain back buffer.
	ComPtr<ID3D11Texture2D> backBuffer;
	DX::ThrowIfFailed(
		m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)), __FILEW__, __LINE__
		);

	// Create an SRV of the back buffer.
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	DX::ThrowIfFailed(
		m_device->CreateShaderResourceView(
		backBuffer.Get(),
		&srvDesc,
		&m_renderTargetSRV
		), __FILEW__, __LINE__
		);

	// Create an RTV of the back buffer.
	DX::ThrowIfFailed(
		m_device->CreateRenderTargetView(
		backBuffer.Get(),
		nullptr,
		&m_renderTargetView
		), __FILEW__, __LINE__
		);

	// Create a depth stencil buffer.
	CD3D11_TEXTURE2D_DESC depthStencilDesc(
		DXGI_FORMAT_D24_UNORM_S8_UINT, 
		static_cast<UINT>(m_renderTargetSize.Width),
		static_cast<UINT>(m_renderTargetSize.Height),
		1,
		1,
		D3D11_BIND_DEPTH_STENCIL
		);
	ComPtr<ID3D11Texture2D> depthStencil;
	DX::ThrowIfFailed(
		m_device->CreateTexture2D(
		&depthStencilDesc,
		nullptr,
		&depthStencil
		), __FILEW__, __LINE__
		);
	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	DX::ThrowIfFailed(
		m_device->CreateDepthStencilView(
		depthStencil.Get(),
		&depthStencilViewDesc,
		&m_depthStencilView
		), __FILEW__, __LINE__
		);

	// Set the 3D rendering viewport to target the entire window.
	CD3D11_VIEWPORT viewport(
		0.0f,
		0.0f,
		m_renderTargetSize.Width,
		m_renderTargetSize.Height
		);
	m_context->RSSetViewports(1, &viewport);
}

// This method is called in the event handler for the LogicalDpiChanged event.
void DirectXBase::SetDpi(float dpi)
{
	if (dpi != m_dpi)
	{
		// Save the updated DPI value.
		m_dpi = dpi;

		// Often a DPI change implies a window size change. In some cases Windows will issue
		// both a size changed event and a DPI changed event. In this case, the resulting bounds 
		// will not change, and the window resize code will only be executed once.
		UpdateForWindowSizeChange();
	}
}

// This method is called in the event handler for the SizeChanged event.
void DirectXBase::UpdateForWindowSizeChange()
{
	// Only handle window size changed if there is no pending DPI change. This prevents us from
	// running the rest of this function multiple times for a single dpi + resolution change event.
	if (m_dpi != DisplayProperties::LogicalDpi)
	{
		return;
	}

	// If the width, height, or orientation has changed, recreate all the window size dependent resources.
	if (m_window->Bounds.Width != m_windowBounds.Width ||
		m_window->Bounds.Height != m_windowBounds.Height ||
		m_orientation != DisplayProperties::CurrentOrientation)
	{
		CreateWindowSizeDependentResources();
	}
}

void DirectXBase::SetBackBuffer()
{
	// If we use a fixed back buffer then we need to ensure that we set that as the "back buffer". In reality the
	// fixed back buffer is just a render target that will be properly drawn to the actual back buffer (complete
	// with scaling and letterboxing) in DirectXBase::Present).
	if (m_usesFixedBackBuffer)
	{
		// Set the proper viewport.
		CD3D11_VIEWPORT viewport(
			0.0f,
			0.0f,
			m_fixedBackBufferDimensions.Width,
			m_fixedBackBufferDimensions.Height
			);
		m_context->RSSetViewports(1, &viewport);

		// If we're using multisampling, make sure we set that set of RTV & DSV as the back buffer rather than the
		// non-MSAA fixed back buffer.
		if (m_usesMultisampledFixedBackBuffer)
		{
			m_context->OMSetRenderTargets(1, m_fixedBackBufferMultisampled.GetRTV(), m_fixedBackBufferMultisampled.GetDSV());
			m_currentRenderTargetView = *m_fixedBackBufferMultisampled.GetRTV();
			m_currentDepthStencilView = m_fixedBackBufferMultisampled.GetDSV();
		}
		else
		{
			// We aren't using multisampling so just set the regular fixed back buffer RTV and DSV.
			m_context->OMSetRenderTargets(1, m_fixedBackBuffer.GetRTV(), m_fixedBackBuffer.GetDSV());
			m_currentRenderTargetView = *m_fixedBackBuffer.GetRTV();
			m_currentDepthStencilView = m_fixedBackBuffer.GetDSV();
		}
	}
	else
	{
		// We aren't using a fixed back buffer so just use the real back buffer directly.
		CD3D11_VIEWPORT viewport(
			0.0f,
			0.0f,
			m_renderTargetSize.Width,
			m_renderTargetSize.Height
			);

		m_context->RSSetViewports(1, &viewport);
		m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
		m_currentRenderTargetView = m_renderTargetView.Get();
		m_currentDepthStencilView = m_depthStencilView.Get();
	}
}

// Method to deliver the final image to the display.
void DirectXBase::Present()
{
	if (!m_deviceResourcesLoaded || !m_windowSizeResourcesLoaded || m_renderTargetView == nullptr || m_depthStencilView == nullptr)
	{
		return;
	}

	// Handle thing correctly if we're using a fixed back buffer.
	if (m_usesFixedBackBuffer)
	{
		// Check to see if we're using multisampling.
		if (m_usesMultisampledFixedBackBuffer)
		{
			// Resolve the multisampled fixed back buffer to the non-multisampled fixed back buffer (which we can then draw to the real back buffer).
			m_context->ResolveSubresource(m_fixedBackBuffer.GetTexture2D(), 0, m_fixedBackBufferMultisampled.GetTexture2D(), 0, m_fixedBackBuffer.GetDesc()->Format);
		}

		// Set the real back buffer and then draw the fixed back buffer scaled up or down appropriately.
		CD3D11_VIEWPORT viewport(
			0.0f,
			0.0f,
			m_renderTargetSize.Width,
			m_renderTargetSize.Height
			);
		m_context->RSSetViewports(1, &viewport);
		m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

		// Clear the back buffer to black (for letterboxing, if any) and clear the depth stencil view's depth.
		m_context->ClearRenderTargetView(m_renderTargetView.Get(), DirectX::Colors::Black);
		m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		// Begin SpriteBatch, using an anisotropic sampler for highest quality.
		m_spriteBatch->Begin(DirectX::SpriteSortMode_Deferred, nullptr, m_commonStates->AnisotropicClamp());//, nullptr, nullptr, nullptr, XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_orientationTransform)));

		// Calculate the aspect ratios of the fixed back buffer and the actual back buffer.
		auto fixedAspectRatio = m_fixedBackBufferDimensions.Width / m_fixedBackBufferDimensions.Height;
		auto destAspectRatio = m_windowBounds.Width / m_windowBounds.Height;//m_renderTargetSize.Width / m_renderTargetSize.Height;

		// Calculate the destination rectangle that we will be drawing the fixed back buffer to using the
		// aspect ratios to position things properly.
		RECT destRect = {};
		auto aspectRatioDifference = destAspectRatio - fixedAspectRatio;
		float dpiAdjust = m_dpi / 96.0f;
		if (fabs(aspectRatioDifference) < 0.001f) // 0.001f is used to avoid any slight discrepancies resulting from the nature of floating point math.
		{
			// Same aspect ratio
			destRect.left = 0;
			destRect.top = 0;
			destRect.right = static_cast<LONG>(m_windowBounds.Width * dpiAdjust);
			destRect.bottom = static_cast<LONG>(m_windowBounds.Height * dpiAdjust);
		}
		else
		{
			if (aspectRatioDifference < 0)
			{
				// fixed is wider than dest so letter box horizontally along the top and bottom
				destRect.left = 0;
				destRect.right = static_cast<LONG>(m_windowBounds.Width * dpiAdjust);
				auto height = static_cast<LONG>((m_windowBounds.Width * dpiAdjust) / fixedAspectRatio + 0.5f);
				auto difference = static_cast<LONG>(m_windowBounds.Height * dpiAdjust) - height;
				destRect.top = difference / 2L;
				destRect.bottom = destRect.top + height;
			}
			else
			{
				// dest is wider than fixed so letter box vertically along the left and right
				destRect.top = 0;
				destRect.bottom = static_cast<LONG>(m_windowBounds.Height * dpiAdjust);
				auto width = static_cast<LONG>((m_windowBounds.Height * dpiAdjust) * fixedAspectRatio + 0.5f);
				auto difference = static_cast<LONG>(m_windowBounds.Width * dpiAdjust) - width;
				destRect.left = difference / 2L;
				destRect.right = destRect.left + width;
			}
		}

		// Draw the fixed back buffer to the actual back buffer.

		// Calculate the adjustments to the detRect so that it will be properly scaled and rotated when drawn with SpriteBatch.
		float rotation = 0.0f;
		switch (m_orientation)
		{
		case Windows::Graphics::Display::DisplayOrientations::None:
			break;
		case Windows::Graphics::Display::DisplayOrientations::Landscape:
			// Do nothing.
			break;
		case Windows::Graphics::Display::DisplayOrientations::Portrait:
			{
				rotation = XM_PIDIV2 + XM_PI;
				auto halfLRDiff = (destRect.right - destRect.left) * 0.5f;
				auto halfTBDiff = (destRect.bottom - destRect.top) * 0.5f;
				destRect.left += static_cast<LONG>(m_renderTargetSize.Width * 0.5f - halfTBDiff);
				destRect.right += static_cast<LONG>(m_renderTargetSize.Width * 0.5f - halfTBDiff);
				destRect.top += static_cast<LONG>(m_renderTargetSize.Width * 0.5f - halfLRDiff);
				destRect.bottom += static_cast<LONG>(m_renderTargetSize.Width * 0.5f - halfLRDiff);
			}
			break;
		case Windows::Graphics::Display::DisplayOrientations::LandscapeFlipped:
			rotation = XM_PI;
			destRect.left += static_cast<LONG>(m_renderTargetSize.Width);
			destRect.right += static_cast<LONG>(m_renderTargetSize.Width);
			destRect.top += static_cast<LONG>(m_renderTargetSize.Height);
			destRect.bottom += static_cast<LONG>(m_renderTargetSize.Height);
			break;
		case Windows::Graphics::Display::DisplayOrientations::PortraitFlipped:
			{
				rotation = XM_PIDIV2;
				auto halfTBDiff = (destRect.bottom - destRect.top) * 0.5f;
				destRect.left += static_cast<LONG>(m_renderTargetSize.Width * 0.5f + halfTBDiff);
				destRect.right += static_cast<LONG>(m_renderTargetSize.Width * 0.5f + halfTBDiff);
				destRect.top -= static_cast<LONG>(m_renderTargetSize.Width * 0.5f - halfTBDiff);
				destRect.bottom -= static_cast<LONG>(m_renderTargetSize.Width * 0.5f - halfTBDiff);
			}
		default:
			break;
		}

		m_spriteBatch->Draw(m_fixedBackBuffer.GetSRV(), destRect, nullptr, DirectX::Colors::White, rotation);//, orientation);
		m_spriteBatch->End();

		// Unbind the fixed back buffer from the pixel shader to avoid having D3D warning messages
		// fire out at us when we try to bind the fixed back buffer as a render target again at
		// the beginning of the next render pass.
		static ID3D11ShaderResourceView* const nullSRV[] = { nullptr };
		m_context->PSSetShaderResources(0, 1, nullSRV);
	}

	// The application may optionally specify "dirty" or "scroll"
	// rects to improve efficiency in certain scenarios.
	DXGI_PRESENT_PARAMETERS parameters = {0};
	parameters.DirtyRectsCount = 0;
	parameters.pDirtyRects = nullptr;
	parameters.pScrollRect = nullptr;
	parameters.pScrollOffset = nullptr;

	// The first argument instructs DXGI to block until VSync, putting the application
	// to sleep until the next VSync. This ensures we don't waste any cycles rendering
	// frames that will never be displayed to the screen.
	HRESULT hr = m_swapChain->Present1(1, 0, &parameters);

	// Discard the contents of the render target.
	// This is a valid operation only when the existing contents will be entirely
	// overwritten. If dirty or scroll rects are used, this call should be removed.
	m_context->DiscardView(m_renderTargetView.Get());

	// Discard the contents of the depth stencil.
	m_context->DiscardView(m_depthStencilView.Get());

	// If the device was removed by a disconnect, a driver upgrade, or a problem in another
	// application, we must recreate all device resources.
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		HandleDeviceLost();
	}
	else
	{
		// Else if everything presented fine, proceed, otherwise throw an exception
		// since an error we aren't setup to handle occurred.
		DX::ThrowIfFailed(hr, __FILEW__, __LINE__);
	}
}

// Method to convert a length in device-independent pixels (DIPs) to a length in physical pixels.
float DirectXBase::ConvertDipsToPixels(float dips)
{
	static const float dipsPerInch = 96.0f;
	return floor(dips * m_dpi / dipsPerInch + 0.5f); // Round to nearest integer.
}

void DirectXBase::ValidateDevice()
{
	// Get the description for the graphics adapter by getting the adapter from the ID3D11Device1
	// which is also an IDXGIDevice1 device. This is the adapter that is affiliated with the
	// existing graphics device.
	ComPtr<IDXGIDevice1> dxgiDevice;
	ComPtr<IDXGIAdapter> deviceAdapter;
	DXGI_ADAPTER_DESC deviceDesc;
	DX::ThrowIfFailed(m_device.As(&dxgiDevice), __FILEW__, __LINE__);
	DX::ThrowIfFailed(dxgiDevice->GetAdapter(&deviceAdapter), __FILEW__, __LINE__);
	DX::ThrowIfFailed(deviceAdapter->GetDesc(&deviceDesc), __FILEW__, __LINE__);

	// Get the description for the graphics adapter that is the current system default
	// graphics adapter.
	ComPtr<IDXGIFactory2> dxgiFactory;
	ComPtr<IDXGIAdapter1> currentAdapter;
	DXGI_ADAPTER_DESC currentDesc;
	DX::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)), __FILEW__, __LINE__);
	DX::ThrowIfFailed(dxgiFactory->EnumAdapters1(0, &currentAdapter), __FILEW__, __LINE__);
	DX::ThrowIfFailed(currentAdapter->GetDesc(&currentDesc), __FILEW__, __LINE__);

	// Compare the manufacturer ids for each to see if they match and also see if the
	// existing device itself has a device removed reason (if it doesn't then it'll
	// return S_OK such that FAILED(...) will return false).
	if (deviceDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart ||
		deviceDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart ||
		FAILED(m_device->GetDeviceRemovedReason()))
	{
		// If there was a mismatch or a device removal reason, we need to recreate the
		// graphics device and all of its resources (which HandleDeviceLost will do).
		// We set dxgiDevice and deviceAdapter to null first so that no we remove the
		// extra reference to each (since COM objects are reference counted).
		dxgiDevice = nullptr;
		deviceAdapter = nullptr;
		HandleDeviceLost();
	}
}

RECT DirectXBase::GetFixedBackBufferBoundingRect()
{
	// Figure out the dpi adjustment.
	float dpiAdjust = m_dpi / 96.0f;
	// Calculate the width of the window in pixels.
	auto windowWidth = m_windowBounds.Width * dpiAdjust;
	// Calculate the height of the window in pixels.
	auto windowHeight = m_windowBounds.Height * dpiAdjust;

	// Figure out the aspect ratio of the fixed back buffer.
	auto fixedAspectRatio = m_fixedBackBufferDimensions.Width / m_fixedBackBufferDimensions.Height;

	// Figure out the aspect ratio of the window.
	auto destAspectRatio = windowWidth / windowHeight;

	// Figure out the size and position of the fixed back buffer.
	RECT destRect = {};

	// Use the aspect ratio difference to figure out whether we're working with the same ratio, whether we need horizontal letterboxing, or whether
	// we need vertical letterboxing.
	auto aspectRatioDifference = destAspectRatio - fixedAspectRatio;
	if (fabs(aspectRatioDifference) < 0.001f) // 0.001f is used to avoid any slight discrepancies resulting from the nature of floating point math.
	{
		// Same aspect ratio so the destination is the window bounds itself (including dpi adjustment).
		destRect.left = 0;
		destRect.top = 0;
		destRect.right = static_cast<LONG>(windowWidth);
		destRect.bottom = static_cast<LONG>(windowHeight);
	}
	else
	{
		if (aspectRatioDifference < 0)
		{
			// The fixed back buffer is wider than the window so there's letterboxing horizontally along the top and bottom. 
			destRect.left = 0;
			destRect.right = static_cast<LONG>(windowWidth);
			// Calculate the height and position using the aspect ratio.
			auto height = static_cast<LONG>(windowWidth / fixedAspectRatio + 0.5f);
			auto difference = static_cast<LONG>(windowHeight) - height;
			destRect.top = difference / 2L;
			destRect.bottom = destRect.top + height;
		}
		else
		{
			// The window is wider than the fixed back buffer so there's letterboxing vertically along the left and right.
			destRect.top = 0;
			destRect.bottom = static_cast<LONG>(windowHeight);
			// Calculate the width and position using the aspect ratio.
			auto width = static_cast<LONG>(windowHeight * fixedAspectRatio + 0.5f);
			auto difference = static_cast<LONG>(windowWidth) - width;
			destRect.left = difference / 2L;
			destRect.right = destRect.left + width;
		}
	}

	return destRect;
}

Windows::Foundation::Point DirectXBase::PointerPositionToFixedPosition(Windows::Foundation::Point pointerPosition)
{
	// If we aren't using a fixed back buffer, just return the passed in value.
	if (!m_usesFixedBackBuffer)
	{
		return pointerPosition;
	}

	// Note: Input is translated automagically for orientation by the runtime so it doesn't need to be modified to switch X/Y or anything.

	// Figure out the dpi adjustment.
	float dpiAdjust = m_dpi / 96.0f;

	// Get the bounding rect for the rendered fixed buffer.
	auto destRect = GetFixedBackBufferBoundingRect();

	// Calculate and return the adjusted pointer position.
	return Windows::Foundation::Point(
		static_cast<float>(static_cast<int>((pointerPosition.X * dpiAdjust - destRect.left) * (m_fixedBackBufferDimensions.Width / static_cast<float>(destRect.right - destRect.left)))),
		static_cast<float>(static_cast<int>((pointerPosition.Y * dpiAdjust - destRect.top) * (m_fixedBackBufferDimensions.Height / static_cast<float>(destRect.bottom - destRect.top))))
		);
}

bool DirectXBase::GetGamePaused()
{
	return m_gamePaused;
}

void DirectXBase::SetGamePaused(bool isPaused)
{
	m_gamePaused = isPaused;
	// Show or hide the XAML element named PausedOverlay based on whether or not the game is paused. We cast to a UIElement since that allows you to change 
	// what the element names PausedOverlay is without needing to come back and modify this code. There must be an element named PausedOverlay or this code
	// will throw an exception.
	safe_cast<Windows::UI::Xaml::UIElement^>(m_panel->FindName("PausedOverlay"))->Visibility = (isPaused ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed);
}

bool DirectXBase::GetWindowIsDeactivated()
{
	return m_windowIsDeactivated;
}

void DirectXBase::SetWindowIsDeactivated(bool isDeactivated)
{
	if (isDeactivated)
	{
		m_windowIsDeactivatedRefCounter++;
		m_windowIsDeactivated = true;
		if (m_windowIsDeactivatedRefCounter == 1)
		{
			OnWindowActivationChanged();
		}
		//#if defined(_DEBUG)
		//OutputDebugStringW(std::wstring(L"Deactivated ref count incremented. Value: ").append(std::to_wstring(m_windowIsDeactivatedRefCounter)).append(L"\n").c_str());
		//#endif
	}
	else
	{
		if (m_windowIsDeactivatedRefCounter > 0)
		{
			--m_windowIsDeactivatedRefCounter;
			//#if defined(_DEBUG)
			//OutputDebugStringW(std::wstring(L"Deactivated ref count decremented. Value: ").append(std::to_wstring(m_windowIsDeactivatedRefCounter)).append(L"\n").c_str());
			//#endif
			if (m_windowIsDeactivatedRefCounter == 0)
			{
				m_windowIsDeactivated = false;
				OnWindowActivationChanged();
			}
		}
		else
		{
			// Just to be sure.
			if (m_windowIsDeactivated)
			{
				// If this code executes, then there is a bug in the reference counting code. This should never happen.
				m_windowIsDeactivated = false;
				OnWindowActivationChanged();
#if defined(_DEBUG)
				OutputDebugStringW(std::wstring(L"Deactivated ref count already zero! Value: ").append(std::to_wstring(m_windowIsDeactivatedRefCounter)).append(L"\n").c_str());
				if (IsDebuggerPresent())
				{
					__debugbreak();
				}
#endif
			}
		}
	}
}

void DirectXBase::SetFixedBackBufferParameters(
	UINT width,
	UINT height,
	DXGI_FORMAT format,
	DXGI_FORMAT depthStencilFormat,
	bool useMultisampling,
	uint32 preferredMultisamplingCount,
	uint32 preferredMultisamplingQuality
	)
{
	// If width or height are zero, we shut off the fixed back buffer.
	if (width == 0 || height == 0)
	{
		m_usesFixedBackBuffer = false;
		m_usesMultisampledFixedBackBuffer = false;
		return;
	}

	// Store the parameters so that we can use them to create the fixed back buffer when we call CreateFixedBackBuffer.
	// We don't do any parameter validation here since that all happens in CreateFixedBackBuffer.
	m_usesFixedBackBuffer = true;
	m_fixedBackBufferDepthStencilFormat = depthStencilFormat;
	m_fixedBackBufferDimensions = Windows::Foundation::Size(static_cast<float>(width), static_cast<float>(height));
	m_fixedBackBufferFormat = format;
	m_fixedBackBufferMultisamplePreferredCount = preferredMultisamplingCount;
	m_fixedBackBufferMultisamplePreferredQuality = preferredMultisamplingQuality;
	m_usesFixedBackBuffer = useMultisampling;
}

void DirectXBase::CreateFixedBackBuffer()
{
	// If no fixed back buffer has been requested, then reset some member variables and return.
	if (!m_usesFixedBackBuffer)
	{
		m_fixedBackBuffer.Reset();
		m_fixedBackBufferMultisampled.Reset();
		m_usesMultisampledFixedBackBuffer = false;
		return;
	}

	// Create the fixed back buffer render target. If we are using multisampling, this will act as the resolve target.
	m_fixedBackBuffer.CreateRenderTarget(
		m_device.Get(),
		static_cast<UINT>(m_fixedBackBufferDimensions.Width),
		static_cast<UINT>(m_fixedBackBufferDimensions.Height),
		m_fixedBackBufferFormat,
		true,
		m_fixedBackBufferDepthStencilFormat,
		1,
		0
		);

	if (m_usesMultisampledFixedBackBuffer)
	{
		// If we're using multisampling, create the multisample fixed render target.
		m_fixedBackBufferMultisampled.CreateRenderTarget(
			m_device.Get(),
			static_cast<UINT>(m_fixedBackBufferDimensions.Width),
			static_cast<UINT>(m_fixedBackBufferDimensions.Height),
			m_fixedBackBufferFormat,
			true,
			m_fixedBackBufferDepthStencilFormat,
			m_fixedBackBufferMultisamplePreferredCount,
			m_fixedBackBufferMultisamplePreferredQuality
			);

		// Check to see if we actually wound up with multisampling.
		m_usesMultisampledFixedBackBuffer = m_fixedBackBufferMultisampled.GetDesc()->SampleDesc.Count > 1;

		// If not mark that we aren't so we don't try to use it by accident.
		if (m_usesMultisampledFixedBackBuffer == false)
		{
			// Creating the multisampled back buffer failed so we're releasing the resources.
			m_fixedBackBufferMultisampled.Reset();
		}
	}
}
