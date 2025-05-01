# PJSUA2Manager - Dart FFI Binding for PJSUA2 SIP Library (Use for raspberry)

A Dart FFI-compatible C++ wrapper for the PJSUA2 SIP stack, enabling VoIP call management in Dart applications.

## Features
- **Call Management**: Initiate, answer, and terminate calls.
- **Event Handling**: 
  - Incoming call notifications.
  - Registration state changes.
  - Real-time call state updates.
- **Error Reporting**: Capture detailed error information (message, source, line number).
- **Multi-Threaded Event Loop**: Async event processing.

## Prerequisites
- **Dart SDK** 2.12+ (FFI support)
- **PJSUA2 Library** (libpjproject)
- **C++ Build Tools** (GCC/Clang on Linux/macOS)
- **Dart FFI Knowledge**

## Installation
1. **Compile the Shared Library**:
   ```bash
   g++ -std=c++17 -fPIC -shared \
       -I/path/to/pjsip/include \
       -L/path/to/pjsip/lib \
       pjsua2_manager.cpp -o libpjsua2manager.so \
       -lpjproject