using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.Graphics;

namespace AXEVoice.WinUI3;

public sealed partial class MainWindow : Window
{
    private bool _dark;
    private bool _ready;

    public MainWindow()
    {
        InitializeComponent();
        try { AppWindow.Resize(new SizeInt32(355, 602)); }
        catch (Exception) { /* Keep system default when AppWindow resize unavailable. */ }

        // This is a true read-only contract with the native C++ projection,
        // not a second SIP engine. No registration/dialing is simulated.
        _ready = NativeBridge.TryRead(out var snapshot);
        SipStatusText.Text = "● SIP INDISPONÍVEL";
        BackendHint.Text = _ready
            ? (snapshot.BackendAttached == 0
                ? "Bridge C++ conectada · motor SIP não vinculado"
                : "Backend presente; chamadas WinUI 3 ainda desabilitadas")
            : "Bridge nativa não encontrada ou versão incompatível";
        CallButton.IsEnabled = false; // no command ABI in stage 1
    }

    private void Digit_Click(object sender, RoutedEventArgs e)
    {
        if (sender is Button { Tag: string digit } && Destination.Text.Length < 128)
            Destination.Text += digit;
    }

    private void Backspace_Click(object sender, RoutedEventArgs e)
    {
        string value = Destination.Text;
        if (value.Length > 0) Destination.Text = value[..^1];
    }

    private void Theme_Click(object sender, RoutedEventArgs e)
    {
        _dark = !_dark;
        MainShell.RequestedTheme = _dark ? ElementTheme.Dark : ElementTheme.Light;
    }

    private void SwitchTo(string title, string explanation, bool dialer)
    {
        DialerPanel.Visibility = dialer ? Visibility.Visible : Visibility.Collapsed;
        DetailsPanel.Visibility = dialer ? Visibility.Collapsed : Visibility.Visible;
        DetailsTitle.Text = title;
        DetailsDescription.Text = explanation;
    }

    private void Dialer_Click(object sender, RoutedEventArgs e) =>
        SwitchTo("", "", true);

    private void Contacts_Click(object sender, RoutedEventArgs e) =>
        SwitchTo("Contatos", "Agenda do AXE Voice Win32 ainda não conectada à edição WinUI 3.", false);

    private void Monitor_Click(object sender, RoutedEventArgs e) =>
        SwitchTo("Monitor SIP", "Nenhum registro SIP desta edição foi iniciado. A ligação do motor Baresip é o próximo estágio.", false);

    private void Settings_Click(object sender, RoutedEventArgs e) =>
        SwitchTo("Preferências", "As configurações SIP e de codecs serão compartilhadas somente após a integração segura do motor nativo.", false);
}
