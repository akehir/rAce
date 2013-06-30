//
// App.xaml.h
// Declaration of the App class.
//

#pragma once

#include "App.g.h"
#include "DirectXPage.xaml.h"
#include "SettingsFlyout.xaml.h"
#include "Game.h"
#include "UICommand.h"
#include "BindableBase.h"
#include "BooleanNegationConverter.h"
#include "BooleanToVisibilityConverter.h"
#include "MultipleConvertersConverter.h"

namespace WindowsStoreDirectXGame
{
	/// <summary>
	/// Provides application-specific behavior to supplement the default Application class.
	/// Note that App inherits from the Windows::UI::Xaml::Application class (but it only shows this in
	/// the App.g.h generated header file).
	/// </summary>
	ref class App sealed
	{
	public:
		// The constructor. This is, in effect, the entry point for the game. You should avoid doing much here.
		App();
		// This is the event handler that runs when the game is launched normally.
		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ args) override;
		// This is the event handler that runs whenever the app is started. We do not use this in this game but
		// sample code showing how to use it is included for completeness. See the implementation for more detail.
		virtual void OnActivated(Windows::ApplicationModel::Activation::IActivatedEventArgs^ args) override;
		// This is the event handler that runs whenever the main window is activated or deactivated. This occurs in
		// situations ranging from bringing up the settings popup to switching between the game and another app when
		// the game is in Filled or Snapped mode.
		virtual void OnWindowActivationChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowActivatedEventArgs^ args);
		// This is the event handler that runs when the window has been created. We use it to store the window size, subscribe to the 
		// window's SizeChanged event, and to subscribe to the SettingsPane's CommandsRequested event.
		virtual void OnWindowCreated(Windows::UI::Xaml::WindowCreatedEventArgs^ args) override;

	private:
		// Runs when the game is suspending, such as when the user switches away from it. The game may be terminated without ever resuming, so this is
		// where you should save any critical game info. There are time limits for suspending in the App Cert Reqs which you must abide by so make
		// sure you do not need to save a lot of data here.
		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
		// Implements the Settings contract by properly populating the settings pane when requested.
		void OnSettingsCommandsRequested(Windows::UI::ApplicationSettings::SettingsPane^ sender, Windows::UI::ApplicationSettings::SettingsPaneCommandsRequestedEventArgs^ args);
		// Ensure that the settings popup and flyout and properly sized and positioned.
		void ResizeAndPositionCustomSettingsPane(WindowsStoreDirectXGame::SettingsFlyoutWidth width);
		// Transitions to the Game Settings settings and ensures that it displays properly.
		void SettingsPaneGameSettings(Windows::UI::Popups::IUICommand^ cmd);
		// Transitions to the Privacy settings and ensures that it displays properly.
		void SettingsPanePrivacyPolicy(Windows::UI::Popups::IUICommand^ cmd);
		// Transitions to the About settings and ensures that it displays properly.
		void SettingsPaneAbout(Windows::UI::Popups::IUICommand^ cmd);

		// This is a reference counted pointer to the XAML page that hosts the game.
		DirectXPage^										m_directXPage;

		// A Popup to hold our SettingsFlyout custom settings control.
		Windows::UI::Xaml::Controls::Primitives::Popup^		m_settingsPopup;

		// This is a reference counted pointer to the SettingsFlyout custom control that hosts our custom settings.
		SettingsFlyout^										m_settingsFlyout;

		// Tracking variable so we know whether the settings command event is registered so that we can properly unregister it.
		bool												m_isSettingsCommandEventRegistered;
		// The event registration token used to store the subscription to the SettingsPane::GetForCurrentView()->CommandsRequested event.
		Windows::Foundation::EventRegistrationToken			m_settingsCommandEventToken;

		// This value is used to ensure that we don't play test sounds when we are preparing the game settings view.
		bool												m_preparingSettings;

		// This stores the window's width and height. We use the height to properly size our custom settings and the width to properly position it.
		Windows::Foundation::Rect							m_windowBounds;
	};
}

#if defined (_DEBUG)

// Called at startup to remember the main thread id for debug assertion checking
void RecordMainThread();

// Called in debug mode only for thread context assertion checks
bool IsMainThread();

// Called in debug mode only for thread context assertion checks
bool IsBackgroundThread();

#endif
