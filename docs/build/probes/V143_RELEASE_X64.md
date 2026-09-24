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
- MicroSIP exit code: not-run
- MicroSIP.exe found: no

## PJSIP log tail

      msg_logger.c
      Generating Code...
      jbuf_test.c
      file.c
      msg_test.c
      aectest.c
      hash_test.c
      ioq_perf.c
      main.c
      multipart_test.c
      ioq_stress_test.c
      mips_test.c
      regc_test.c
      ioq_tcp.c
      aviplay.c
      rtp_test.c
      test.c
      ioq_udp.c
      pjlib_util_test.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjlib-util\bin\pjlib-util-test-x86_64-x64-vc14-Release.exe
      sdp_neg_test.c
      transport_loop_test.c
      ioq_unreg.c
      test.c
      transport_tcp_test.c
      list.c
      clidemo.c
      vid_codec_test.c
      main.c
      transport_test.c
      vid_dev_test.c
      mutex.c
      transport_udp_test.c
      vid_port_test.c
      os.c
      pool.c
      pool_perf.c
      tsx_basic_test.c
      confsample.c
      Generating Code...
      rand.c
      Generating Code...
      tsx_bench.c
      tsx_uac_test.c
      confbench.c
      Compiling...
      rbtree.c
      tsx_uas_test.c
      pjmedia_test.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjmedia\bin\pjmedia-test-x86_64-x64-vc14-Release.exe
      select.c
      txdata_test.c
      encdec.c
      sleep.c
      uri_test.c
      sock.c
      httpdemo.c
      Generating Code...
      sock_perf.c
      ssl_sock.c
      icedemo.c
      string.c
      test.c
      thread.c
      jbsim.c
      timer.c
      pjsip_test.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjsip\bin\pjsip-test-x86_64-x64-vc14-Release.exe
      timestamp.c
      udp_echo_srv_ioqueue.c
      latency.c
      udp_echo_srv_sync.c
      unittest_test.c
      level.c
      util.c
      Generating Code...
      mix.c
      pcaputil.c
    ..\src\samples\pcaputil.c(424): warning C4018: '<=': signed/unsigned mismatch [D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjsip-apps\build\samples.vcxproj]
      pjsip-perf.c
      playfile.c
      pjlib_test.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjlib\bin\pjlib-test-x86_64-x64-vc14-Release.exe
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
      main_console.c
      concur_test.c
      systest.c
      ice_test.c
      main.c
      Generating Code...
      debug.cpp
      server.c
      sess_auth.c
      stun.c
      stun_sock_test.c
      test.c
      pjsystest.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjsip-apps\bin\pjsystest-x86_64-x64-vc14-Release.exe
      turn_sock_test.c
      Generating Code...
      sample_debug.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjsip-apps\bin\sample-debug-x86_64-x64-vc14-Release.exe
      pjnath_test.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjnath\bin\pjnath-test-x86_64-x64-vc14-Release.exe
      main.c
      pjsua_app.c
      pjsua_app_cli.c
      pjsua_app_common.c
      pjsua_app_config.c
      pjsua_app_legacy.c
      Generating Code...
      pjsua.vcxproj -> D:\a\axe-voice-desktop\axe-voice-desktop\.probe\pjproject\pjsip-apps\bin\pjsua-x86_64-x64-vc14-Release.exe

## MicroSIP log tail

    Log not created.
