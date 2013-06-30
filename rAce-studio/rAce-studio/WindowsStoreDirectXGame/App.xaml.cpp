//
// App.xaml.cpp
// Implementation of the App class.
//

#include "pch.h"
#include "DirectXPage.xaml.h"

using namespace WindowsStoreDirectXGame;

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::UI::ApplicationSettings;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App() :
	m_directXPage(),
	m_settingsPopup(),
	m_settingsFlyout(),
	m_isSettingsCommandEventRegistered(),
	m_settingsCommandEventToken(),
	m_preparingSettings(),
	m_windowBounds()
{
#if defined(_DEBUG)
	// Remember the thread ID of the main thread for assertion checking of runtime context. Operations that will
	// effect any of the XAML UI must run on the main thread. Recording the main thread so you can check to ensure
	// that an async operation that affects the XAML UI is running on the main thread will help with debugging during
	// development.
	RecordMainThread();
#endif
	InitializeComponent();
	Suspending += ref new SuspendingEventHandler(this, &App::OnSuspending);
}

/// <summary>
/// Invoked when the application is launched normally by the end user.  Other entry points
/// will be used when the application is launched to open a specific file, to display
/// search results, and so forth.
/// </summary>
/// <param name="args">Details about the launch request and process.</param>
void App::OnLaunched(LaunchActivatedEventArgs^ args)
{
	m_directXPage = ref new DirectXPage();

	//auto previousExecutionState = args->PreviousExecutionState;
	//if (previousExecutionState == ApplicationExecutionState::Terminated || previousExecutionState == ApplicationExecutionState::ClosedByUser)
	//{
	//}

	// Loads any saved state from a previous run.
	m_directXPage->LoadInternalState();

	// Place the page in the current window and ensure that it is active.
	Window::Current->Content = m_directXPage;
	Window::Current->Activate();

	// Sign up our event handlers for key down, key up, pointer pressed, pointer moved, and pointer released.
	Window::Current->CoreWindow->KeyDown += 
		ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(m_directXPage->GetGame(), &Game::KeyDown);

	Window::Current->CoreWindow->KeyUp +=
		ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(m_directXPage->GetGame(), &Game::KeyUp);

	Window::Current->CoreWindow->PointerPressed +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(m_directXPage->GetGame(), &Game::PointerPressed);

	Window::Current->CoreWindow->PointerMoved +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(m_directXPage->GetGame(), &Game::PointerMoved);

	Window::Current->CoreWindow->PointerReleased +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(m_directXPage->GetGame(), &Game::PointerReleased);

	// Create settings flyout and set the event handlers.
	m_settingsFlyout = ref new SettingsFlyout(SettingsFlyoutWidth::Narrow);

	// Access the AppxManifest.xml for the game to get certain values for the SettingsFlyout views.
	auto asyncBackgroundColorTask = concurrency::create_task([this]()
	{
		return Windows::ApplicationModel::Package::Current->InstalledLocation->GetFileAsync("AppxManifest.xml");
	}).then([this](Windows::Storage::StorageFile^ file)
	{
		return Windows::Data::Xml::Dom::XmlDocument::LoadFromFileAsync(file);
	}).then([this](Windows::Data::Xml::Dom::XmlDocument^ xmlDoc)
	{
		// Assign the application's BackgroundColor from its manifest to be the background color of the settings flyout header, as per
		// the App Settings guidelines: http://msdn.microsoft.com/en-us/library/windows/apps/hh770544.aspx

		// First, find the first element with a VisualElements tag. There should only be one. http://msdn.microsoft.com/en-us/library/windows/apps/br211472.aspx
		auto visualElementsNode = xmlDoc->GetElementsByTagName("VisualElements")->GetAt(0);

		// Get the BackgroundColor tag that's contained in the VisualElements tag.
		auto colorAttribute = visualElementsNode->Attributes->GetNamedItem("BackgroundColor");

		// Cast the BackgroundColor's value as a string (which it should be as it's XML text). It will be either a named color or a #RRGGBB color,
		// either of which is fine for our purposes.
		auto backgroundColorString = safe_cast<Platform::String^>(colorAttribute->NodeValue);

		// Create a string of XAML text which we can load and insert into the settings flyout. See the documentation on XamlReader::Load for more info on
		// how this all works. 
		auto xamlText = "<SolidColorBrush xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" Color=\"" + backgroundColorString + "\" />";
		auto xaml = Windows::UI::Xaml::Markup::XamlReader::Load(xamlText);

		// Now that we've loaded the generated XAML, it will be a SolidColorBrush, which we just need to cast as a Brush.
		auto brush = safe_cast<Windows::UI::Xaml::Media::Brush^>(xaml);

		// Assign the brush to be the background color of the SettingsHeader grid in the settings flyout. This ensures that the app tile color and settings
		// flyout color match.
		safe_cast<Windows::UI::Xaml::Controls::Grid^>(m_settingsFlyout->FindName("SettingsHeader"))->Background = brush;

		// Next task.
		// Assign the version number to the about page's version number text block.
		auto identityNode = xmlDoc->GetElementsByTagName("Identity")->GetAt(0);
		safe_cast<Windows::UI::Xaml::Controls::TextBlock^>(m_settingsFlyout->FindName("AboutVersionNumberTextBlock"))->Text =
			safe_cast<Platform::String^>(identityNode->Attributes->GetNamedItem("Version")->NodeValue);

		// Assign the publisher name to the about page's publisher name text block.
		auto publisherNode = xmlDoc->GetElementsByTagName("PublisherDisplayName")->GetAt(0);
		safe_cast<Windows::UI::Xaml::Controls::TextBlock^>(m_settingsFlyout->FindName("AboutPublisherNameTextBlock"))->Text =
			safe_cast<Platform::String^>(publisherNode->InnerText);

		// Assign the app's display name to the about page's app name text block.
		safe_cast<Windows::UI::Xaml::Controls::TextBlock^>(m_settingsFlyout->FindName("AboutAppNameTextBlock"))->Text =
			safe_cast<Platform::String^>(visualElementsNode->Attributes->GetNamedItem("DisplayName")->NodeValue);
	});

	// Get the game settings stack panel.
	auto sp = safe_cast<Windows::UI::Xaml::Controls::StackPanel^>(m_settingsFlyout->FindName("GameSettingsStackPanel"));

	// Get the music on/off toggle switch, set it to its proper value, and hook up a Toggled event handler.
	auto toggleSwitch = safe_cast<Windows::UI::Xaml::Controls::ToggleSwitch^>(sp->FindName("MusicOnOffToggleSwitch"));
	toggleSwitch->IsOn = !m_directXPage->GetGame()->GetAudioEngine()->GetMusicOff();
	toggleSwitch->Toggled += ref new Windows::UI::Xaml::RoutedEventHandler([this](Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ args)
	{
		auto sp = safe_cast<Windows::UI::Xaml::Controls::StackPanel^>(m_settingsFlyout->FindName("GameSettingsStackPanel"));

		auto toggleSwitch = safe_cast<Windows::UI::Xaml::Controls::ToggleSwitch^>(sp->FindName("MusicOnOffToggleSwitch"));
		toggleSwitch->IsOn = !m_directXPage->GetGame()->GetAudioEngine()->SetMusicOnOff(!toggleSwitch->IsOn);
	});

	// Get the music volume slider.
	auto slider = safe_cast<Windows::UI::Xaml::Controls::Slider^>(sp->FindName("MusicVolumeSlider"));
	// Have it make the volume test sound when we the player hasn't changed the value.
	slider->Tapped += ref new Windows::UI::Xaml::Input::TappedEventHandler([this](Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ args)
	{
		auto audioEngine = m_directXPage->GetGame()->GetAudioEngine();
		auto slider = dynamic_cast<Windows::UI::Xaml::Controls::Slider^>(sender);
		if (!m_preparingSettings && slider != nullptr && (fabs(audioEngine->GetMusicVolume() - (slider->Value)) < 0.1))
		{
			audioEngine->PlayMusicVolumeTestSound();
		}
	});
	// Have it update the value internally and make the volume test sound when the player has changed the value.
	slider->ValueChanged += ref new Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventHandler([this](Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ args)
	{
		auto audioEngine = m_directXPage->GetGame()->GetAudioEngine();
		if (!m_preparingSettings)
		{
			audioEngine->SetMusicVolume(args->NewValue);
			audioEngine->PlayMusicVolumeTestSound();
		}
	});

	// Get the sound effects on/off toggle switch, set it to its proper value, and hook up a Toggled event handler.
	toggleSwitch = safe_cast<Windows::UI::Xaml::Controls::ToggleSwitch^>(sp->FindName("SoundEffectsOnOffToggleSwitch"));
	toggleSwitch->IsOn = !m_directXPage->GetGame()->GetAudioEngine()->GetSoundEffectsOff();
	toggleSwitch->Toggled += ref new Windows::UI::Xaml::RoutedEventHandler([this](Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ args)
	{
		auto sp = safe_cast<Windows::UI::Xaml::Controls::StackPanel^>(m_settingsFlyout->FindName("GameSettingsStackPanel"));

		auto toggleSwitch = safe_cast<Windows::UI::Xaml::Controls::ToggleSwitch^>(sp->FindName("SoundEffectsOnOffToggleSwitch"));
		toggleSwitch->IsOn = !m_directXPage->GetGame()->GetAudioEngine()->SetSoundEffectsOnOff(!toggleSwitch->IsOn);
	});

	// Get the sound effects volume slider.
	slider = safe_cast<Windows::UI::Xaml::Controls::Slider^>(sp->FindName("SoundEffectsVolumeSlider"));
	// Have it make the test sound when we the player hasn't changed the value.
	slider->Tapped += ref new Windows::UI::Xaml::Input::TappedEventHandler([this](Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ args)
	{
		auto audioEngine = m_directXPage->GetGame()->GetAudioEngine();
		auto slider = dynamic_cast<Windows::UI::Xaml::Controls::Slider^>(sender);
		if (!m_preparingSettings && slider != nullptr && (fabs(audioEngine->GetSoundEffectsVolume() - (slider->Value)) < 0.1f))
		{
			audioEngine->PlaySoundEffectsVolumeTestSound();
		}
	});
	// Have it update the value internally and make the volume test sound when the player has changed the value.
	slider->ValueChanged += ref new Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventHandler([this](Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ args)
	{
		auto audioEngine = m_directXPage->GetGame()->GetAudioEngine();
		if (!m_preparingSettings)
		{
			audioEngine->SetSoundEffectsVolume(args->NewValue);
			audioEngine->PlaySoundEffectsVolumeTestSound();
		}
	});

	// Create the popup for the SettingsFlyout. A Popup is a simple child window that o
	m_settingsPopup = ref new Popup();
	// Tell the game that the game window is deactivated when the settings popup opens.
	m_settingsPopup->Opened += ref new Windows::Foundation::EventHandler<Platform::Object^>([this](Platform::Object^ sender, Platform::Object^ e)
	{
		m_directXPage->GetGame()->SetWindowIsDeactivated(true);
	});
	// Tell the game that the game window is no longer deactivated when the settings popup closes.
	m_settingsPopup->Closed += ref new Windows::Foundation::EventHandler<Platform::Object^>([this](Platform::Object^ sender, Platform::Object^ e)
	{
		m_directXPage->GetGame()->SetWindowIsDeactivated(false);
	});
	// Set the popup to automatically close itself if the user taps or clicks outside of the settings flyout.
	m_settingsPopup->IsLightDismissEnabled = true;
	// Make the settings flyout the child of the settings popup. The settings flyout is the XAML page where all of our custom settings are actually
	// implemented.
	m_settingsPopup->Child = m_settingsFlyout;

	// Hook up the event handler for changes in the activation state of the game's window.
	Window::Current->CoreWindow->Activated += ref new TypedEventHandler<CoreWindow^, WindowActivatedEventArgs^>(this, &App::OnWindowActivationChanged);
}

void App::OnActivated(Windows::ApplicationModel::Activation::IActivatedEventArgs^ args)
{
	// This member function runs for all activation events, such as activation by Launch, from the Search charm, or from the Share charm. For
	// more info, see: http://msdn.microsoft.com/en-us/library/windows/apps/hh464906.aspx . Note that you must register your game
	// for each ActivationKind (except for Launch) that you wish to support in order for your app to be a target of the contract or extension
	// that you desire. You can do this in the Package.appxmanifest editor.

	// To differentiate between different types of activation, check the args->Kind property. We currently do nothing for any of these since
	// we handle Launch in App::OnLaunched. Several other types of activation also have On________ member functions that you can override. See
	// the Application class documentation for more info: http://msdn.microsoft.com/en-us/library/windows/apps/xaml/br242324.aspx .

	// The following code intentionally does nothing in this sample.
	// Note that we assign the ActivationKind from the args to a variable and then switch on that variable. Because of how Visual Studio
	// currently handles the C++ 'switch' code snippet, it will auto populate a switch on an enum when you have the enum stored in a variable 
	// but won't if you try to switch directly on a property. So we store the value of the property in a variable first and voila, we get all
	// the enum values just by typing 'switch', pressing tab, then typing 'activationKind' and pressing Enter.
	auto activationKind = args->Kind;
	switch (activationKind)
	{
	case Windows::ApplicationModel::Activation::ActivationKind::Launch:
		break;
	case Windows::ApplicationModel::Activation::ActivationKind::Search:
		break;
	case Windows::ApplicationModel::Activation::ActivationKind::ShareTarget:
		break;
	case Windows::ApplicationModel::Activation::ActivationKind::File:
		break;
	case Windows::ApplicationModel::Activation::ActivationKind::Protocol:
		break;
	case Windows::ApplicationModel::Activation::ActivationKind::FileOpenPicker:
		break;
	case Windows::ApplicationModel::Activation::ActivationKind::FileSavePicker:
		break;
	case Windows::ApplicationModel::Activation::ActivationKind::CachedFileUpdater:
		break;
	case Windows::ApplicationModel::Activation::ActivationKind::ContactPicker:
		break;
	case Windows::ApplicationModel::Activation::ActivationKind::Device:
		break;
	case Windows::ApplicationModel::Activation::ActivationKind::PrintTaskSettings:
		break;
	case Windows::ApplicationModel::Activation::ActivationKind::CameraSettings:
		break;
	default:
		break;
	}
}

void App::OnWindowActivationChanged(CoreWindow^ sender, WindowActivatedEventArgs^ args)
{
	auto activationState = args->WindowActivationState;
	auto game = m_directXPage->GetGame();

	//// You can uncomment this code if you are curious about when an application is considered to be CodeActivated vs PointerActivated.
	//OutputDebugStringW(std::wstring(L"Activation State: ").append(
	//	(activationState == CoreWindowActivationState::Deactivated) ? L"Deactivated.\n" :
	//	((activationState == CoreWindowActivationState::CodeActivated) ? L"Code activated.\n" : L"Pointer activated.\n")).c_str());

	// Activate or Deactivate the game based on the current CoreWindowActivationState.
	switch (activationState)
	{
	case Windows::UI::Core::CoreWindowActivationState::CodeActivated:
		game->SetWindowIsDeactivated(false);
		break;
	case Windows::UI::Core::CoreWindowActivationState::Deactivated:
		game->SetWindowIsDeactivated(true);
		break;
	case Windows::UI::Core::CoreWindowActivationState::PointerActivated:
		game->SetWindowIsDeactivated(false);
		break;
	default:
		break;
	}
}

void App::OnWindowCreated(Windows::UI::Xaml::WindowCreatedEventArgs^ args)
{
	// Store the window bounds.
	m_windowBounds = args->Window->Bounds;

	// We subscribed to SizeChanged to ensure that the settings flyout always appears properly sized and positioned.
	args->Window->SizeChanged += ref new Windows::UI::Xaml::WindowSizeChangedEventHandler([&](Platform::Object^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args)
	{
		(sender); // Unused parameter.

		m_windowBounds.Width = args->Size.Width;
		m_windowBounds.Height = args->Size.Height;
	});

	// We subscribe to CommandsRequested in order to display our custom settings pane commands. This is part of implementing the Settings contract. For
	// more info see http://msdn.microsoft.com/en-us/library/windows/apps/xaml/hh770543.aspx and http://msdn.microsoft.com/en-us/library/windows/apps/hh770544.aspx .
	m_settingsCommandEventToken = SettingsPane::GetForCurrentView()->CommandsRequested +=
		ref new TypedEventHandler<SettingsPane^, SettingsPaneCommandsRequestedEventArgs^>(this, &App::OnSettingsCommandsRequested);
}

/// <summary>
/// Invoked when the application is being suspended.
/// </summary>
/// <param name="sender">Details about the origin of the event.</param>
/// <param name="args">Details about the suspending event.</param>
void App::OnSuspending(Object^ sender, SuspendingEventArgs^ args)
{
	(sender); // Unused parameter.
	(args); // Unused parameter.

	// Call DirectXPage::SaveInternalState. Doing this rather than calling the game's save method directly lets us have the option
	// of saving state that's part of the DirectXPage itself. How you setup your game and how much use you make of XAML will 
	// determine how much use, if any, you make of the ability to save state for the DirectXPage.
	m_directXPage->SaveInternalState();
}

void App::OnSettingsCommandsRequested(SettingsPane^ sender, SettingsPaneCommandsRequestedEventArgs^ args)
{
	// Creating a ResourceLoader lets us use RESW files to display strings in the appropriate language.
	auto resourceLoader = ref new Windows::ApplicationModel::Resources::ResourceLoader("SettingsResources");

	// Add a "Game Settings" command to the settings pane.
	auto cmd = ref new SettingsCommand(
		"gamesettings",
		resourceLoader->GetString("GameSettings"),
		ref new Windows::UI::Popups::UICommandInvokedHandler(this, &App::SettingsPaneGameSettings)
		);
	args->Request->ApplicationCommands->Append(cmd);

	// Add a "Privacy Policy" command to the settings pane.
	cmd = ref new SettingsCommand(
		"privacypolicysettings",
		resourceLoader->GetString("PrivacyPolicy"),
		ref new Windows::UI::Popups::UICommandInvokedHandler(this, &App::SettingsPanePrivacyPolicy)
		);
	args->Request->ApplicationCommands->Append(cmd);

	// Add an "About" command to the settings pane.
	cmd = ref new SettingsCommand(
		"aboutsettings",
		resourceLoader->GetString("About"),
		ref new Windows::UI::Popups::UICommandInvokedHandler(this, &App::SettingsPaneAbout)
		);
	args->Request->ApplicationCommands->Append(cmd);
}

void App::ResizeAndPositionCustomSettingsPane(SettingsFlyoutWidth width)
{
	// Set the size of the settings popup. The height must always equal the window height. The
	// width must be either 346 (narrow) or 646 (wide).
	m_settingsPopup->Width = static_cast<double>(width);
	m_settingsPopup->Height = m_windowBounds.Height;
	m_settingsFlyout->Width = m_settingsPopup->Width;
	m_settingsFlyout->Height = m_settingsPopup->Height;

	// RTL languages have the charms bar on the left side of the screen.
	double sideOffset = (SettingsPane::Edge == SettingsEdgeLocation::Right) ?
		m_windowBounds.Width - m_settingsPopup->Width :
	0.0;

	// Properly position the settings popup based on its dimensions and the language settings.
	m_settingsPopup->SetValue(Canvas::LeftProperty, sideOffset);
	double top = 0.0;
	m_settingsPopup->SetValue(Canvas::TopProperty, top);
}

void App::SettingsPaneGameSettings(Windows::UI::Popups::IUICommand^ cmd)
{
	// Indicate that we are preparing settings. Checking this value in event handlers for controls that might have side effects (such as the
	// volume sliders in the game settings pane, which play a test sound when the volume is set) allows us to bypass those side effects if
	// desired.
	m_preparingSettings = true;


	// Set the page title. Since we reuse the same popup for all of the settings views, this must be done in code rather than in XAML.
	safe_cast<Windows::UI::Xaml::Controls::TextBlock^>(m_settingsFlyout->FindName("PageTitleTextBlock"))->Text = 
		(ref new Windows::ApplicationModel::Resources::ResourceLoader("SettingsResources"))->GetString("GameSettingsPageTitle");

	// Configure the game settings settings pane.

	// Find the GameSettingsStackPanel.
	// safe_cast<T> is a C++/CX extension that lets us cast a WinRT object to a WinRT type or interface that it is/implements. If
	// you have an improper cast (e.g. you tried to cast a StackPanel to a TextBlock) a Platform::InvalidCastException would be thrown.
	// Using FrameworkElement::FindName lets us find an object specified in a XAML file by using the Name we assigned to it. If you go into
	// SettingsFlyout.xaml you will find a StackPanel tag with x:Name="GameSettingsStackPanel" as one of its attributes.
	auto sp = safe_cast<Windows::UI::Xaml::Controls::StackPanel^>(m_settingsFlyout->FindName("GameSettingsStackPanel"));

	auto game = m_directXPage->GetGame();

	// If there was an error initializing Media Foundation, display some information informing the user of the problem and
	// suggest that if they are running an N or KN version that they install the Microsoft Media Feature Pack, then display a link
	// directing them to it.
	// Note that this requires internet access and thus you will need at least a minimal privacy policy that complies with the 
	// disclosure rules of the App Certification Requirements.
	auto windowsMediaVisibility = game->GetAudioEngine()->GetNoMediaFoundation() ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
	safe_cast<Windows::UI::Xaml::Controls::TextBlock^>(sp->FindName("WindowsMediaNotInstalledTextBlock"))->Visibility =
		windowsMediaVisibility;
	safe_cast<Windows::UI::Xaml::Controls::HyperlinkButton^>(sp->FindName("WindowsMediaNotInstalledHyperlinkButton"))->Visibility =
		windowsMediaVisibility;

	// Ensure that the Music On/Off toggle is set to its proper value.
	auto toggleSwitch = safe_cast<Windows::UI::Xaml::Controls::ToggleSwitch^>(sp->FindName("MusicOnOffToggleSwitch"));
	if (toggleSwitch->IsOn != (!m_directXPage->GetGame()->GetAudioEngine()->GetMusicOff()))
	{
		toggleSwitch->IsOn = !m_directXPage->GetGame()->GetAudioEngine()->GetMusicOff();
	}

	// Ensure that the Music Volume slider is set to its proper value.
	auto slider = safe_cast<Windows::UI::Xaml::Controls::Slider^>(sp->FindName("MusicVolumeSlider"));
	slider->Value = m_directXPage->GetGame()->GetAudioEngine()->GetMusicVolume();

	// Ensure that the Sound Effects On/Off toggle is set to its proper value.
	toggleSwitch = safe_cast<Windows::UI::Xaml::Controls::ToggleSwitch^>(sp->FindName("SoundEffectsOnOffToggleSwitch"));
	if (toggleSwitch->IsOn != (!m_directXPage->GetGame()->GetAudioEngine()->GetSoundEffectsOff()))
	{
		toggleSwitch->IsOn = !m_directXPage->GetGame()->GetAudioEngine()->GetSoundEffectsOff();
	}

	// Ensure that the Sound Effects Volume slider is set to its proper value.
	slider = safe_cast<Windows::UI::Xaml::Controls::Slider^>(sp->FindName("SoundEffectsVolumeSlider"));
	slider->Value = m_directXPage->GetGame()->GetAudioEngine()->GetSoundEffectsVolume();

	// Finished configuring the game settings settings pane.

	// Make sure the settings popup and flyout are the proper width and height and are positioned correctly.
	// Note: If a translation causes this settings view to not fit in a narrow width, use SettingsFlyoutWidth::Wide instead. 
	ResizeAndPositionCustomSettingsPane(SettingsFlyoutWidth::Narrow);

	// Open the settings popup.
	m_settingsPopup->IsOpen = true;

	// Use the VisualStateManager to switch to our "GameSettingsState" view. I created these views and the appropriate visual states
	// using Blend for Visual Studio 2012. For more information, see: http://msdn.microsoft.com/en-us/library/windows/apps/jj155089.aspx and
	// http://msdn.microsoft.com/en-us/library/windows/apps/jj154981.aspx .
	//
	// If you are a developer, Blend can seem formidable at first. Just like you needed to learn how to program using an IDE, you will need
	// to spend time learning how to design using Blend. If you read through the documentation at the first link (including all the subtopics
	// such as "Working with object and properties" and all of their subtopics), and you experiment with Blend while following along with
	// the documentation, you will come to learn it and then you will be able to do design work in addition to development work. Even if you
	// work with a designer (or hire one later to help polish up all the designs), knowing how the design tools work will help you better
	// understand XAML and the work that designers do, which will make it much easier for you both to communicate from a common base of knowledge.
	//
	// If you already know Blend, the second link is a direct link to the documentation page that shows how to define and modify visual states.
	VisualStateManager::GoToState(m_settingsFlyout, "GameSettingsState", true);

	// Indicate that we are done preparing settings so that anything we were suppressing (such as playing the volume test noise when setting the
	// volume sliders) will no longer be suppressed.
	m_preparingSettings = false;
}

void App::SettingsPanePrivacyPolicy(Windows::UI::Popups::IUICommand^ cmd)
{
	// Indicate that we are preparing settings. Checking this value in event handlers for controls that might have side effects (such as the
	// volume sliders in the game settings pane, which play a test sound when the volume is set) allows us to bypass those side effects if
	// desired.
	m_preparingSettings = true;

	// Set the page title. Since we reuse the same popup for all of the settings views, this must be done in code rather than in XAML.
	safe_cast<Windows::UI::Xaml::Controls::TextBlock^>(m_settingsFlyout->FindName("PageTitleTextBlock"))->Text = 
		(ref new Windows::ApplicationModel::Resources::ResourceLoader("SettingsResources"))->GetString("PrivacyPolicyPageTitle");

	// The sample has nothing specific to the Privacy settings pane to do here. If your game does need code for it, put it here.

	// Make sure the settings popup and flyout are the proper width and height and are positioned correctly.
	// Note: If a translation causes this settings view to not fit in a narrow width, use SettingsFlyoutWidth::Wide instead. 
	ResizeAndPositionCustomSettingsPane(SettingsFlyoutWidth::Narrow);

	// Open the settings popup.
	m_settingsPopup->IsOpen = true;

	// Transition to displaying the PrivacyPolicyState in the settings flyout.
	VisualStateManager::GoToState(m_settingsFlyout, "PrivacyPolicyState", true);

	// Indicate that we are done preparing settings so that anything we were suppressing during preparation will no longer be suppressed.
	m_preparingSettings = false;
}

void App::SettingsPaneAbout(Windows::UI::Popups::IUICommand^ cmd)
{
	// Indicate that we are preparing settings. Checking this value in event handlers for controls that might have side effects (such as the
	// volume sliders in the game settings pane, which play a test sound when the volume is set) allows us to bypass those side effects if
	// desired.
	m_preparingSettings = true;

	// Set the page title. Since we reuse the same popup for all of the settings views, this must be done in code rather than in XAML.
	safe_cast<Windows::UI::Xaml::Controls::TextBlock^>(m_settingsFlyout->FindName("PageTitleTextBlock"))->Text = 
		(ref new Windows::ApplicationModel::Resources::ResourceLoader("SettingsResources"))->GetString("AboutPageTitle");

	// The sample has nothing specific to the About settings pane to do here. If your game does need code for it, put it here.

	// Make sure the settings popup and flyout are the proper width and height and are positioned correctly.
	// Note: If a translation causes this settings view to not fit in a narrow width, use SettingsFlyoutWidth::Wide instead. 
	ResizeAndPositionCustomSettingsPane(SettingsFlyoutWidth::Narrow);

	// Open the settings popup.
	m_settingsPopup->IsOpen = true;

	// Transition to displaying the AboutState in the settings flyout.
	VisualStateManager::GoToState(m_settingsFlyout, "AboutState", true);

	// Indicate that we are done preparing settings so that anything we were suppressing during preparation will no longer be suppressed.
	m_preparingSettings = false;
}

#if defined (_DEBUG)

// Put MainThreadId into a namespace to avoid polluting the global namespace.
namespace MainThread
{
	// MainThreadId is used to record the main thread's ID to simplify checking async code that interacts with XAML during development of our application.
	static DWORD MainThreadId = 0U;
};

// The IsMainThread function returns true if the current thread is the app's main thread and false otherwise.
bool IsMainThread()
{
	return MainThread::MainThreadId == 0U || (MainThread::MainThreadId == GetCurrentThreadId());
}

// The IsBackgroundThread function returns false if the current thread is the app's main thread and true otherwise.
bool IsBackgroundThread()
{
	return MainThread::MainThreadId == 0U || (MainThread::MainThreadId != GetCurrentThreadId());
}

// The RecordMainThread function registers the main thread ID for use by the IsMainThread and IsBackgroundThread functions.
void RecordMainThread()
{
	MainThread::MainThreadId = GetCurrentThreadId();
}

#endif /* not NDEBUG */

