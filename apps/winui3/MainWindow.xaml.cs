using System;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Input;
using Windows.ApplicationModel.DataTransfer;
using Windows.Graphics;
using Windows.System;

namespace AXEVoice.WinUI3;

public sealed partial class MainWindow : Window
{
    private bool _dark;
    private bool _ready;
    private NativeBridge.Snapshot _snapshot;
    private readonly PreviewProfileStore _profileStore=PreviewProfileStore.ForCurrentUser();
    private readonly PreviewProfile _profile;

    public MainWindow()
    {
        InitializeComponent();
        MainShell.RequestedTheme=ElementTheme.Light;
        AutomationProperties.SetName(ThemeToggle,"Usar tema escuro");
        _profile=_profileStore.Load();
        ApplyWindowSize();
        try {
            AppWindow.SetIcon(System.IO.Path.Combine(AppContext.BaseDirectory,
                "Assets", "axe-voice.ico"));
        }
        catch (Exception) {
            // Keep the app usable when the preview is launched without assets.
        }
        RefreshMonitor();
        RefreshFavorites();
        ValidateDestination();
        // No native SIP command API exists for this edition yet.
        CallButton.IsEnabled=false;
    }

    private void ApplyWindowSize()
    {
        var size=PreviewWindowSizing.Dimensions(_profile.Size);
        try { AppWindow.Resize(new SizeInt32(size.Width,size.Height)); }
        catch (Exception) { /* Keep OS default if resizing is unavailable. */ }
        SizeModeButton.Content=_profile.Size==PreviewSizeMode.Compact
            ?"Tamanho: compacto (alterar)":"Tamanho: conforto (alterar)";
    }

    private void ValidateDestination()
    {
        var validity=NativeBridge.Validate(Destination.Text??"");
        DestinationFeedback.Text=validity switch {
            NativeBridge.DestinationStatus.Valid=>
                "Destino válido · chamadas desabilitadas nesta prévia",
            NativeBridge.DestinationStatus.Empty=>"Digite o ramal ou URI SIP",
            NativeBridge.DestinationStatus.Invalid=>"Destino SIP inválido",
            _=>"Validação nativa indisponível"
        };
        AddFavoriteButton.IsEnabled=validity==NativeBridge.DestinationStatus.Valid;
        CallButton.IsEnabled=false;
    }

    private void Destination_TextChanged(object sender,TextChangedEventArgs e)
        =>ValidateDestination();

    private void Digit_Click(object sender,RoutedEventArgs e)
    {
        if(sender is Button {Tag:string digit})
            Destination.Text=DialerEditing.AppendKey(Destination.Text,digit);
    }

    private void Backspace_Click(object sender,RoutedEventArgs e)
        =>Destination.Text=DialerEditing.ApplyKey(Destination.Text,8);

    private void Destination_KeyDown(object sender,KeyRoutedEventArgs e)
    {
        if(e.Key==VirtualKey.Escape){
            Destination.Text="";
            e.Handled=true;
        }
    }

    private void MainShell_KeyDown(object sender,KeyRoutedEventArgs e)
    {
        // Native TextBox processes its own text (including sip: or @).
        if(e.OriginalSource is TextBox)return;
        int key=(int)e.Key;
        if((key>=48 && key<=57)||(key>=96 && key<=105)||
           key==8 || key==27){
            Destination.Text=DialerEditing.ApplyKey(Destination.Text,key);
            e.Handled=true;
        }
        // Enter is never a dial shortcut without an authenticated SIP host.
    }

    private void AddFavorite_Click(object sender,RoutedEventArgs e)
    {
        if(NativeBridge.Validate(Destination.Text)!=NativeBridge.DestinationStatus.Valid)
            return;
        if(!_profile.AddFavorite(Destination.Text)){
            DestinationFeedback.Text="Favorito repetido ou limite de 10 atingido";
            return;
        }
        if(!_profileStore.Save(_profile))
            DestinationFeedback.Text="Favorito somente nesta sessão: não foi possível proteger arquivo";
        else DestinationFeedback.Text="Favorito salvo no perfil Windows";
        RefreshFavorites();
    }

    private void RefreshFavorites()
    {
        FavoriteList.ItemsSource=null;
        FavoriteList.ItemsSource=_profile.Favorites.ToArray();
        FavoriteCount.Text=$"{_profile.Favorites.Count}/10 destinos locais protegidos";
    }

    private void Favorite_ItemClick(object sender,ItemClickEventArgs e)
    {
        if(e.ClickedItem is not string value)return;
        Destination.Text=value;
        SwitchTo("","",true);
        // Choosing a favorite fills the number; it NEVER starts a call.
    }

    private void DeleteFavorite_Click(object sender,RoutedEventArgs e)
    {
        if(FavoriteList.SelectedItem is not string value)return;
        if(_profile.RemoveFavorite(value)){
            _profileStore.Save(_profile);
            RefreshFavorites();
        }
    }

    private void RefreshMonitor()
    {
        _ready=NativeBridge.TryRead(out _snapshot);
        SipStatusText.Text="● SIP INDISPONÍVEL";
        BackendHint.Text=_ready
            ? (_snapshot.BackendAttached==0
                ? "Bridge C++ conectada · motor SIP não vinculado"
                : "Bridge presente · comandos WinUI 3 ainda bloqueados")
            : "Bridge nativa ausente ou incompatível";
        string report=SipDiagnosticReport.Format(
            _ready,_ready && _snapshot.BackendAttached!=0,
            _ready?_snapshot.AbiVersion:0);
        if(MonitorTools.Visibility==Visibility.Visible)
            DetailsDescription.Text=report;
        CallButton.IsEnabled=false;
    }

    private void RefreshMonitor_Click(object sender,RoutedEventArgs e)=>RefreshMonitor();

    private void CopyDiagnostic_Click(object sender,RoutedEventArgs e)
    {
        // Copy only the fixed-schema report, never raw SIP logs or entered text.
        var data=new DataPackage();
        data.SetText(SipDiagnosticReport.Format(
            _ready,_ready && _snapshot.BackendAttached!=0,
            _ready?_snapshot.AbiVersion:0));
        Clipboard.SetContent(data);
        MonitorFeedback.Text="Diagnóstico técnico copiado (sem credenciais)";
    }

    private void ChangeSize_Click(object sender,RoutedEventArgs e)
    {
        _profile.Size=_profile.Size==PreviewSizeMode.Compact
            ?PreviewSizeMode.Comfortable:PreviewSizeMode.Compact;
        ApplyWindowSize();
        _profileStore.Save(_profile);
    }

    private void Theme_Click(object sender,RoutedEventArgs e)
    {
        _dark=!_dark;
        MainShell.RequestedTheme=_dark?ElementTheme.Dark:ElementTheme.Light;
        ThemeToggle.Content=_dark?"☀":"☾";
        AutomationProperties.SetName(ThemeToggle,
            _dark?"Usar tema claro":"Usar tema escuro");
    }

    private void SwitchTo(string title,string explanation,bool dialer,string page="")
    {
        DialerPanel.Visibility=dialer?Visibility.Visible:Visibility.Collapsed;
        DetailsPanel.Visibility=dialer?Visibility.Collapsed:Visibility.Visible;
        FavoritesTools.Visibility=page=="favorites"?Visibility.Visible:Visibility.Collapsed;
        MonitorTools.Visibility=page=="monitor"?Visibility.Visible:Visibility.Collapsed;
        SettingsTools.Visibility=page=="settings"?Visibility.Visible:Visibility.Collapsed;
        DetailsTitle.Text=title;
        DetailsDescription.Text=explanation;
        if(page=="favorites")RefreshFavorites();
        if(page=="monitor")RefreshMonitor();
    }

    private void Dialer_Click(object sender,RoutedEventArgs e)=>
        SwitchTo("","",true);

    private void Contacts_Click(object sender,RoutedEventArgs e)=>
        SwitchTo("Favoritos",
            "Toque em um favorito para preencher o destino. Chamadas não são iniciadas.",
            false,"favorites");

    private void Monitor_Click(object sender,RoutedEventArgs e)=>
        SwitchTo("Monitor SIP","",false,"monitor");

    private void Settings_Click(object sender,RoutedEventArgs e)=>
        SwitchTo("Preferências",
            "Mude o tamanho da janela. Contas e codecs SIP continuam disponíveis na edição Win32.",
            false,"settings");
}
