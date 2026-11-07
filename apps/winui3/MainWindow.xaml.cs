using System;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.UI.Dispatching;
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
    private readonly Microsoft.UI.Dispatching.DispatcherQueueTimer _sipTimer;
    private bool _commandBusy;
    private bool _closing;
    private string _transientFeedback="";
    private DateTime _feedbackExpiresUtc=DateTime.MinValue;
    private readonly PreviewProfileStore _profileStore=PreviewProfileStore.ForCurrentUser();
    private readonly PreviewProfile _profile;

    public MainWindow()
    {
        InitializeComponent();
        MainShell.RequestedTheme=ElementTheme.Light;
        AutomationProperties.SetName(ThemeToggle,"Usar tema escuro");
        _profile=_profileStore.Load();
        ApplyWindowSize();
        RefreshMonitor();
        RefreshFavorites();
        ValidateDestination();
        _sipTimer=DispatcherQueue.CreateTimer();
        _sipTimer.Interval=TimeSpan.FromMilliseconds(500);
        _sipTimer.Tick+=(_,_)=>RefreshMonitor();
        _sipTimer.Start();
        Closed+=(_,_)=>{
            _closing=true;
            _sipTimer.Stop();
            // Stop the reactor outside DllMain, before process unload.
            _ = NativeBridge.Disconnect();
        };
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
                _ready && _snapshot.BackendAttached==1
                    ?"Destino válido"
                    :"Destino válido · motor SIP não vinculado",
            NativeBridge.DestinationStatus.Empty=>"Digite o ramal ou URI SIP",
            NativeBridge.DestinationStatus.Invalid=>"Destino SIP inválido",
            _=>"Validação nativa indisponível"
        };
        AddFavoriteButton.IsEnabled=validity==NativeBridge.DestinationStatus.Valid;
        UpdateCallControls();
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

    private NativeCallUiProjection CurrentCallUi()
    {
        var value=NativeBridge.Validate(Destination.Text??"");
        return NativeCallUiProjector.Project(
            _ready,_snapshot.BackendAttached,_snapshot.Connection,_snapshot.View,
            _snapshot.CanDial,_snapshot.CanAnswer,_snapshot.CanHangup,
            _snapshot.Session,_snapshot.CallId,
            value==NativeBridge.DestinationStatus.Valid);
    }

    private void UpdateCallControls()
    {
        var view=CurrentCallUi();
        bool receiving=view.CanAnswer;
        bool ending=view.CanHangup;
        CallButton.Visibility=receiving||ending?Visibility.Collapsed:Visibility.Visible;
        CallButton.IsEnabled=!_commandBusy && view.CanDial;
        CallButton.Content=_ready && _snapshot.BackendAttached==1
            ?"Ligar":"Ligar · indisponível";
        AnswerButton.Visibility=receiving?Visibility.Visible:Visibility.Collapsed;
        AnswerButton.IsEnabled=!_commandBusy && receiving;
        HangupButton.Visibility=ending?Visibility.Visible:Visibility.Collapsed;
        HangupButton.Content=receiving?"Recusar":"Encerrar";
        HangupButton.IsEnabled=!_commandBusy && ending;
        Grid.SetColumn(HangupButton,receiving?1:0);
        Grid.SetColumnSpan(HangupButton,receiving?1:2);
        ConnectButton.Content=view.CanDisconnect?"Desconectar":"Conectar";
        ConnectButton.IsEnabled=!_commandBusy &&
            (view.CanConnect||view.CanDisconnect);
    }

    private static string CommandFeedback(NativeBridge.CommandResult result) =>
        result switch {
            NativeBridge.CommandResult.Accepted=>
                "Comando enviado ao motor SIP; confirmação da central pendente",
            NativeBridge.CommandResult.MissingAccount=>
                "Configure a conta no AXE Voice Win32 e marque salvar senha. "+
                "Confira transporte UDP/TLS e certificado CA.",
            NativeBridge.CommandResult.Unavailable=>
                "Esta instalação é a prévia sem motor SIP nativo",
            NativeBridge.CommandResult.Invalid=>"Destino SIP inválido",
            _=>"Comando recusado: sessão mudou ou motor indisponível"
        };

    private void ShowActionFeedback(string message)
    {
        _transientFeedback=message;
        _feedbackExpiresUtc=DateTime.UtcNow.AddSeconds(10);
        BackendHint.Text=message;
    }

    private async void Connect_Click(object sender,RoutedEventArgs e)
    {
        if(_closing || _commandBusy)return;
        var state=CurrentCallUi();
        if(!state.CanConnect && !state.CanDisconnect)return;
        _commandBusy=true;
        UpdateCallControls();
        var result=await Task.Run(()=>
            state.CanDisconnect?NativeBridge.Disconnect():NativeBridge.ConnectSaved());
        if(_closing){
            // Connection completed after window close: ensure reactor is stopped.
            await Task.Run(NativeBridge.Disconnect);
            return;
        }
        _commandBusy=false;
        RefreshMonitor();
        ShowActionFeedback(result==NativeBridge.CommandResult.Accepted
            ?(state.CanDisconnect
                ?"Motor SIP desconectado"
                :"Registro SIP solicitado; aguardando confirmação da central")
            :CommandFeedback(result));
        UpdateCallControls();
    }

    private void Call_Click(object sender,RoutedEventArgs e)
    {
        if(_commandBusy || !CurrentCallUi().CanDial)return;
        var bound=_snapshot;
        var result=NativeBridge.Dial(Destination.Text,bound.Session);
        DestinationFeedback.Text=CommandFeedback(result);
        RefreshMonitor();
    }

    private void Answer_Click(object sender,RoutedEventArgs e)
    {
        if(_commandBusy || !CurrentCallUi().CanAnswer)return;
        var bound=_snapshot;
        var result=NativeBridge.Answer(bound.Session,bound.CallId);
        RefreshMonitor();
        ShowActionFeedback(CommandFeedback(result));
    }

    private void Hangup_Click(object sender,RoutedEventArgs e)
    {
        if(_commandBusy || !CurrentCallUi().CanHangup)return;
        var bound=_snapshot;
        var result=NativeBridge.Hangup(bound.Session,bound.CallId);
        RefreshMonitor();
        ShowActionFeedback(CommandFeedback(result));
    }

    private void RefreshMonitor()
    {
        _ready=NativeBridge.TryRead(out _snapshot);
        var state=CurrentCallUi();
        SipStatusText.Text=state.StatusLabel;
        BackendHint.Text=!_ready
            ?"Bridge nativa ausente ou incompatível"
            :_snapshot.BackendAttached==0
                ?"Prévia sem motor SIP: instale o candidato nativo"
                :_snapshot.Connection==2
                    ?"Registro confirmado pelo SDK; áudio exige homologação"
                    :_snapshot.Connection==1
                        ?"Aguardando confirmação de registro da central"
                        :"Motor SIP disponível; conexão manual necessária";
        if(DateTime.UtcNow<_feedbackExpiresUtc)
            BackendHint.Text=_transientFeedback;
        else _transientFeedback="";
        string report=SipDiagnosticReport.Format(
            _ready,_ready && _snapshot.BackendAttached!=0,
            _ready?_snapshot.AbiVersion:0,_snapshot.Connection,_snapshot.View);
        if(MonitorTools.Visibility==Visibility.Visible)
            DetailsDescription.Text=report;
        UpdateCallControls();
    }

    private void RefreshMonitor_Click(object sender,RoutedEventArgs e)=>RefreshMonitor();

    private void CopyDiagnostic_Click(object sender,RoutedEventArgs e)
    {
        // Copy only the fixed-schema report, never raw SIP logs or entered text.
        var data=new DataPackage();
        data.SetText(SipDiagnosticReport.Format(
            _ready,_ready && _snapshot.BackendAttached!=0,
            _ready?_snapshot.AbiVersion:0,_snapshot.Connection,_snapshot.View));
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
