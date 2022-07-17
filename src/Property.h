
#pragma once
#ifndef OPT_PROPERTY_H
#define OPT_PROPERTY_H﻿

#include "Serialize.h"

using namespace std;

class UnifiedObject;

using PropType = int;


enum PROP_FLAGS
{
  SEARCHABLE = 1,

  READONLY = 2,

  NO_DELETE = 4,
};




template <class DefType>
class DefinitionRegistry : public vector<unique_ptr<DefType>>
{
public:

  template<class CreateFn> 
  void add(int id, int flags, const string& name, CreateFn fn)
  {
    unique_ptr<DefType> newDefinition = make_unique<DefType>(id, flags, name, fn);

    auto it = find_if(begin(*this), end(*this), [&](const unique_ptr<DefType>& a) { return a->name_ == newDefinition->name_; });
    if (it != end(*this))
      throw ErrorCode(DuplicatedObjectName);
    it = lower_bound(begin(*this), end(*this), newDefinition,
      [](const unique_ptr<DefType>& a, const unique_ptr<DefType>& b) { return a->id_ < b->id_; });
    if (it != end(*this))
      if ((*it)->id_ == newDefinition->id_)
        throw ErrorCode(DuplicatedObjectId);
    vector<unique_ptr<DefType>>::insert(it, move(newDefinition));
  }

  const DefType& get(int pt) 
  {                                 
    auto tmp = make_unique<DefType>(pt);
    auto it = lower_bound(begin(*this), end(*this), tmp,
      [](const unique_ptr<DefType>& a, const unique_ptr<DefType>& b) { return a->id_ < b->id_; });
    if (it == end(*this))
      throw ErrorCode(UnknownObjType); 
    if ((*it)->id_ != pt)
      throw ErrorCode(UnknownObjType);
    return *(it->get());
  }

  static const DefType* findByName(const string&); 
};



class Property  
{
public:

  struct Definition
  {
    int id_;
    int flags_; 
    string name_;
    function<Property* (Reader&)> creator_;

    bool inline searchable() const { return 0 != (SEARCHABLE & flags_); }
    bool inline readonly() const { return 0 != (READONLY & flags_); }
    bool inline deletable() const { return 0 == (NO_DELETE & flags_); }
  };

  static DefinitionRegistry<Definition> propDefs_;

  template<class T>
  static void addPropertyDefinition(const string& name, int flags)
  {
    propDefs_.add<function<Property* (Reader&)>>(T::typeId_, flags, name, [](Reader& r) {return new T(r); });
  }


  virtual const Definition& def() const = 0;

  inline PropType type() const { return def().id_; }

  virtual void write(Writer&) const = 0;

  virtual void putInto(UnifiedObject*) {}

  virtual void removeFrom(UnifiedObject*) {}

  virtual ~Property() {}
};


using PropPtr = shared_ptr<Property>;







enum class DocVariantEnum
{
  Disabled,
  Enabled,
  Active
};


template <PropType PT, typename ValueType>
class PropValueTemplate : public Property
{
public:

  static constexpr PropType typeId_ = PT;

  const Definition& def() const override { static const Definition* d; return *(d ? d : d = &Property::propDefs_.get(PT)); }

  explicit PropValueTemplate(ValueType value) : value_(value) {}
  explicit PropValueTemplate(Reader& r) : value_(r.getInt<ValueType>()) { }




  void write(Writer& w) const override { w.save<ValueType>(value_); }
  inline ValueType value() const { return value_; }

protected:
  const ValueType value_;
};


template <PropType PT>
class PropValueTemplate<PT, double> : public Property
{
};

template <PropType PT>
class PropValueTemplate<PT, string> : public Property
{
public:
  static constexpr PropType id_ = PT;
  explicit PropValueTemplate(const string & value) : Property(PT), value_(value) {}
  explicit PropValueTemplate(Reader & r) : Property(PT), value_(r.getStr()) { }
  void write(Writer & w) const override { w.save(value_); }
  inline const string & value() const { return value_; }

protected:
  const string value_;
};






using DocVariantProp = PropValueTemplate<155, DocVariantEnum>;

using DocIdProp = PropValueTemplate<1, DocId>;

using UserIdProp = PropValueTemplate<2, UserId>;


#endif


