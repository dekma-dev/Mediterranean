#include "game/ProfileStorage.h"
#include <Windows.h>
#include <atomic>
#include <cwctype>
#include <fstream>

namespace profile {
namespace {
constexpr std::size_t maxBytes=4096;
struct Mutex {
  HANDLE handle=nullptr;
  bool acquired=false;
  ~Mutex() {if(handle) {if(acquired) ReleaseMutex(handle);CloseHandle(handle);}}
};
struct Temporary {
  std::filesystem::path path;
  HANDLE handle=INVALID_HANDLE_VALUE;
  bool owned=false;
  ~Temporary() {if(handle!=INVALID_HANDLE_VALUE) CloseHandle(handle);if(owned) DeleteFileW(path.c_str());}
};
}
ReadResult Read(const std::filesystem::path& file,std::string& content) {
  try {
    std::error_code error;const bool exists=std::filesystem::exists(file,error);
    if(error) return ReadResult::Error;
    if(!exists) return ReadResult::Missing;
    const auto length=std::filesystem::file_size(file,error);
    if(error || !length || length>maxBytes) return ReadResult::Error;
    std::ifstream input(file,std::ios::binary);
    std::string candidate(std::size_t(length),'\0');
    if(!input.read(candidate.data(),std::streamsize(length)) || input.peek()!=std::char_traits<char>::eof()) return ReadResult::Error;
    content=std::move(candidate);return ReadResult::Ready;
  } catch(const std::exception&) {return ReadResult::Error;}
}
WriteResult Write(const std::filesystem::path& file,const std::string& content,const std::string* expected) {
  if(content.empty() || content.size()>maxBytes) return WriteResult::Error;
  try {
    std::error_code error;
    const auto canonical=std::filesystem::weakly_canonical(std::filesystem::absolute(file),error);
    if(error) return WriteResult::Error;
    unsigned long long hash=14695981039346656037ull;
    for(wchar_t c:canonical.wstring()) {hash^=static_cast<unsigned long long>(std::towlower(c));hash*=1099511628211ull;}
    const auto name=L"Local\\Mediterranean-profile-save-"+std::to_wstring(hash);
    Mutex mutex;mutex.handle=CreateMutexW(nullptr,FALSE,name.c_str());
    if(!mutex.handle) return WriteResult::Error;
    const auto wait=WaitForSingleObject(mutex.handle,0);
    mutex.acquired=wait==WAIT_OBJECT_0 || wait==WAIT_ABANDONED;
    if(!mutex.acquired) return wait==WAIT_TIMEOUT?WriteResult::Busy:WriteResult::Error;
    // An abandoned writer may have left a temporary file. Only the destination
    // is authoritative, and every writer compares it again before publication.
    const auto matches=[&]() {
      std::string current;const auto read=Read(file,current);
      if(read==ReadResult::Error) return WriteResult::Error;
      if(expected) return read==ReadResult::Ready && current==*expected?WriteResult::Saved:WriteResult::Conflict;
      return read==ReadResult::Missing?WriteResult::Saved:WriteResult::Conflict;
    };
    auto result=matches();if(result!=WriteResult::Saved) return result;
    if(!file.parent_path().empty()) std::filesystem::create_directories(file.parent_path(),error);
    if(error) return WriteResult::Error;
    static std::atomic<unsigned long long> serial{0};
    Temporary temp;temp.path=file;
    temp.path+=L".write-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L"-"+
      std::to_wstring(serial.fetch_add(1,std::memory_order_relaxed))+L".tmp";
    temp.handle=CreateFileW(temp.path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(temp.handle==INVALID_HANDLE_VALUE) return WriteResult::Error;
    temp.owned=true;DWORD written=0;
    if(!WriteFile(temp.handle,content.data(),DWORD(content.size()),&written,nullptr) || written!=content.size() ||
        !FlushFileBuffers(temp.handle)) return WriteResult::Error;
    const auto closed=CloseHandle(temp.handle);temp.handle=INVALID_HANDLE_VALUE;
    if(!closed) return WriteResult::Error;
    result=matches();if(result!=WriteResult::Saved) return result;
    if(!MoveFileExW(temp.path.c_str(),file.c_str(),MOVEFILE_WRITE_THROUGH|(expected?MOVEFILE_REPLACE_EXISTING:0))) return WriteResult::Error;
    temp.owned=false;return WriteResult::Saved;
  } catch(const std::exception&) {return WriteResult::Error;}
}
}
