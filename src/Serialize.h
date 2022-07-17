
#pragma once
#ifndef OPT_SERIALIZE_H
#define OPT_SERIALIZE_H﻿




#include "Config.h"
#include <fstream>


class Reader
{
public:

  virtual bool isBinaryFormat() const = 0;

  enum class DataType { Int, Real, String, Null, Array, Map };
  virtual DataType nextDataType() const = 0;

  inline bool isInt() const { return DataType::Int == nextDataType(); }
  inline bool isReal() const { return DataType::Real == nextDataType(); }
  inline bool isStr() const { return DataType::String == nextDataType(); }
  inline bool isNull() const { return DataType::Null == nextDataType(); }
  inline bool isArray() const { return DataType::Array == nextDataType(); }
  inline bool isMap() const { return DataType::Map == nextDataType(); }

  virtual void load(int64_t&) = 0;
  virtual void load(string&) = 0;
  virtual double getReal() = 0;
  virtual void getNull() = 0;
  virtual size_t getArray() = 0;
  virtual size_t getMap() = 0;

  // Skip current element with all nested children array/map
  void skip();

  virtual void startEncription(const string&) { throw ErrorCode(NotImplemented); }

  // Read array's header. Throw an exception in case the size mismatch.
  void getArray(size_t expectedSize);

  // Read map's header. Throw an exception in case the size mismatch.
  void getMap(size_t expectedSize);
  


  // Slight slower but more convient method to read string.
  string getStr();
  
  // Read vector of any integer types
  template<typename T>
  void getVector(vector<T>& v)
  {
    size_t size = getArray();
    v.reserve(size);
    while (size--)
      v.push_back(getInt<T>());
  }

  template <typename IntT> inline void load(IntT& t) { int64_t i; load(i); t = static_cast<IntT>(i); }

  template <typename IntT = int> inline IntT getInt() { int64_t i; load(i); return static_cast<IntT>(i); }

  virtual ~Reader() = default;
};


class Writer
{
public:
  virtual bool isBinaryFormat() const = 0;

  template <class IntType>
  void save(IntType t)
  {
    static_assert(sizeof(t) <= sizeof(int64_t), "");
    if (sizeof(t) == sizeof(int64_t))
      if (uint64_t(t) > INT64_MAX)
        throw ErrorCode(6);
    putInt(static_cast<int64_t>(t));
  }



  virtual void putInt(int64_t) = 0;
  virtual void putReal(double) = 0;
  virtual void putStr(const string&) = 0;
  virtual void putNull() = 0;
  virtual void putArray(size_t) = 0; 
  virtual void putMap(size_t) = 0; 

  virtual void startEncription(const string&) { throw ErrorCode(NotImplemented); }



///    if (sizeof(T) >= sizeof(int32_t))
///    {
///      if (i < INT32_MIN || i > INT32_MAX || i < INT32_MIN || i > INT32_MAX)
///        
///    }

  template<typename T>
  void putVector(const vector<T>& v)
  {
    size_t size = v.size();
    auto ptr = v.begin();
    putArray(size);
    while (size--)
      save<T>(*(ptr++));
  }

  virtual ~Writer() = default;
};










struct MSItem 
{
  const Reader::DataType type_;
  explicit MSItem(Reader::DataType t) : type_(t) {}
  union
  {
    int64_t int_;
    double real_;
    const char* str_ = nullptr;
    size_t size_;
  };
  void setStr(const char* str);
  
  MSItem(MSItem&& other);
  
  MSItem(const MSItem& other);
  
  ~MSItem();
};

using MSDataT = vector<MSItem>;


class MSReader : public Reader
{
public:
  explicit MSReader(const MSDataT& data);

  bool isBinaryFormat() const override { return true; }

  Reader::DataType nextDataType() const;
  
  void load(int64_t& i);
  
  void load(string& str);
  
  double getReal();
  
  size_t getArray();
  
  size_t getMap();
  
  void getNull();
  
protected:
  const MSDataT& data_;
  MSDataT::const_iterator ptr_;
  void ensureType(DataType type) const;
  
private:
  MSReader(const MSReader&) = delete;
  void operator = (const MSReader&) = delete;
};


class MSWriter : public Writer
{
public:
  explicit MSWriter(MSDataT&);

  bool isBinaryFormat() const override { return true; }

  void putInt(int64_t i);
  
  void putReal(double d);
  
  void putStr(const string& str);
  
  void putArray(size_t size);
  
  void putMap(size_t size);
  
  void putNull();
  
  Reader* getBackReader();

protected:
  MSDataT& data_;

private:
  MSWriter(const MSWriter&) = delete;
  void operator=(const MSWriter&) = delete;
};



class JsonWriter : public Writer
{
  struct ContainerState
  {
    ContainerState(bool isMap, size_t count) : isMap_(isMap), count_(count) {}
    bool isMap_;
    size_t count_;
  };
  vector<ContainerState> cs_;
public:

  JsonWriter() = default;

  bool isBinaryFormat() const override { return false; }

  void putInt(int64_t i);
  
  void putReal(double value);
  
  void putStr(const string& str);
  
  void putNull();
  
  void putArray(size_t size);
  
  void putMap(size_t size);
  
  const string& data();
  operator const char* ();

protected:
  void writeBack();
  
  string data_;

private:
  JsonWriter(const JsonWriter&) = delete;
  void operator=(const JsonWriter&) = delete;
};




class CborReader : public Reader
{
public:

  bool isBinaryFormat() const override { return true; }

  DataType nextDataType() const override;
  
  void load(int64_t&) override;
  
  void load(string&);
  
  uint64_t additionalInfo();
  
  double getReal();
  
  size_t getArray();
  
  size_t getMap();
  
  void getNull();
  
protected:

  uint8_t majorType() const;
  
  uint64_t getBytes(size_t count);
  
  virtual uint8_t getByte() = 0;
  virtual uint8_t peekByte() const = 0;
};


class CborMemReader : public CborReader
{
public:

  explicit CborMemReader(const vector<uint8_t>& data);
  
protected:
  const vector<uint8_t>& data_;
  vector<uint8_t>::const_iterator ptr_;

  uint8_t getByte() override;
  
  uint8_t peekByte() const override;
  
private:
  CborMemReader(const CborMemReader&) = delete;
  void operator=(const CborMemReader&) = delete;
};


class CborFileReader : public CborReader
{
public:
  explicit CborFileReader(const filesystem::path&);
  
protected:
  mutable ifstream file_;

  uint8_t getByte();
  
  uint8_t peekByte() const;
  
private:
  CborFileReader(const CborFileReader&) = delete;
  void operator = (const CborFileReader&) = delete;
};


class CborWriter : public Writer
{
public:

  bool isBinaryFormat() const override { return true; }

  void putInt(int64_t value) override;
  
  void putNull() override;
  
  void putArray(size_t size) override;
  
  void putMap(size_t size) override;
  
protected:
  virtual void putHeader(int64_t value, int mask) = 0;
};


class CborMemWriter : public CborWriter
{
public:
  CborMemWriter() = default;

  void putStr(const string& str) override;
  
  void putReal(double value) override;
  
  inline const vector<uint8_t>& data() const { return data_; }

protected:
  void putHeader(int64_t value, int mask) override;
  
  vector<uint8_t> data_;
private:
  CborMemWriter(const CborMemWriter&) = delete;
  void operator=(const CborMemWriter&) = delete;
};


class CborFileWriter : public CborWriter
{
public:
  explicit CborFileWriter(const filesystem::path& p);

  ~CborFileWriter();
  
  void putStr(const string& str) override;
  
  void putReal(double value) override;
  
protected:
  ofstream file_;

  void putHeader(int64_t value, int mask) override;
  
private:
  CborFileWriter(const CborFileWriter&) = delete;
  void operator=(const CborFileWriter&) = delete;
};

#endif

