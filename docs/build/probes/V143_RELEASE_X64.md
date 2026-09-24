# Compatibility Probe — v143 / Release x64

## Rule

No MicroSIP or PJSIP source file was edited. Toolset v143 was supplied only through MSBuild properties.

## Result

- MicroSIP baseline: 3.22.16.0
- PJSIP candidate: 2.15.1
- Toolset override: v143
- Configuration: Release
- Platform: x64
- PJSIP exit code: 0
- MicroSIP exit code: 1
- MicroSIP.exe found: no

## PJSIP log tail

      exception.c
      main.c
      msg_logger.c
      xml.c
      auddemo.c
      fifobuf.c
      mips_test.c
      msg_test.c
      Generating Code...
      file.c
      rtp_test.c
      multipart_test.c
      hash_test.c
      ioq_perf.c
      sdp_neg_test.c
      regc_test.c
      ioq_stress_test.c
      aectest.c
      test.c
      ioq_tcp.c
      test.c
      vid_codec_test.c
      transport_loop_test.c
      ioq_udp.c
      pjlib_util_test.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjlib-util\bin\pjlib-util-test-x86_64-x64-vc14-Release.exe
      vid_dev_test.c
      transport_tcp_test.c
      ioq_unreg.c
      pjsystest.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjsip-apps\bin\pjsystest-x86_64-x64-vc14-Release.exe
      vid_port_test.c
      transport_test.c
      list.c
      main.c
      Generating Code...
      transport_udp_test.c
      tsx_basic_test.c
      mutex.c
      aviplay.c
      os.c
      tsx_bench.c
      pool.c
      pool_perf.c
      tsx_uac_test.c
      rand.c
      Generating Code...
      pjmedia_test.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjmedia\bin\pjmedia-test-x86_64-x64-vc14-Release.exe
      clidemo.c
      tsx_uas_test.c
      confsample.c
      txdata_test.c
      Compiling...
      rbtree.c
      uri_test.c
      select.c
      Generating Code...
      confbench.c
      sleep.c
      sock.c
      sock_perf.c
      encdec.c
      debug.cpp
      ssl_sock.c
      httpdemo.c
      string.c
      test.c
      pjsip_test.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjsip\bin\pjsip-test-x86_64-x64-vc14-Release.exe
      thread.c
      icedemo.c
      timer.c
      timestamp.c
      udp_echo_srv_ioqueue.c
      jbsim.c
      udp_echo_srv_sync.c
      unittest_test.c
      util.c
      latency.c
      Generating Code...
      level.c
      mix.c
      sample_debug.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjsip-apps\bin\sample-debug-x86_64-x64-vc14-Release.exe
      pcaputil.c
    ..\src\samples\pcaputil.c(424): warning C4018: '<=': signed/unsigned mismatch [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjsip-apps\build\samples.vcxproj]
      pjsip-perf.c
      pjlib_test.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjlib\bin\pjlib-test-x86_64-x64-vc14-Release.exe
      playfile.c
      playsine.c
      recfile.c
      resampleplay.c
      simpleua.c
      simple_pjsua.c
      sipecho.c
      siprtp.c
    ..\src\samples\siprtp.c(703): warning C4102: 'TODO___HANDLE_FORKING': unreferenced label [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjsip-apps\build\samples.vcxproj]
      sipstateless.c
      stateful_proxy.c
      stateless_proxy.c
      stereotest.c
      streamutil.c
      strerror.c
      tonegen.c
      vid_streamutil.c
      concur_test.c
      main.c
      ice_test.c
      pjsua_app.c
      main.c
      pjsua_app_cli.c
      server.c
      pjsua_app_common.c
      sess_auth.c
      stun.c
      pjsua_app_config.c
      stun_sock_test.c
      pjsua_app_legacy.c
      test.c
      Generating Code...
      turn_sock_test.c
      Generating Code...
      pjnath_test.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjnath\bin\pjnath-test-x86_64-x64-vc14-Release.exe
      pjsua.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjsip-apps\bin\pjsua-x86_64-x64-vc14-Release.exe

## MicroSIP log tail

    MSBuild version 17.14.60+43b635718 for .NET Framework
    
      stdafx.cpp
      AAOptionsDlg.cpp
      AccountDlg.cpp
      AddDlg.cpp
      addons.cpp
      BaseDialog.cpp
      ButtonBottom.cpp
      ButtonDialer.cpp
      ButtonEx.cpp
      ButtonSafe.cpp
      Calls.cpp
      CListCtrl_Sortable.cpp
      CListCtrl_SortItemsEx.cpp
      ClosableTabCtrl.cpp
      Contacts.cpp
      CallsRepository.cpp
      ContactsRepository.cpp
      Database.cpp
      Dialer.cpp
      ExportDlg.cpp
      FeatureCodesDlg.cpp
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\lib\MessageBoxX.h(38,9): warning C4005: '_NODISCARD': macro redefinition [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/Contacts.cpp')
          C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\14.44.35207\include\vcruntime.h(328,13):
          see previous definition of '_NODISCARD'
      
      global.cpp
      IconButton.cpp
      ImportDlg.cpp
      jumplist.cpp
      CListCtrl_ToolTip.cpp
      CMask.cpp
      Crypto.cpp
      CSVFile.cpp
      Hid.cpp
      langpack.cpp
      LevelsSliderCtrl.cpp
      Markup.cpp
      MessageBoxX.cpp
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\lib\MessageBoxX.h(38,9): warning C4005: '_NODISCARD': macro redefinition [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/lib/MessageBoxX.cpp')
          C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\14.44.35207\include\vcruntime.h(328,13):
          see previous definition of '_NODISCARD'
      
      ModelessMessageBox.cpp
      MSIP.cpp
      StdioFileEx.cpp
      VisualStylesXP.cpp
      mainDlg.cpp
      MessagesDlg.cpp
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\lib\Hid.cpp(39,9): warning C4005: 'HID_USAGE_TELEPHONY_HOOK_SWITCH': macro redefinition [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/lib/Hid.cpp')
          C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared\hidusage.h(2858,9):
          see previous definition of 'HID_USAGE_TELEPHONY_HOOK_SWITCH'
      
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\lib\Hid.cpp(41,9): warning C4005: 'HID_USAGE_TELEPHONY_REDIAL': macro redefinition [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/lib/Hid.cpp')
          C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared\hidusage.h(2862,9):
          see previous definition of 'HID_USAGE_TELEPHONY_REDIAL'
      
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\lib\Hid.cpp(42,9): warning C4005: 'HID_USAGE_TELEPHONY_FLASH': macro redefinition [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/lib/Hid.cpp')
          C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared\hidusage.h(2859,9):
          see previous definition of 'HID_USAGE_TELEPHONY_FLASH'
      
      microsip.cpp
      Preview.cpp
      RinginDlg.cpp
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.cpp(324,35): warning C4311: 'type cast': pointer truncation from 'Dialer *' to 'UINT' [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/microsip.cpp')
      
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.cpp(325,34): warning C4311: 'type cast': pointer truncation from 'Calls *' to 'UINT' [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/microsip.cpp')
      
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.cpp(326,37): warning C4311: 'type cast': pointer truncation from 'Contacts *' to 'UINT' [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/microsip.cpp')
      
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.cpp(327,32): warning C4311: 'type cast': pointer truncation from 'CTabCtrl *' to 'UINT' [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/microsip.cpp')
      
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.cpp(328,38): warning C4311: 'type cast': pointer truncation from 'StatusBar *' to 'UINT' [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/microsip.cpp')
      
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.cpp(329,36): warning C4311: 'type cast': pointer truncation from 'HWND' to 'UINT' [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/microsip.cpp')
      
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.cpp(330,74): warning C4311: 'type cast': pointer truncation from 'HWND' to 'UINT' [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/microsip.cpp')
      
      CallsService.cpp
      ContactsService.cpp
      settings.cpp
      SettingsDlg.cpp
      ShortcutsDlg.cpp
      StatusBar.cpp
      ThemeDialog.cpp
      Transfer.cpp
      json_reader.cpp
      json_value.cpp
      json_writer.cpp
    LINK : fatal error LNK1181: cannot open input file 'bcg729.lib' [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
