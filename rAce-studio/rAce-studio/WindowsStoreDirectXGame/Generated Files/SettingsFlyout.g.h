﻿

#pragma once
//------------------------------------------------------------------------------
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//------------------------------------------------------------------------------

namespace Windows {
    namespace UI {
        namespace Xaml {
            ref class VisualStateGroup;
            ref class VisualState;
        }
    }
}
namespace Windows {
    namespace UI {
        namespace Xaml {
            namespace Controls {
                ref class Grid;
                ref class StackPanel;
                ref class TextBlock;
                ref class ToggleSwitch;
                ref class Slider;
                ref class HyperlinkButton;
            }
        }
    }
}

namespace WindowsStoreDirectXGame
{
    partial ref class SettingsFlyout : public ::Windows::UI::Xaml::Controls::UserControl, 
        public ::Windows::UI::Xaml::Markup::IComponentConnector
    {
    public:
        void InitializeComponent();
        virtual void Connect(int connectionId, ::Platform::Object^ target);
    
    private:
        bool _contentLoaded;
    
        private: ::Windows::UI::Xaml::VisualStateGroup^ SettingsStates;
        private: ::Windows::UI::Xaml::VisualState^ GameSettingsState;
        private: ::Windows::UI::Xaml::VisualState^ PrivacyPolicyState;
        private: ::Windows::UI::Xaml::VisualState^ AboutState;
        private: ::Windows::UI::Xaml::Controls::Grid^ SettingsHeader;
        private: ::Windows::UI::Xaml::Controls::Grid^ SettingsGrid;
        private: ::Windows::UI::Xaml::Controls::StackPanel^ GameSettingsStackPanel;
        private: ::Windows::UI::Xaml::Controls::StackPanel^ PrivacyPolicyStackPanel;
        private: ::Windows::UI::Xaml::Controls::StackPanel^ AboutStackPanel;
        private: ::Windows::UI::Xaml::Controls::TextBlock^ AboutAppNameTextBlock;
        private: ::Windows::UI::Xaml::Controls::TextBlock^ AboutPublisherNameTextBlock;
        private: ::Windows::UI::Xaml::Controls::TextBlock^ AboutVersionNumberTextBlock;
        private: ::Windows::UI::Xaml::Controls::ToggleSwitch^ MusicOnOffToggleSwitch;
        private: ::Windows::UI::Xaml::Controls::Slider^ MusicVolumeSlider;
        private: ::Windows::UI::Xaml::Controls::ToggleSwitch^ SoundEffectsOnOffToggleSwitch;
        private: ::Windows::UI::Xaml::Controls::Slider^ SoundEffectsVolumeSlider;
        private: ::Windows::UI::Xaml::Controls::TextBlock^ WindowsMediaNotInstalledTextBlock;
        private: ::Windows::UI::Xaml::Controls::HyperlinkButton^ WindowsMediaNotInstalledHyperlinkButton;
        private: ::Windows::UI::Xaml::Controls::TextBlock^ PageTitleTextBlock;
    };
}

