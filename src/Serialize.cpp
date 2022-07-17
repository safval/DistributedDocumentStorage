
// Data serialization: objects, properties, tranzactions

#include "Serialize.h"
#include <cstring>

#include "../json/single_include/nlohmann/json.hpp"
using namespace nlohmann;

using namespace std;




// Skip current element with all nested children array/map
void Reader::skip()
{
  switch (nextDataType())
  {
    case DataType::Int:
    {
      int64_t i;
      load(i);
      break;
    }
    case DataType::Real:
      getReal();
      break;
    case DataType::String:
      getStr();
      break;
    case DataType::Null:
      getNull();
      break;
    case DataType::Array:
    {
      size_t size = getArray();
      while (size--)
        skip();
      break;
    }
    case DataType::Map:
    {
      size_t size = getMap();
      while (size--)
      {
        skip();
        skip();
      }
      break;
    }
  }
}

void Reader::getArray(size_t expectedSize)
{
  if (getArray() != expectedSize)
    throw ErrorCode(SerializationFormatError);
}

void Reader::getMap(size_t expectedSize)
{
  if (getMap() != expectedSize)
    throw ErrorCode(SerializationFormatError);
}

string Reader::getStr()
{
  string str;
  load(str);
  return str;
}

void MSItem::setStr(const char* str)
{
  char* s = new char[strlen(str) + 1];
  strcpy(s, str);
  str_ = s;
}

MSItem::MSItem(MSItem&& other) : type_(other.type_)
{
  switch (type_)
  {
    case Reader::DataType::String: str_ = other.str_; other.str_ = nullptr; break;
    case Reader::DataType::Int: int_ = other.int_; break;
    case Reader::DataType::Real: real_ = other.real_; break;
    case Reader::DataType::Null:
    case Reader::DataType::Array:
    case Reader::DataType::Map: size_ = other.size_; break;
  }
}

MSItem::MSItem(const MSItem& other) : type_(other.type_)
{
  switch (type_)
  {
    case Reader::DataType::String: setStr(other.str_); break;
    case Reader::DataType::Int: int_ = other.int_; break;
    case Reader::DataType::Real: real_ = other.real_; break;
    case Reader::DataType::Null:
    case Reader::DataType::Array:
    case Reader::DataType::Map: size_ = other.size_; break;
  }
}

MSItem::~MSItem()
{
  if (Reader::DataType::String == type_)
    delete str_;
}


using MSDataT = vector<MSItem>;

MSReader::MSReader(const MSDataT& data) : data_(data), ptr_(data.begin())
{
  if (data_.empty())
    throw ErrorCode(SerializationInternalError); 
}

Reader::DataType MSReader::nextDataType() const
{
  if (ptr_ == data_.end())
    throw ErrorCode(SerializationInternalError);
  return ptr_->type_;
}

void MSReader::load(int64_t& i)
{
  ensureType(DataType::Int);
  i = ptr_->int_;
  ++ptr_;
}

void MSReader::load(string& str)
{
  ensureType(DataType::String);
  str = ptr_->str_;
  ++ptr_;
}

double MSReader::getReal()
{
  ensureType(DataType::Real);
  return (ptr_++)->real_;
}

size_t MSReader::getArray()
{
  ensureType(DataType::Array);
  return (ptr_++)->size_;
}

size_t MSReader::getMap()
{
  ensureType(DataType::Map);
  return (ptr_++)->size_;
}

void MSReader::getNull()
{
  ensureType(DataType::Null);
  ++ptr_;
}

void MSReader::ensureType(DataType type) const
{
  if (type != nextDataType())
    throw ErrorCode(SerializationFormatError);
}

MSWriter::MSWriter(MSDataT& data) : data_(data)
{
}

void MSWriter::putInt(int64_t i)
{
  data_.emplace_back(Reader::DataType::Int);
  data_.back().int_ = i;
}

void MSWriter::putReal(double d)
{
  data_.emplace_back(Reader::DataType::Real);
  data_.back().real_ = d;
}

void MSWriter::putStr(const string& str)
{
  data_.emplace_back(Reader::DataType::String);
  data_.back().setStr(str.c_str());
}

void MSWriter::putArray(size_t size)
{
  data_.emplace_back(Reader::DataType::Array);
  data_.back().size_ = size;
}

void MSWriter::putMap(size_t size)
{
  data_.emplace_back(Reader::DataType::Map);
  data_.back().size_ = size;
}

void MSWriter::putNull()
{
  data_.emplace_back(Reader::DataType::Null);
}

Reader* MSWriter::getBackReader()
{
  return new MSReader(data_);
}




  
void JsonWriter::putInt(int64_t i)
{
  data_ += to_string(i);
  writeBack();
}

void JsonWriter::putReal(double value)
{
  data_ += to_string(value);
  writeBack();
}


void JsonWriter::putStr(const string& str)
{
  data_ += "\"" + str + "\""; 
  writeBack();
}

void JsonWriter::putNull()
{
  data_ += "null";
  writeBack();
}

void JsonWriter::putArray(size_t size)
{
  if (size)
  {
    cs_.emplace_back(false, size);
    data_ += "[";
  }
  else
  {
    data_ += "[]";
    writeBack();
  }
}

void JsonWriter::putMap(size_t size)
{
  if (size)
  {
    cs_.emplace_back(true, 2 * size);
    data_ += "{";
  }
  else
  {
    data_ += "{}";
    writeBack();
  }
}

const string& JsonWriter::data()
{
  writeBack();
  return data_;
}

JsonWriter::operator const char* ()
{
  return data().c_str();
}

void JsonWriter::writeBack()
{
  if (!cs_.empty())
  {
    auto& cs = cs_.back();
    if (--cs.count_ == 0)
    {
      data_ += (cs.isMap_ ? "}" : "]");
      cs_.pop_back();
      writeBack();
    }
    else
    {
      if (cs.isMap_ && (cs.count_ % 2))
        data_ += ":";
      else
        data_ += ",";
    }
  }
}







  Reader::DataType CborReader::nextDataType() const 
  {
    switch (majorType())
    {
      case 0:    
      case 0x20: 
        return DataType::Int;
      case 0x60:
        return DataType::String;
      case 0x80:
        return DataType::Array;
      case 0xA0:
        return DataType::Map;
      case 0xE0:
        switch (peekByte())
        {
          case 0xE0 | 22: return DataType::Null;
          case 0xE0 | 26: return DataType::Real; 
          case 0xE0 | 27: return DataType::Real; 
        }
    }

    throw ErrorCode(SerializationFormatError);
  }

  void CborReader::load(int64_t& i)  
  {
    if (!isInt())
      throw ErrorCode(SerializationFormatError);

    const auto negative = 0x20 & peekByte();
    const uint64_t value = additionalInfo();

    if (negative)
    {
      if (value > -(INT64_MIN + 1))
        throw ErrorCode(SerializationInternalError); 
      i = -1 - value;
    }
    else
    {
      if (value > INT64_MAX)
        throw ErrorCode(SerializationInternalError); 
      i = value;
    }
  }

  void CborReader::load(string& str)
  {
    if (!isStr())
      throw ErrorCode(SerializationFormatError);

    int64_t i = additionalInfo();

    str.reserve(i);

    while (i--)
    {
      str.push_back(getByte());
    }
  }

  uint64_t CborReader::additionalInfo()
  {
    uint8_t firstByte = getByte();
    switch (firstByte & 0x1f)
    {
      case 24: return getBytes(1);
      case 25: return getBytes(2);
      case 26: return getBytes(4);
      case 27: return getBytes(8);
      case 28: 
      case 29: 
      case 30: 
      case 31: 
        throw ErrorCode(SerializationFormatError);
      default: return firstByte & 0x1f;
    }
  }

  double CborReader::getReal()
  {
    if (!isReal())
      throw ErrorCode(SerializationFormatError);

    uint8_t chars[8];
    double res = 0;

    switch (getByte())
    {
      case 0xfa: 
        chars[3] = getByte();
        chars[2] = getByte();
        chars[1] = getByte();
        chars[0] = getByte();
        res = *reinterpret_cast<float*>(chars);
        break;

      case 0xfb: 
        chars[7] = getByte();
        chars[6] = getByte();
        chars[5] = getByte();
        chars[4] = getByte();
        chars[3] = getByte();
        chars[2] = getByte();
        chars[1] = getByte();
        chars[0] = getByte();
        res = *reinterpret_cast<double*>(chars);
        break;
    }

    return res;
  }

  size_t CborReader::getArray()
  {
    if (!isArray())
      throw ErrorCode(SerializationFormatError);
    return additionalInfo();
  }

  size_t CborReader::getMap()
  {
    if (!isMap())
      throw ErrorCode(SerializationFormatError);
    return additionalInfo();
  }

  void CborReader::getNull()
  {
    if (!isNull())
      throw ErrorCode(SerializationFormatError);
    getByte(); 
  }

  uint8_t CborReader::majorType() const
  {
    return 0xe0 & peekByte();
  }

  uint64_t CborReader::getBytes(size_t count)
  {
    uint64_t res = 0;
    while (count--)
    {
      res = (res << 8) | getByte();
    }
    return res;
  }

  


  CborMemReader::CborMemReader(const vector<uint8_t>& data) : data_(data), ptr_(data.begin())
  {
    if (data_.empty())
      throw ErrorCode(SerializationInternalError); 
  }


  uint8_t CborMemReader::getByte()
  {
    uint8_t res = peekByte();
    ptr_++;
    return res;
  }

  uint8_t CborMemReader::peekByte() const
  {
    if (ptr_ == data_.end())
      throw ErrorCode(SerializationFormatError);
    return *ptr_;
  }


  CborFileReader::CborFileReader(const filesystem::path& p)
    : file_(p, ios_base::in | ios_base::binary)
  {
  }

  uint8_t CborFileReader::getByte()
  {
    if (!file_.good())
      throw ErrorCode(SerializationFileOpenError);
    return file_.get();
  }

  uint8_t CborFileReader::peekByte() const
  {
    if (!file_.good())
      throw ErrorCode(SerializationFileOpenError);
    return file_.peek();
  }




  void CborWriter::putInt(int64_t value)
  {
    if (value >= 0)
    {
      putHeader(value, 0);
    }
    else 
    {
      putHeader(-value - 1, 0x20);
    }
  }

  void CborWriter::putNull()
  {
    putHeader(22, 0xe0);
  }

  void CborWriter::putArray(size_t size)
  {
    putHeader(size, 0x80);
  }

  void CborWriter::putMap(size_t size)
  {
    putHeader(size, 0xA0);
  }



  void CborMemWriter::putStr(const string& str)
  {
    const size_t strLen = str.length();
    putHeader(strLen, 0x60);
    data_.insert(data_.end(), str.begin(), str.end());
  }

  void CborMemWriter::putReal(double value)
  {
    data_.push_back(0xe0 | 27);

    char* byteArray = reinterpret_cast<char*>(&value);
    data_.push_back(byteArray[7]);
    data_.push_back(byteArray[6]);
    data_.push_back(byteArray[5]);
    data_.push_back(byteArray[4]);
    data_.push_back(byteArray[3]);
    data_.push_back(byteArray[2]);
    data_.push_back(byteArray[1]);
    data_.push_back(byteArray[0]);
  }

  void CborMemWriter::putHeader(int64_t value, int mask)
  {
    auto putBytes = [&](int64_t value, size_t count)
    {
      while (count--)
      {
        data_.push_back(static_cast<uint8_t>(value >> (count << 3)));
      }
    };

    if (value <= 23)
    {
      data_.push_back(static_cast<uint8_t>(value | mask));
    }
    else if (value <= 0xff)
    {
      data_.push_back(0x18 | mask);
      data_.push_back(static_cast<uint8_t>(value));
    }
    else if (value <= 0xffff)
    {
      data_.push_back(0x19 | mask);
      putBytes(value, 2);
    }
    else if (value <= 0xffff'ffff)
    {
      data_.push_back(0x1A | mask);
      putBytes(value, 4);
    }
    else
    {
      data_.push_back(0x1B | mask);
      putBytes(value, 8);
    }
  }
  

  CborFileWriter::CborFileWriter(const filesystem::path& p)
    : file_(p, ios_base::out | ios_base::binary | ios_base::trunc)
  {
  }

  CborFileWriter::~CborFileWriter()
  {
    file_.close();
  }
  
  void CborFileWriter::putStr(const string& str)
  {
    const size_t strLen = str.length();
    putHeader(strLen, 0x60);
    file_.write(str.c_str(), strLen);
  }

  void CborFileWriter::putReal(double value)
  {
    uint8_t c = 0xe0 | 27;
    file_.write((const char*)&c, 1);

    char* byteArray = reinterpret_cast<char*>(&value);
    file_.write(byteArray + 7, 1);
    file_.write(byteArray + 6, 1);
    file_.write(byteArray + 5, 1);
    file_.write(byteArray + 4, 1);
    file_.write(byteArray + 3, 1);
    file_.write(byteArray + 2, 1);
    file_.write(byteArray + 1, 1);
    file_.write(byteArray + 0, 1);
  }

  void CborFileWriter::putHeader(int64_t value, int mask)
  {
    auto putBytes = [&](int64_t value, size_t count)
    {
      while (count--)
      {
        file_.put(static_cast<uint8_t>(value >> (count << 3)));
      }
    };

    if (!file_.good())
      throw ErrorCode(SerializationFileOpenError);

    if (value <= 23)
    {
      file_.put(static_cast<uint8_t>(value | mask));
    }
    else if (value <= 0xff)
    {
      file_.put(0x18 | mask);
      file_.put(static_cast<uint8_t>(value));
    }
    else if (value <= 0xffff)
    {
      file_.put(0x19 | mask);
      putBytes(value, 2);
    }
    else if (value <= 0xffff'ffff)
    {
      file_.put(0x1A | mask);
      putBytes(value, 4);
    }
    else
    {
      file_.put(0x1B | mask);
      putBytes(value, 8);
    }

  }


