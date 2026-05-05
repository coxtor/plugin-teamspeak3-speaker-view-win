#pragma once

#include <QtCore/QString>

// Write a log line to TS3's own logging subsystem. Lines appear in
// %APPDATA%\TS3Client\logs\ts3client_*.log with channel "speakerview".
// Safe to call from any thread — TS3's logMessage is thread-safe, and
// if the plugin's ts3_functions are not yet installed the call is a no-op.
void sv_log(const QString& msg);

// Convenience overload for C-string literals.
inline void sv_log(const char* msg) { sv_log(QString::fromUtf8(msg)); }
