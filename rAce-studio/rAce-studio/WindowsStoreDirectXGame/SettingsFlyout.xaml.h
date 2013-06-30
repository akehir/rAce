//
// SettingsNarrow.xaml.h
// Declaration of the SettingsNarrow class
//

#pragma once

#include "SettingsFlyout.g.h"

namespace WindowsStoreDirectXGame
{
	public enum class SettingsFlyoutWidth
	{
		Narrow =	346,
		Wide =		646,
	};

	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class SettingsFlyout sealed
	{
	public:
		SettingsFlyout();
		SettingsFlyout(SettingsFlyoutWidth settingsFlyoutWidth);
		void BackButtonClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ args);
	};
}
