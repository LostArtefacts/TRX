using System;
using System.Windows.Input;

namespace Installer.Utils;

public class RelayCommand : ICommand
{
    public RelayCommand(Action execute, Func<bool>? canExecute)
    {
        _execute = execute;
        _canExecute = canExecute;
    }

    public RelayCommand(Action execute)
    {
        _execute = execute;
        _canExecute = null;
    }

    public event EventHandler? CanExecuteChanged
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

    public bool CanExecute(object? parameter)
    {
        return _canExecute == null || _canExecute();
    }

    public void Execute(object? parameter)
    {
        _execute();
    }

    public void RaiseCanExecuteChanged()
    {
        _canExecuteChanged?.Invoke(this, EventArgs.Empty);
    }

    private readonly Func<bool>? _canExecute;
    private readonly Action _execute;

    private event EventHandler? _canExecuteChanged;
}
