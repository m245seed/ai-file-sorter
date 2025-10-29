using AiFileSorter.Models;
using Avalonia.Controls;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace AiFileSorter.Dialogs;

public class LLMSelectionDialog
{
    public static async Task<LLMChoice?> ShowAsync(Window parentWindow)
    {
        var dialog = new Window
        {
            Title = "Select LLM Type",
            Width = 400,
            Height = 300,
            WindowStartupLocation = WindowStartupLocation.CenterOwner
        };

        var stack = new StackPanel { Margin = new Avalonia.Thickness(20) };

        var title = new TextBlock
        {
            Text = "Choose which LLM to use for file categorization:",
            FontSize = 14,
            Margin = new Avalonia.Thickness(0, 0, 0, 20),
            TextWrapping = Avalonia.Media.TextWrapping.Wrap
        };

        var options = new ListBox
        {
            ItemsSource = new List<string>
            {
                "Local LLM (Rule-based, no internet required)",
                "Remote LLM (ChatGPT, requires API key)",
                "Local LLM - 3B Model",
                "Local LLM - 7B Model"
            },
            SelectedIndex = 0,
            Margin = new Avalonia.Thickness(0, 0, 0, 20)
        };

        var buttonStack = new StackPanel
        {
            Orientation = Avalonia.Layout.Orientation.Horizontal,
            HorizontalAlignment = Avalonia.Layout.HorizontalAlignment.Right
        };

        LLMChoice? result = null;

        var okButton = new Button
        {
            Content = "OK",
            Width = 80,
            Margin = new Avalonia.Thickness(0, 0, 10, 0)
        };

        okButton.Click += (s, e) =>
        {
            result = options.SelectedIndex switch
            {
                0 => LLMChoice.Unset,
                1 => LLMChoice.Remote,
                2 => LLMChoice.Local_3b,
                3 => LLMChoice.Local_7b,
                _ => LLMChoice.Unset
            };
            dialog.Close();
        };

        var cancelButton = new Button
        {
            Content = "Cancel",
            Width = 80
        };

        cancelButton.Click += (s, e) => dialog.Close();

        buttonStack.Children.Add(okButton);
        buttonStack.Children.Add(cancelButton);

        stack.Children.Add(title);
        stack.Children.Add(options);
        stack.Children.Add(buttonStack);

        dialog.Content = stack;

        await dialog.ShowDialog(parentWindow);

        return result;
    }
}
