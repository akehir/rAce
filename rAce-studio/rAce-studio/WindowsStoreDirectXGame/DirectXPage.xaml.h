//
// BlankPage.xaml.h
// Declaration of the BlankPage.xaml class.
//

#pragma once

#include "DirectXPage.g.h"

// Forward declarations so we can use these types as pointer data members without needing to include their headers here. This
// speeds up compilation times.
ref class Game;
ref class BasicTimer;

namespace WindowsStoreDirectXGame
{
	/// <summary>
	/// A DirectX page that can be used on its own.  Note that it may not be used within a Frame.
	/// </summary>
    [Windows::Foundation::Metadata::WebHostHidden]
	public ref class DirectXPage sealed
	{
	public:
		// Constructor.
		DirectXPage();
		// Saves state for the page and call's the game's save state function.
		void SaveInternalState();
		// Loads state for the page and call's the game's load state function. 
		void LoadInternalState();

	internal:
		// Return the Game object that is the core of our game. Game inherits from DirectXBase.
		Game^ GetGame() { return m_game; }

	private:
		// Called when the window size changes. This could be due to a screen resolution change, a move to a different screen, or
		// a change between full screen, filled, and snapped view.
		void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
		// Called when the screen DPI changes.
		void OnLogicalDpiChanged(Platform::Object^ sender);
		// Called when the device orientation (landscape/portrait) changes. You can limit the orientations you want to support in 
		// Package.appxmanifest.
		void OnOrientationChanged(Platform::Object^ sender);
		// Called when the display content are invalid. The documentation on this is a bit light. The device may have changed
		// such as with switchable graphics in a laptop or the move to a different adapter in a desktop so this will be tested and
		// the device will be recreated if needed.
		void OnDisplayContentsInvalidated(Platform::Object^ sender);
		// Called when the game needs to be drawn. This member function serves to run the game loop for our game.
		void OnRendering(Object^ sender, Object^ args);

		// Stores the event registration token for our subscription to the Windows::UI::Xaml::Media::CompositionTarget::Rendering event.
		Windows::Foundation::EventRegistrationToken			m_renderingEventToken;

		// The game.
		Game^												m_game;

		// Stores the last mouse/touch point. This sample is designed to track only one touch and will disregard other touches until the first
		// touch is lifted.
		Windows::Foundation::Point							m_lastPoint;
		// If true, the value of m_lastPoint represents a position from a currently active touch or from the mouse while the left mouse button is down.
		bool												m_lastPointValid;

		// A simple timer for tracking elapsed time between OnRendering calls and the total elapsed time since the game has started.
		BasicTimer^											m_timer;
	};
}
