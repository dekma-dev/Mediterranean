#pragma once
#include <filesystem>
#include <string>

namespace profile {
enum class ReadResult { Missing, Ready, Error };
enum class WriteResult { Saved, Conflict, Busy, Error };
// Bounded reads leave the caller's output unchanged on failure.
ReadResult Read(const std::filesystem::path& file,std::string& content);
// Null expected means create-only. Otherwise the current file must match the
// exact snapshot last entered, loaded or saved. Cooperating writers serialize
// the comparison and publication; no waiting on the game thread.
WriteResult Write(const std::filesystem::path& file,const std::string& content,const std::string* expected=nullptr);
}
