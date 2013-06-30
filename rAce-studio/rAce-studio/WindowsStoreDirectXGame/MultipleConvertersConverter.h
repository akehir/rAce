#pragma once
#include <vector>
#include <collection.h>

namespace WindowsStoreDirectXGame
{
	// Value converter that allows you to combine multiple value converters together in XAML. The XAML syntax would look like this:
	//     <Page.Resources>
	//        <local:MultipleConvertersConverter x:Key="NegatedBooleanToVisibilityConverter">
	//            <local:BooleanNegationConverter />
	//            <local:BooleanToVisibilityConverter />
	//        </local:MultipleConvertersConverter>
	//    </Page.Resources>
	[Windows::Foundation::Metadata::WebHostHidden]
	[Windows::UI::Xaml::Markup::ContentProperty(Name="Converters")]
	public ref class MultipleConvertersConverter sealed : Windows::UI::Xaml::Data::IValueConverter
	{
	public:
		MultipleConvertersConverter() :
			m_converters(ref new Platform::Collections::Vector<Windows::UI::Xaml::Data::IValueConverter^>())
		{ }

        // This runs all the value through all of the converters in order, returning the result. If there are no child converters
        // then the result is returned unchanged. Note that all converters get the same targetType, parameter, and language arguments.
		virtual Platform::Object^ Convert(Platform::Object^ value, Windows::UI::Xaml::Interop::TypeName targetType, Platform::Object^ parameter, Platform::String^ language)
		{
			Platform::Object^ result;
			result = value;
			for (auto i = 0U; i < m_converters->Size; ++i)
			{
				auto converter = m_converters->GetAt(i);
				result = converter->Convert(result, targetType, parameter, language);
			}

			return result;
		}

        // This runs all the value through all of the converters in reverse order, returning the result. If there are no child converters
        // then the result is returned unchanged. Note that all converters get the same targetType, parameter, and language arguments.
		virtual Platform::Object^ ConvertBack(Platform::Object^ value, Windows::UI::Xaml::Interop::TypeName targetType, Platform::Object^ parameter, Platform::String^ language)
		{
			Platform::Object^ result;
			result = value;
			for (auto i = m_converters->Size - 1; i >= 0; --i)
			{
				auto converter = m_converters->GetAt(i);
				result = converter->Convert(result, targetType, parameter, language);
			}

			return result;
		}

        // This is what all the child converters are added to.
		property Windows::Foundation::Collections::IVector<Windows::UI::Xaml::Data::IValueConverter^>^ Converters
		{
			Windows::Foundation::Collections::IVector<Windows::UI::Xaml::Data::IValueConverter^>^ get() { return m_converters; }
		}

	private:
        // This is the actual backing store for the converters. You can't publically expose a Platform::Collections::Vector<T> so
        // we use the IVector<T> as the 
		Platform::Collections::Vector<Windows::UI::Xaml::Data::IValueConverter^>^		m_converters;
	};
}
