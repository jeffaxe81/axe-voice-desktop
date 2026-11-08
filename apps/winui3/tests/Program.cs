using System;
using System.IO;
using System.Text;
using AXEVoice.WinUI3;

static class Tests
{
    static void Check(bool ok, string explanation)
    {
        if(!ok) throw new Exception(explanation);
    }
    static void Main()
    {
        // Front 2: physical keyboard + input safety.
        Check(DialerEditing.AppendKey("10","2") == "102","append digit");
        Check(DialerEditing.AppendKey("10","X") == "10","reject non-dialpad button");
        Check(DialerEditing.ApplyKey("23", 8) == "2","backspace");
        Check(DialerEditing.ApplyKey("23", 27) == "","escape clears");
        Check(DialerEditing.ApplyKey("23", 105) == "239","numpad 9");
        Check(DialerEditing.ApplyKey("23", 49) == "231","digit 1");
        Check(DialerEditing.AppendKey(new string('9',128),"8").Length==128,"max length");
        Check(DialerEditing.ApplyKey("23", 13) == "23","enter never dials");

        // Front 3: favorites require explicit request, deduplicate, bound list.
        var profile=new PreviewProfile();
        Check(profile.AddFavorite("1001"),"first favorite");
        Check(!profile.AddFavorite("1001"),"duplicate favorite rejected");
        Check(!profile.AddFavorite(" "),"empty favorite rejected");
        for(int i=2;i<=10;++i) Check(profile.AddFavorite((1000+i).ToString()),"append favorite");
        Check(profile.Favorites.Count==10,"max 10 favorites");
        Check(!profile.AddFavorite("9999"),"11th favorite rejected");
        Check(profile.RemoveFavorite("1001") && !profile.RemoveFavorite("1001"),"remove once");

        // Current-user Windows DPAPI protects local sensitive destinations.
        string sandbox=Path.Combine(Path.GetTempPath(),"axv-winui-features-"+Guid.NewGuid().ToString("N"));
        try {
            var local=new PreviewProfileStore(Path.Combine(sandbox,"favorites.bin"));
            profile.Size=PreviewSizeMode.Comfortable;
            Check(local.Save(profile),"DPAPI save succeeded");
            byte[] content=File.ReadAllBytes(local.StoragePath);
            Check(!Encoding.UTF8.GetString(content).Contains("1002",StringComparison.Ordinal),
                  "favorite value visible in storage");
            var loaded=local.Load();
            Check(loaded.Favorites.Count==9 &&
                  loaded.Size==PreviewSizeMode.Comfortable,"DPAPI roundtrip");
            File.WriteAllBytes(local.StoragePath,new byte[]{1,2,3,4});
            Check(local.Load().Favorites.Count==0,"corrupt profile fail closed");
            // Validate post-decryption schema again: a syntactically valid,
            // encrypted profile may still contain duplicate, too-long or
            // control-character values, or an unsupported persisted size.
            var dirty=new PreviewProfile {Size=(PreviewSizeMode)99};
            dirty.Favorites.Add("1001");
            dirty.Favorites.Add("1001");
            dirty.Favorites.Add("1002");
            dirty.Favorites.Add("1002");
            dirty.Favorites.Add("bad\nnumber");
            dirty.Favorites.Add(new string('9',129));
            dirty.Favorites.Add("");
            for(int n=3;n<=18;++n)dirty.Favorites.Add((1000+n).ToString());
            Check(local.Save(dirty),"encrypted invalid fixture saved");
            var clean=local.Load();
            Check(clean.Size==PreviewSizeMode.Compact,"invalid encrypted saved mode rejected");
            Check(clean.Favorites.Count==10,"encrypted favorite list revalidated and capped");
            Check(clean.Favorites[0]=="1001" && clean.Favorites[1]=="1002","unique destinations preserved");
            Check(!clean.Favorites.Contains("bad\nnumber") &&
                  !clean.Favorites.Contains(new string('9',129)),"unsafe destinations discarded");
            // Bit flips to DPAPI ciphertext must revoke the whole profile,
            // not partially show favorite numbers or leak cleartext.
            byte[] encrypted=File.ReadAllBytes(local.StoragePath);
            encrypted[encrypted.Length/2]^=0x5a;
            File.WriteAllBytes(local.StoragePath,encrypted);
            Check(local.Load().Favorites.Count==0,"tampered DPAPI profile rejected");
            File.Delete(local.StoragePath);
            Check(local.Load().Favorites.Count==0,"missing profile starts empty");
        }
        finally { try { Directory.Delete(sandbox,true); } catch {} }

        // Front 4: diagnostic text is a fixed schema; no SIP credentials or contacts.
        var report=SipDiagnosticReport.Format(
            bridgeAvailable:true,backendAttached:false,abiVersion:1);
        Check(report.Contains("SIP indisponível",StringComparison.OrdinalIgnoreCase),
              "offline reporting");
        Check(!report.Contains("registered",StringComparison.OrdinalIgnoreCase),
              "no false connection");
        Check(!report.Contains("1001",StringComparison.Ordinal),"no contact number in report");
        Check(!SipDiagnosticReport.Format(false,false,0).Contains("Conectado",StringComparison.Ordinal),
              "missing bridge never connected");

        // Front 5: bounded modes, no hidden or oversized viewport.
        var compact=PreviewWindowSizing.Dimensions(PreviewSizeMode.Compact);
        var comfortable=PreviewWindowSizing.Dimensions(PreviewSizeMode.Comfortable);
        Check(compact.Width==355 && compact.Height==602,"compact original dimensions");
        Check(comfortable.Width==400 && comfortable.Height==680,"comfortable dimensions");
        Check(PreviewWindowSizing.Dimensions((PreviewSizeMode)99)==compact,
              "invalid saved mode defaults compact");
        // Stage 2B: UI must not expose SIP commands until the native SDK has
        // proved registration AND the opaque call/session tokens are present.
        var offline=NativeCallUiProjector.Project(false,0,0,0,1,1,1,0,0,true);
        Check(!offline.CanDial && !offline.CanAnswer &&
              !offline.CanConnect,"missing bridge fail closed");
        var preview=NativeCallUiProjector.Project(true,0,2,1,1,1,1,4,99,true);
        Check(!preview.CanDial && !preview.CanAnswer &&
              !preview.CanHangup && !preview.CanConnect,"preview cannot call");
        var ready=NativeCallUiProjector.Project(true,1,2,0,1,0,0,4,0,true);
        Check(ready.CanDial && !ready.CanAnswer &&
              !ready.CanHangup,"verified registration allows dial");
        Check(!NativeCallUiProjector.Project(true,1,2,0,1,0,0,0,0,true).CanDial,
              "zero session denies dial");
        Check(!NativeCallUiProjector.Project(true,1,2,0,1,0,0,4,0,false).CanDial,
              "invalid destination denies dial");
        var ringing=NativeCallUiProjector.Project(true,1,2,1,0,1,1,4,99,false);
        Check(ringing.CanAnswer && ringing.CanHangup &&
              !ringing.CanDial,"incoming call actions");
        var incomplete=NativeCallUiProjector.Project(true,1,2,1,0,1,1,4,0,false);
        Check(!incomplete.CanAnswer && !incomplete.CanHangup,
              "missing call id denies answer and hangup");
        var calling=NativeCallUiProjector.Project(true,1,2,2,0,0,1,4,99,false);
        Check(!calling.CanAnswer && calling.CanHangup,"outgoing hangup only");
        var pending=NativeCallUiProjector.Project(true,1,1,0,0,0,0,0,0,true);
        Check(!pending.CanDial && !pending.CanAnswer &&
              !pending.CanConnect && pending.CanDisconnect,"pending cannot dial");
        Check(SipDiagnosticReport.Format(true,true,2,2,1).Contains("recebida",
              StringComparison.OrdinalIgnoreCase),"incoming status in monitor");

        Console.WriteLine("WinUI 3 preview: keyboard, favorites, DPAPI, monitor and sizing checks OK");
    }
}
