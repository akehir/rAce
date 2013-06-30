#pragma once

namespace WindowsStoreDirectXGame
{
	public delegate bool CanExecuteDelegate(Platform::Object^ parameter);
	public delegate void ExecuteDelegate(Platform::Object^ parameter);
};

// The UICommand class is a basic implementation of the Windows::UI::Xaml::Input::ICommand
// interface. This interface lets us bind functions, lambdas, and member functions to any 
// XAML control that derives from Windows::UI::Xaml::Controls::Primitives::ButtonBase. This
// helps keep your XAML page code-behind files clean and also helps with UI reusability and
// implementing the MVVM design pattern in general.
[Windows::Foundation::Metadata::WebHostHiddenAttribute()]
ref class UICommand sealed : public Windows::UI::Xaml::Input::ICommand
{
public:
    // Constructor. Use this overload when you only want to assign an Execute handler. The
    // command will be executable unless its executability is changed with SetCanExecute.
    // executeHandler - A delegate for a function, lambda, or member function in the form
    //		void Foo(Platform::Object^);
	UICommand(
		WindowsStoreDirectXGame::ExecuteDelegate^ executeHandler
		) : m_execute(executeHandler),
		m_canExecute(),
		m_canExecuteState(true)
	{ }

    // Constructor. Use this overload when you want to assign an Execute handler and a 
    // CanExecute handler. The command will be executable unless its executability is changed
    // with SetCanExecute or if the CanExecuteDelegate is non-null and it returns false after
    // the Execute handler is executed.
    // executeHandler - A delegate for a function, lambda, or member function in the form
    //		void Foo(Platform::Object^);
    // canExecuteHandler - A delegate for a function, lambda, or member function in the form
    //		bool Foo(Platform::Object^);. Should return true if the command can be executed,
    //		false if not.
	UICommand(
		WindowsStoreDirectXGame::ExecuteDelegate^ executeHandler,
		WindowsStoreDirectXGame::CanExecuteDelegate^ canExecuteHandler
		) : m_execute(executeHandler),
		m_canExecute(canExecuteHandler),
		m_canExecuteState(true)
	{ }

    // The CanExecuteChanged event is used by controls to which a UICommand instance is bound
    // to tell the control that there may have been a change in the CanExecute state of the
    // command. The control will check the CanExecute state and will enable or disable itself
    // accordingly. Use the RaiseCanExecuteChanged member function to raise this event.
	virtual event Windows::Foundation::EventHandler<Platform::Object^>^ CanExecuteChanged;

    // CanExecute is used by controls to which a UICommandInstance is bound to determine
    // whether or not they should be enabled. If no CanExecuteDelegate has been assigned to
    // this UICommand instance then the last value of m_canExecuteState will be returned. By
    // default m_canExecuteState is true.
	virtual bool CanExecute(Platform::Object^ parameter)
	{
		auto canExecute = m_canExecute;
		if (canExecute != nullptr)
		{
			m_canExecuteState = canExecute(parameter);
		}
		return m_canExecuteState;
	}

	virtual void Execute(Platform::Object^ parameter)
	{
		auto execute = m_execute;
		if (execute != nullptr)
		{
			execute(parameter);
            // Running the command might have caused the state of CanExecute to have changed.
            // So we raise the CanExecuteChanged event so the UI can update any bindings and
            // disable any controls that should no longer respond to user interaction.
			RaiseCanExecuteChanged();
		}
#if defined(_DEBUG)
		else
		{
			OutputDebugStringW(L"No execute handler found for event.\n");
		}
#endif
	}
    
    virtual void SetCanExecute(bool value)
    {
        m_canExecuteState = value;
        RaiseCanExecuteChanged();
    }

    virtual bool GetCanExecute(void)
    {
        return m_canExecuteState;
    }
    
	virtual void RaiseCanExecuteChanged()
	{
		CanExecuteChanged(this, nullptr);
	}

private:
	WindowsStoreDirectXGame::ExecuteDelegate^					m_execute;
	WindowsStoreDirectXGame::CanExecuteDelegate^				m_canExecute;
	bool														m_canExecuteState;
};

