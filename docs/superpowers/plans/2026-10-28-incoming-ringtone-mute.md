# Incoming ringtone mute Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans.

**Goal:** Offer reversible app-local incoming ringtone mute without affecting calls.
**Architecture:** Defaulted fourth policy argument suppresses sound requests; existing cooperative player cancels. GUI-owned RAM bool, button/shortcut and compact-visible duration badge expose preference.
**Tech Stack:** C++17, Win32, CMake/CTest.
**Spec:** docs/superpowers/specs/2026-10-28-incoming-ringtone-mute-design.md

## Global Constraints
- Candidate0.67.0; defaultnormal on startup, no persistence or SIP rejection.
- request(call,endpoint,preview=false,sound_muted=false) keeps old callers unchanged.
- No changes to Answer/Hangup guards, call-specific acknowledgment, microphone/media, manual preview or exports.

## Review Focus
- Unmuting the same still-ringing call must resume unless specifically acknowledged.
- Rapid mute/unmute must not overlap renderers or revive obsolete call identity.
- Cooperative cancellation may lag; UI describes a sound setting, not guaranteed immediate silence.
- Compact mode must show active mute, with an accessible shortcut to toggle.
- Muted requests must avoid allocations even for a long endpoint string.

### Task1: Sound preference, UI and verification
**Files:** src/incoming_ringtone_policy.h, src/main.cpp, src/ui_help.h, tests/test_incoming_ringtone_mute*.cpp, CMakeLists.txt, version/docs.
**Interfaces:** fourthbool sound_muted argument, defaultfalse; UIbool notpersisted and noengine command.
- [ ] Observe missing-fourth-argument RED; implement early empty-request gate; four focused hosttests GREEN.
- [ ] Wire toggle/button/shortcut/mode badge; add real nativeGUI smoke for label, toggle, unchanged SIP observation, compact mode and recovery to normal.
- [ ] Run strict tests/fullcrossbuild/nativeGUIcompile/24Python/version checks and exact-head Windows/SDK/installer gates.
- [ ] Fresh final review, resolve material findings RED→GREEN, then authorized PR/main merge.
