# Candidate0.57.1 — parallel reliability pass

The user requested the pace of five developers. Run five independent audits of calls/DTMF, audio, UI, private diagnostics and packaging. Correct only demonstrated defects; preserve deferred Dígitro/headset/IVR acceptance. Calls and diagnostics audits found no actionable defects and ran focused tests including sanitizers.

Three defects need correction: thread construction failure leaves a ringtone request active without a worker; missing BS_NOTIFY prevents button-focus scrolling; reused packaging stage can include obsolete files and packaging does not verify the GUI/dependency manifest against actual inputs. Use minimal fixes with regression coverage and final Windows/SDK/installer gates.

The package manifest now records product version/hash after GUI smoke, validates declared SDK file hashes and rejects undeclared dependencies before staging. Recreate the owned staging folder, verify licenses recursively, reinstall the candidate and confirm packaged files disappear after uninstall. These hashes are integrity records, not publisher signatures or production approval. Physical acceptance remains pending.
