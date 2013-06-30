//
// DirectXPage.xaml.cpp
// Implementation of the DirectXPage.xaml class.
//

#include "pch.h"
#include "DirectXPage.xaml.h"
#include "Game.h"
#include "BasicTimer.h"

using namespace WindowsStoreDirectXGame;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Input;
using namespace Windows::UI::Core;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// Used to contain static variables for frame rate tracking.
namespace FPSCounter
{
	// How many frames have been displayed. Resets every time LastUpdateTime is updated (appox. every 1 second).
	static int FrameCount = 0;
	// The last time the frame rate was calculated.
	static float LastUpdateTime = 0.0f;
	// Set to true to display the FPS counter; false to not display it.
	const static bool DisplayFPSCounter = true;
}

DirectXPage::DirectXPage() :
	m_renderingEventToken(),
	m_game(),
	m_lastPoint(),
	m_lastPointValid(),
	m_timer()
{
	// InitializeComponent prepares all the XAML objects and runs the generated code. This call should generally always come first
	// in the constructor of a XAML page.
	InitializeComponent();

	// If we are using the FPS counter, make it visible.
	if (FPSCounter::DisplayFPSCounter)
	{
		FPSCounterTextBlock->Visibility = Windows::UI::Xaml::Visibility::Visible;
	}

	// Get the game's window.
	auto window = Window::Current->CoreWindow;

	// Create a new instance of the Game class. Game is a reference counted class so you do not need to worry about calling delete on
	// it or anything. The same goes for anything else that is 'ref new ...'.
	m_game = ref new Game();

	// Initialize the game with the window, the XAML page's SwapChainBackgroundPanel control, and the DPI of the display. DPI is important
	// because unlike traditional Windows versions where you could mostly assume a DPI of 96, modern screens (especially tablets) frequently
	// have higher DPIs so we must accomodate this.
	m_game->Initialize(
		window,
		SwapChainPanel,
		DisplayProperties::LogicalDpi
		);

	// Register for the window's SizeChanged event so that we know when the window becomes snapped, filled, moves to a different screen (with a
	// different size), or stays on the same screen but the resolution is changed.
	window->SizeChanged += 
		ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &DirectXPage::OnWindowSizeChanged);

	// Register to know when the DPI changes. You can most easily test this sort of change using the Simulator.
	DisplayProperties::LogicalDpiChanged +=
		ref new DisplayPropertiesEventHandler(this, &DirectXPage::OnLogicalDpiChanged);

	// Register to know when the screen orientation has changed.
	DisplayProperties::OrientationChanged +=
		ref new DisplayPropertiesEventHandler(this, &DirectXPage::OnOrientationChanged);

	// Register to know when something happens that may require us to recreate the device resources.
	DisplayProperties::DisplayContentsInvalidated +=
		ref new DisplayPropertiesEventHandler(this, &DirectXPage::OnDisplayContentsInvalidated);

	// Create a new timer to track elapsed time between OnRendering calls and total game runtime.
	m_timer = ref new BasicTimer();

	// Subscribe to the Rendering event so that our game loop runs and our game shows up.
	m_renderingEventToken = CompositionTarget::Rendering::add(ref new EventHandler<Object^>(this, &DirectXPage::OnRendering));
}

void DirectXPage::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	// Use the VisualStateManager to go to the correct view state. The view states are defined in the XAML itself and were
	// created using Blend for Visual Studio. If you are viewing a XAML page in Visual Studio, you can open it in Blend from
	// the View menu.
	switch (ApplicationView::Value)
	{
	case Windows::UI::ViewManagement::ApplicationViewState::FullScreenLandscape:
		VisualStateManager::GoToState(this, "FullScreenLandscape", true);
		break;
	case Windows::UI::ViewManagement::ApplicationViewState::Filled:
		VisualStateManager::GoToState(this, "Filled", true);
		break;
	case Windows::UI::ViewManagement::ApplicationViewState::Snapped:
		VisualStateManager::GoToState(this, "Snapped", true);
		break;
	case Windows::UI::ViewManagement::ApplicationViewState::FullScreenPortrait:
		VisualStateManager::GoToState(this, "FullScreenPortrait", true);
		break;
	default:
		break;
	}

	// Have the game update itself for the window size change.
	m_game->UpdateForWindowSizeChange();
}

void DirectXPage::OnLogicalDpiChanged(Object^ sender)
{
	// Have the game update itself for the new DPI.
	m_game->SetDpi(DisplayProperties::LogicalDpi);
}

void DirectXPage::OnOrientationChanged(Object^ sender)
{
	// An orientation change implies a window size change since the dimensions will swap at a minimum, so update for that.
	m_game->UpdateForWindowSizeChange();
}

void DirectXPage::OnDisplayContentsInvalidated(Object^ sender)
{
	// Have the game validate the device and recreate resources if necessary.
	m_game->ValidateDevice();
}

void DirectXPage::OnRendering(Object^ sender, Object^ args)
{
	// This is the main game loop.
	
	// First we update the timer so we know how long it's been since the last call to OnRendering and how long the game
	// has been running in total.
	m_timer->Update();

	// Then we update the FPS counter, but only if we're using it.
	if (FPSCounter::DisplayFPSCounter)
	{
		// Update the FPS counter.
		++FPSCounter::FrameCount;

		// If at least 1 second has passed since the last time we updated the frame rate counter, update it again.
		if (m_timer->Total >= FPSCounter::LastUpdateTime + 1.0f)
		{
			// Use a wstringstream to easily control formatting of the FPS string.
			std::wstringstream fpsTimeString;
			fpsTimeString << L"FPS: " << std::setprecision(1) << std::fixed << FPSCounter::FrameCount / (m_timer->Total - FPSCounter::LastUpdateTime);
			// Make the FPS string into a Platform::String
			auto fpsPlatformString = ref new Platform::String(fpsTimeString.str().c_str());

			// We cannot update the UI except on the UI thread. So check to see if we're there.
			if (Dispatcher->HasThreadAccess)
			{
				// If so, update the FPSCounterTextBlock text directly.
				FPSCounterTextBlock->Text = fpsPlatformString;
			}
			else
			{
				// Otherwise use the CoreDispatcher to update the FPSCounterTextBlock text on the UI thread.
				Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, fpsPlatformString]()
				{
					FPSCounterTextBlock->Text = fpsPlatformString;
				}));
			}

			// Since we have now updated the FPS counter, set the LastUpdateTime to the current total time and reset FrameCount to 0.
			FPSCounter::LastUpdateTime = m_timer->Total;
			FPSCounter::FrameCount = 0;
		}
	}

	// Next we call the game's Update member function to allow for processing changes to game state based on new input and
	// the passage of time.
	m_game->Update(m_timer->Total, m_timer->Delta);

	// Then we call the game's Render member function to draw all of the DirectX content.
	m_game->Render(m_timer->Total, m_timer->Delta);

	// Lastly we call the game's Present member function, which causes the content to be drawn to the screen and handles
	// any errors that might be detected from doing that. It also handles all the fixed back buffer conversion if you are
	// using that functionality as well as properly handling MSAA if that is being used.
	m_game->Present();
}

void DirectXPage::SaveInternalState()
{
	// In this sample we don't have any page-specific state to save so we just proceed to calling the game's save state
	// member function. If we did want to save state information about this page, this is where we would do it.
	m_game->SaveInternalState();
}

void DirectXPage::LoadInternalState()
{
	// In this sample we don't save any page-specific state such that there's no page-specific state to load. As such
	// we just proceed to loading the game's saved state.
	m_game->LoadInternalState();
}
