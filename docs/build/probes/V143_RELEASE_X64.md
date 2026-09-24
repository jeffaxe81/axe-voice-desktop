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
      main_console.c
      encryption.c
      activesock.c
      dlg_core_test.c
      systest.c
      http_client.c
      atomic.c
      dns_test.c
      echo_clt.c
      json_test.c
      main.c
      inv_offer_answer_test.c
      Generating Code...
      resolver_test.c
      errno.c
      main.c
      msg_err_test.c
      stun.c
      test.c
      exception.c
      msg_logger.c
      xml.c
      fifobuf.c
      msg_test.c
      Generating Code...
      file.c
      multipart_test.c
      hash_test.c
      ioq_perf.c
      pjsystest.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjsip-apps\bin\pjsystest-x86_64-x64-vc14-Release.exe
      regc_test.c
      ioq_stress_test.c
      pjlib_util_test.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjlib-util\bin\pjlib-util-test-x86_64-x64-vc14-Release.exe
      test.c
      ioq_tcp.c
      transport_loop_test.c
      ioq_udp.c
      transport_tcp_test.c
      ioq_unreg.c
      list.c
      transport_test.c
      main.c
      transport_udp_test.c
      mutex.c
      tsx_basic_test.c
      os.c
      pool.c
      pool_perf.c
      tsx_bench.c
      codec_vectors.c
      rand.c
      Generating Code...
      tsx_uac_test.c
      jbuf_test.c
      tsx_uas_test.c
      main.c
      txdata_test.c
      Compiling...
      rbtree.c
      mips_test.c
      select.c
      uri_test.c
      rtp_test.c
      sleep.c
      Generating Code...
      sdp_neg_test.c
      sock.c
      test.c
      sock_perf.c
      ssl_sock.c
      vid_codec_test.c
      string.c
      test.c
      vid_dev_test.c
      concur_test.c
      thread.c
      vid_port_test.c
      ice_test.c
      timer.c
      main.c
      Generating Code...
      timestamp.c
      udp_echo_srv_ioqueue.c
      server.c
      udp_echo_srv_sync.c
      unittest_test.c
      sess_auth.c
      pjsip_test.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjsip\bin\pjsip-test-x86_64-x64-vc14-Release.exe
      util.c
      stun.c
      Generating Code...
      stun_sock_test.c
      test.c
      turn_sock_test.c
      Generating Code...
      pjlib_test.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjlib\bin\pjlib-test-x86_64-x64-vc14-Release.exe
      pjnath_test.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjnath\bin\pjnath-test-x86_64-x64-vc14-Release.exe
      pjmedia_test.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjmedia\bin\pjmedia-test-x86_64-x64-vc14-Release.exe
      debug.cpp
      main.c
      pjsua_app.c
      pjsua_app_cli.c
      sample_debug.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjsip-apps\bin\sample-debug-x86_64-x64-vc14-Release.exe
      pjsua_app_common.c
      pjsua_app_config.c
      pjsua_app_legacy.c
      Generating Code...
      pjsua.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjsip-apps\bin\pjsua-x86_64-x64-vc14-Release.exe

## MicroSIP log tail

    MSBuild version 17.14.51+25f168cee for .NET Framework
    
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
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\Data\Database.h(21,10): error C1083: Cannot open include file: 'SQLiteCpp/SQLiteCpp.h': No such file or directory [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/Calls.cpp')
      
      CallsRepository.cpp
      ContactsRepository.cpp
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\Data\CallsRepository.h(21,10): error C1083: Cannot open include file: 'SQLiteCpp/SQLiteCpp.h': No such file or directory [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/Data/CallsRepository.cpp')
      
      Database.cpp
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\Data\Database.h(21,10): error C1083: Cannot open include file: 'SQLiteCpp/SQLiteCpp.h': No such file or directory [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/Data/Database.cpp')
      
      Dialer.cpp
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\Data\ContactsRepository.h(21,10): error C1083: Cannot open include file: 'SQLiteCpp/SQLiteCpp.h': No such file or directory [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/Data/ContactsRepository.cpp')
      
      ExportDlg.cpp
      FeatureCodesDlg.cpp
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\lib\MessageBoxX.h(38,9): warning C4005: '_NODISCARD': macro redefinition [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/Contacts.cpp')
          C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\14.44.35207\include\vcruntime.h(328,13):
          see previous definition of '_NODISCARD'
      
    D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\Data\Database.h(21,10): error C1083: Cannot open include file: 'SQLiteCpp/SQLiteCpp.h': No such file or directory [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\MicroSIP-3.22.16-src\microsip.vcxproj]
      (compiling source file '/Contacts.cpp')
      
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
