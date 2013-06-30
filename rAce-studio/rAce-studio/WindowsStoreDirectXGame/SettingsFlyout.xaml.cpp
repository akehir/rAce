//
// SettingsNarrow.xaml.cpp
// Implementation of the SettingsNarrow class
//

#include "pch.h"
#include "SettingsFlyout.xaml.h"

using namespace WindowsStoreDirectXGame;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

SettingsFlyout::SettingsFlyout()
{
	Width = static_cast<double>(SettingsFlyoutWidth::Narrow);
	InitializeComponent();
}

SettingsFlyout::SettingsFlyout(SettingsFlyoutWidth settingsFlyoutWidth)
{
	Width = static_cast<double>(settingsFlyoutWidth);
	InitializeComponent();
}

void SettingsFlyout::BackButtonClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ args)
{
	auto parentType = Parent->GetType();
	auto t = Boolean::typeid;
	//if (parentType::typeid 
	safe_cast<Popup^>(Parent)->IsOpen = false;
	Windows::UI::ApplicationSettings::SettingsPane::Show();
}
