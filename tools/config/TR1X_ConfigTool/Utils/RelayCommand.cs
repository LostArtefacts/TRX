using System;
using System.Windows;
using System.Windows.Input;

namespace TR1X_ConfigTool.Utils;

public class RelayCommand : ICommand
{
    public RelayCommand(Action execute, Func<bool> canExecute)
    {
        _execute = execute;
        _canExecute = canExecute;
    }

    public RelayCommand(Action execute)
    {
        _execute = execute;
        _canExecute = null;
    }

    public event EventHandler CanExecuteChanged
    {
        add
        {
            CommandManager.RequerySuggested += value;
            _canExecuteChanged += value;
        }
        remove
        {
            CommandManager.RequerySuggested -= value;
            _canExecuteChanged -= value;
        }
    }

    public bool CanExecute(object parameter)
    {
        return _canExecute == null || _canExecute();
    }

    public void Execute(object parameter)
    {
        _execute();
    }

    public void RaiseCanExecuteChanged()
    {
        _canExecuteChanged.Invoke(this, EventArgs.Empty);
    }

    private readonly Func<bool> _canExecute;
    private readonly Action _execute;

    private EventHandler _canExecuteChanged;
}

public class RelayCommand<T> : ICommand
{
    public RelayCommand(Action<T> execute, Func<T, bool> canExecute)
    {
        _execute = execute;
        _canExecute = canExecute;
    }

    public RelayCommand(Action<T> execute)
    {
        _execute = execute;
        _canExecute = null;
    }

    public event EventHandler CanExecuteChanged
    {
        add
        {
            CommandManager.RequerySuggested += value;
            _canExecuteChanged += value;
        }
        remove
        {
            CommandManager.RequerySuggested -= value;
            _canExecuteChanged -= value;
        }
    }

    public bool CanExecute(object parameter)
    {
        return _canExecute == null || _canExecute((T)parameter);
    }

    public void Execute(object parameter)
    {
        _execute((T)parameter);
    }

    public void RaiseCanExecuteChanged()
    {
        _canExecuteChanged.Invoke(this, EventArgs.Empty);
    }

    private readonly Func<T, bool> _canExecute;
    private readonly Action<T> _execute;

    private EventHandler _canExecuteChanged;
}

public class RelayKeyBinding : KeyBinding
{
    public static readonly DependencyProperty CommandBindingProperty = DependencyProperty.Register
    (
        nameof(CommandBinding),
        typeof(ICommand),
        typeof(RelayKeyBinding),
        new FrameworkPropertyMetadata(OnCommandBindingChanged)
    );

    public ICommand CommandBinding
    {
        get { return (ICommand)GetValue(CommandBindingProperty); }
        set { SetValue(CommandBindingProperty, value); }
    }

    private static void OnCommandBindingChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        RelayKeyBinding keyBinding = (RelayKeyBinding)d;
        keyBinding.Command = (ICommand)e.NewValue;
    }
}
