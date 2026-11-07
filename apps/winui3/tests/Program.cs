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
        Console.WriteLine("WinUI 3 preview: keyboard, favorites, DPAPI, monitor and sizing checks OK");
    }
}
