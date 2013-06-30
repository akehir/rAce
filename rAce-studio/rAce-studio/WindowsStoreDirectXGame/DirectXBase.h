#pragma once

#include "DirectXHelper.h"
#include "RenderTarget2D.h"
#include "SpriteBatch.h"
#include "CommonStates.h"

// Helper class that initializes DirectX APIs for both 2D and 3D rendering.
// Some of the code in this class may be omitted if only 2D or only 3D rendering is being used.
ref class DirectXBase abstract
{
internal:
	// Constructor.
	DirectXBase();

public:
	// Caches the CoreWindow and SwapChainBackgroundPanel then proceeds to call CreateDeviceIndependentResources, CreateDeviceResources, and
	// SetDpi.
	virtual void Initialize(Windows::UI::Core::CoreWindow^ window, Windows::UI::Xaml::Controls::SwapChainBackgroundPanel^ panel, float dpi);

	// Recreate all device resources and window-size dependent resources and set them back to the current state.
	virtual void HandleDeviceLost();

	// Creates any resources that aren't dependent on the graphics device.
	virtual void CreateDeviceIndependentResources();

	// Creates the ID3D11Device, the ID3D11DeviceContext, and the SpriteBatch and CommonStates instances.
	virtual void CreateDeviceResources();

	// Creates or resizes the swap chain and creates its render target view, the depth stencil buffer, and sets the device viewport.
	virtual void CreateWindowSizeDependentResources();

	// Updates the dpi and, if necessary, (re)creates the window size-dependent resources.
	virtual void SetDpi(float dpi);

	// Updates for a window size change and, if necessary, (re)creates the window size-dependent resources. Avoids recreating the
	// resources if there is a pending DPI change as well, deferring that to the call to SetDpi that will happen.
	virtual void UpdateForWindowSizeChange();

	// Sets the appropriate back buffer on the device. You should call this when you are ready to draw to the back buffer. This takes
	// the fixed back buffer and multisampling (if any) into account so you should use this rather than trying to set the back buffer
	// directly.
	void SetBackBuffer();

	// Executed when the window becomes activated or deactivated.
	virtual void OnWindowActivationChanged() = 0;

	// Draw the scene.
	virtual void Render(float timeTotal, float timeDelta) = 0;

	// Presents the rendered scene for display. Handles any fixed back buffer and multisampling that might be configured along with any scaling
	// and letterboxing that is required.
	void Present();

	// Method to convert a length in device-independent pixels (DIPs) to a length in physical pixels.
	virtual float ConvertDipsToPixels(float dips);

	// Checks to see if the current device matches the values we cached when we created the device and also checks to see if the device is valid.
	// If not, the device resources and window sized resources are recreated.
	void ValidateDevice();

internal:
	// Returns the scaled bounding rectangle representing the final size and position of the fixed back buffer as it will be rendered.
	RECT GetFixedBackBufferBoundingRect();

public:
	// If you are using a fixed back buffer, this converts pointer input into uniform coordinates based on the scale and position of the fixed
	// back buffer when it is presented to the screen such that pointer input to a specific part of the fixed back buffer always returns the same
	// output (e.g. a touch at (100, 100) always results in a return value of approximately (100, 100) regardless of the scale and position of the
	// back buffer.
	Windows::Foundation::Point PointerPositionToFixedPosition(Windows::Foundation::Point pointerPosition);

	// Returns true if the game is paused, false if not.
	bool GetGamePaused();

	// Sets whether the game is paused or not.
	void SetGamePaused(bool isPaused);

	// Returns true if the window is deactivated, false if not.
	bool GetWindowIsDeactivated();

	// WindowIsDeactivated is a reference counted bool property. For every call to SetWindowIsDeactivated with 'true', there must be a later call with 'false'
	// otherwise the game will think it is still deactivated. It is safe to call this with 'false' too many times, though that is likely an indication of a
	// logic error in your code. In debug builds, a debug message will be printed and the debugger (if present) will be instructed to break.
	void SetWindowIsDeactivated(bool isDeactivated);

	// Returns true if using a fixed back buffer, false if not.
	bool IsUsingFixedBackBuffer() { return m_usesFixedBackBuffer;}

	// Returns the fixed back buffer size. Only valid if a fixed back buffer is actually being used.
	Windows::Foundation::Size GetFixedBackBufferSize() { return m_fixedBackBufferDimensions; }

	// Returns the window size.
	Windows::Foundation::Size GetWindowSize() { return std::move(Windows::Foundation::Size(m_windowBounds.Width, m_windowBounds.Height)); }

internal:

	// Returns a pointer to the D3D device. Do not cache this value (except in a local function variable) since it will become invalid if the device needs to be recreated.
	ID3D11Device1* GetD3DDevice() { return m_device.Get(); }

	// Returns a pointer to the immediate context. Do not cache this value (except in a local function variable) since it will become invalid if the device needs to be recreated.
	ID3D11DeviceContext1* GetImmediateContext() { return m_context.Get(); }

	// Returns a pointer to the SpriteBatch instance used by DirectXBase, which can also be used by the game itself and by game components.
	DirectX::SpriteBatch* GetSpriteBatch() { return m_spriteBatch.get(); }

	// Returns a pointer to the CommonStates instance used by DirectXBase, which can also be used by the game itsef and by game components.
	DirectX::CommonStates* GetCommonStates() { return m_commonStates.get(); }

	// Returns a pointer to the SRV of the fixed back buffer, multisampled fixed back buffer, or the swap chain's back buffer, whichever is appropriate.
	ID3D11ShaderResourceView* GetCurrentRenderTargetSRV()
	{
		if (m_usesFixedBackBuffer)
		{
			if (m_usesMultisampledFixedBackBuffer)
			{
				return m_fixedBackBufferMultisampled.GetSRV();
			}
			else
			{
				return m_fixedBackBuffer.GetSRV();
			}
		}
		else
		{
			return m_renderTargetSRV.Get();
		}
	}

protected private:
	// This sets the parameters for a fixed back buffer which will be automatically scaled (anisotropic) and letterboxed as necessary so that you can target a particular
	// resolution (e.g. 1366x768) and be confident that objects will be positioned correctly on other resolutions. You can also use this to enable
	// multisampling. XAML + DirectX composition does not currently allow for multisampled back buffers so a multisampled render target must be used and
	// then resolved into a non-multisampled render target, which is then drawn to the back buffer using SpriteBatch.
	// width - The width of the fixed back buffer (e.g. 1366 or 1920). Set width or height to 0 to turn off the fixed back buffer.
	// height - The height of the fixed back buffer (e.g. 768 or 1080). Set width or height to 0 to turn off the fixed back buffer.
	// format - The DXGI_FORMAT of the fixed back buffer. For feature level 9.1, pick from: DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_B8G8R8X8_UNORM, DXGI_FORMAT_B8G8R8X8_UNORM_SRGB, or DXGI_FORMAT_B5G6R5_UNORM. Note that the last three do not support alpha blending.
	// depthStencilFormat - The DXGI_FORMAT of the depth stencil buffer.  For feature level 9.1, pick from: DXGI_FORMAT_D24_UNORM_S8_UINT or DXGI_FORMAT_D16_UNORM. Note that the later has no stencil support and has less depth precision.
	// useMultisampling - If true, the values for preferredMultisamplingCount and preferredMultisamplingQuality will be used (to the extent possible).
	// preferredMultisamplingCount - The preferred count for multisampling. 1 means no multisampling. Other typical values are 2, 4, 8, 16, and 32 (D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT). Check this value first using either DX::GetSupportedMultisampleSettings in Utility.h or ID3D11Device::CheckMultisampleQualityLevels (see: http://msdn.microsoft.com/en-us/library/ff476499(v=vs.85).aspx ) since if this value is unsupported by the card then multisampling will not be enabled.
	// preferredMultisamplingQuality - The preferred quality for multisampling. This is vendor dependent (e.g. AMD typically only supports a value of 0 while NVIDIA typically supports a range of values).
	void SetFixedBackBufferParameters(
		UINT width,
		UINT height,
		DXGI_FORMAT format,
		DXGI_FORMAT depthStencilFormat,
		bool useMultisampling,
		uint32 preferredMultisamplingCount,
		uint32 preferredMultisamplingQuality
		);

	// Create the fixed back buffer, if any. This function returns silently if no fixed back buffer has been setup or if the fixed back buffer has been turned off.
	void CreateFixedBackBuffer();

	// Stores the core window for reference.
	Platform::Agile<Windows::UI::Core::CoreWindow>			m_window;

	// The XAML control that the DirectX content is drawn to.
	Windows::UI::Xaml::Controls::SwapChainBackgroundPanel^	m_panel;

	// The graphics device.
	Microsoft::WRL::ComPtr<ID3D11Device1>					m_device;

	// The device context.
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1>			m_context;

	// The swap chain.
	Microsoft::WRL::ComPtr<IDXGISwapChain1>					m_swapChain;

	// The RTV of the swap chain.
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>			m_renderTargetView;

	// The SRV of the swap chain's back buffer.
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>		m_renderTargetSRV;

	// The DSV associated with the swap chain.
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>			m_depthStencilView;

	// A pointer to the current back buffer RTV when using a fixed back buffer.
	ID3D11RenderTargetView*									m_currentRenderTargetView;

	// A pointer to the current depth stencil buffer when using a fixed back buffer.
	ID3D11DepthStencilView*									m_currentDepthStencilView;

	// The feature level of the graphics device.
	D3D_FEATURE_LEVEL										m_featureLevel;

	// The size of the swap chain's render target.
	Windows::Foundation::Size								m_renderTargetSize;

	// The bounds of the current window. Not adjusted for DPI.
	Windows::Foundation::Rect								m_windowBounds;

	// The last stored DPI.
	float													m_dpi;

	// The last stored display orientation.
	Windows::Graphics::Display::DisplayOrientations			m_orientation;

	// Stores whether the window is deactivated. Deactivated could mean that the charms bar is open, that settings are being shown,
	// that another window has focus (snapped/filled), or that another application is currently the active application.
	bool													m_windowIsDeactivated;

	// A ref counter for tracking whether or not the window should be considered deactivated or not. Helpful in tracking transitions
	// from the charms bar to settings then back to the charms bar, etc.
	uint32													m_windowIsDeactivatedRefCounter;

	// Tracks whether the game is paused.
	bool													m_gamePaused;

	// Tracks whether we're using a fixed back buffer.
	bool													m_usesFixedBackBuffer;

	// The fixed back buffer (if we're using one). If we're using multisampling, this is the target that the multisample buffer is resolved to.
	RenderTarget2D											m_fixedBackBuffer;

	// The multisample fixed back buffer (if we're using one).
	RenderTarget2D											m_fixedBackBufferMultisampled;

	// The dimensions of the fixed back buffer.
	Windows::Foundation::Size								m_fixedBackBufferDimensions;

	// The format of the fixed back buffer.
	DXGI_FORMAT												m_fixedBackBufferFormat;

	// The format of the depth stencil buffer associated with the fixed back buffer.
	DXGI_FORMAT												m_fixedBackBufferDepthStencilFormat;

	// Tracks whether we're using multisampling.
	bool													m_usesMultisampledFixedBackBuffer;

	// The multisampling count. If 1 then multisampling is not being used.
	uint32													m_fixedBackBufferMultisamplePreferredCount;

	// The multisampling quality. GPU vendor dependent value. If not using multisampling, should be 0.
	uint32													m_fixedBackBufferMultisamplePreferredQuality;

	// A SpriteBatch used for drawing a fixed back buffer. Can also be used by the game itself.
	std::unique_ptr<DirectX::SpriteBatch>					m_spriteBatch;

	// A CommonStates used for drawing a fixed back buffer with anisotropic filter (for best quality). Can also be used by the game itself.
	std::unique_ptr<DirectX::CommonStates>					m_commonStates;

	// Transforms used for display orientation.
	DirectX::XMFLOAT4X4										m_orientationTransform;

	// Tracks whether or not device independent resources are finished loading.
	bool													m_deviceIndependentResourcesLoaded;

	// Tracks whether or not device resources are finished loading.
	bool													m_deviceResourcesLoaded;

	// Tracks whether or not window size resources are finished loading.
	bool													m_windowSizeResourcesLoaded;
};
