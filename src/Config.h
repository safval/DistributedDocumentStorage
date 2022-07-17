
#ifndef OPT_CONFIG_H
#define OPT_CONFIG_H

#include <array>
#include <memory>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <functional>
#include <optional>
#include <string>
#include <mutex>
#include <climits>
#include <filesystem>
#include <codecvt>
#include <exception>
#include <cstdlib>

using namespace std;

// date and time (unixtime * 1000)  https://www.unixtimestamp.com
// always use UTC, timezone is't stored here
using datetime_t = int64_t;

using UserId = uint32_t;
constexpr UserId DefaultLocalUserId = 1;
constexpr UserId MainUserId = 101; 
constexpr UserId InvalidUserId = 0; 

// Document identifier, cannot be zero
using DocId = uint64_t;
constexpr DocId createDocId(UserId user, datetime_t time)
{
  return (uint64_t(user) << 32) | uint32_t(time/1000);
}

using DeviceId = uint64_t;

constexpr DocId StorageInfoDocId = 1;

// Current version for data serialization
constexpr int SerializationFormatVersion = 1;

// Error codes
constexpr int DuplicatedObjectName = 993;
constexpr int DuplicatedObjectId = 994;
constexpr int UnknownObjType = 995;
constexpr int UnknownPropType = 996;
constexpr int WrongPropertyId = 997;
constexpr int DuplicatedPropertyName = 998;
constexpr int DuplicatedPropertyId = 999;
constexpr int NotImplemented = 1000;
constexpr int NoUndoRedoData = 225;
constexpr int SelfInsertedDocument = 228;

constexpr int SerializationFormatError = 229;
constexpr int SerializationInternalError = 230;
constexpr int SerializationFileOpenError = 231; 




class ErrorCode : public std::exception
{
public:
  ErrorCode(int code) : code_(code) {}
  const int code_;
};


#define ASSERT(t) void(t);

#endif

