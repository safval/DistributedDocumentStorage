
#pragma once
#ifndef OPT_OBJECT_H
#define OPT_OBJECT_H﻿

#include "Property.h" 

class UnifiedObject;
class ObjectStorage;
struct ObjectChanges;


using ObjType = int;

int constexpr ObjTypeUnchanged = -3;
int constexpr ObjTypeDeleted = -4;


using ObjName = uint32_t;

struct LongName : public vector<ObjName>
{
public:
  LongName() {}
  LongName(const initializer_list<ObjName>& list) : vector<ObjName>(list) {}
};








enum OBJ_FLAGS
{
  TOPOBJECT = 1
};


enum CHG_OBJ_FLAGS 
{
  osCreated = 0x0001, 
  osActive = 0x8000, 
  osDeleted = 0x0002, 
  osAddProp = 0x0004,
  osDelProp = 0x0008,
  osChgProp = 0x0010,
  osAddLink = 0x0020,
  osDelLink = 0x0040,
  osParentDeleted = 0x0080,
  osParentChanged = 0x0100,
};


// Unified multi-purpose object. Document consist of these objects.
// Object have type, unique name and set of properties.
// Using tranzaction is the only way to create, change and delete object.
class UnifiedObject 
{

public:
  struct Definition
  {
    ObjType id_;
    int flags_; 
    string name_; 
    function<UnifiedObject* ()> creator_;

    bool inline isTopObject() const { return 0 != (TOPOBJECT & flags_); }
  };

  static DefinitionRegistry<Definition> objDefs_;

  template<class T>
  static void addObjectDefinition(const string& name, int flags)
  {
    objDefs_.add<std::function<UnifiedObject*()>>(T::typeId_, flags, name, [](){ return new T(); });
  }

  virtual const Definition& def() const = 0;

private:

  struct LinkedObjIterContainer
  {
    struct Iter 
    {
      const LinkedObjIterContainer& container_;
      size_t pos_;

      bool operator!= (const Iter& other) const { return pos_ != other.pos_; }
      const Iter& operator++ () { pos_ = findNext(container_, pos_); return *this; }

      static size_t findNext(const LinkedObjIterContainer& container_, size_t pos);

      UnifiedObject* operator* () const { return container_.objects_[pos_].second; }
    };

    explicit LinkedObjIterContainer(PropType pt, const vector<pair<PropType, UnifiedObject*>>& objects) : propType_(pt), objects_(objects) {}
    Iter begin() const { return{ *this, Iter::findNext(*this, SIZE_MAX) }; }
    Iter end() const { return{ *this, objects_.size() }; }
    const vector<pair<PropType, UnifiedObject*>>& objects_;
    const PropType propType_;
  };


public:

  virtual ObjectStorage* isStorage() const { return nullptr; }

  // Is the object exists. Deleted object only marked as deleted when tranzaction applied.
  bool inline isActive() const { return !!(state_ & osActive); }

  inline ObjType type() const { return def().id_; }

  inline ObjName name() const { return name_; }

  LongName LName() const;

  ObjectStorage& storage() const
  {
    if (!storage_)
      throw ErrorCode(1245);
    return *storage_;
  }

  bool changeRemove();

  bool changeRemove(PropType pt);

  void change(const ObjectChanges& chgs);

  string debugString() const;
  

protected:
  virtual void onChange(uint16_t) {}

public:
  void initIfChangedAndClearState();

  const Property* findProp(PropType pt) const;
  template <class T> inline const T* findProp() const { return static_cast<const T*>(findProp(T::typeId_)); }

  inline size_t propertyCount() const { return props_.size(); }

  inline const vector<PropPtr>& props() const { return props_; }

  LinkedObjIterContainer linked(PropType pt = 0) const { return LinkedObjIterContainer(pt, linked_); }

  vector<int> availableEnums(PropType) const; 

  virtual ~UnifiedObject() = default;

protected:
  UnifiedObject() = default;

private:
  friend class ObjectStorage;

  // Coping is prohibited because of object name uniqueness
  UnifiedObject(const UnifiedObject&) = delete;
  const UnifiedObject& operator=(const UnifiedObject&) = delete;

  // Object cannot have several properties with same type.
  // Object may be empty, it is useful sometimes.
  mutable vector<PropPtr> props_;    
  mutable bool resortProps_ = false; 

  vector<PropPtr>::iterator findPropIterator(PropType pt) const;

  friend class LinkToObject;
  vector<pair<PropType, UnifiedObject*>> linked_;

  uint16_t state_ = 0;


  // Assygned by ObjectStorage while object creating
  ObjName name_ = 0;
  // Every object is stored in ObjectStorage. 
  // All storage, except top storage, are UnifiedObject.
  ObjectStorage* storage_ = nullptr;
};

template <ObjType OT>
class UnifiedObjectTempl : public UnifiedObject
{
public:
  static constexpr ObjType typeId_ = OT;
  const Definition& def() const override { static const Definition* d; return *(d ? d : d = &UnifiedObject::objDefs_.get(OT)); }
};




struct ObjectChanges 
{
  // Construct by a document editor
  ObjectChanges(ObjType type, ObjectStorage &);  // Create new object
  ObjectChanges(const UnifiedObject&);           // Change or delete of object or property
  ObjectChanges(const LongName&);                // Change or delete of object or property

  // Construct from tranzaction storage archive
  ObjectChanges(Reader&);

  void write(Writer&) const;

  // Merge two data changes into this object
  void merge(const ObjectChanges*);

  // Apply changes
  ObjectChanges & prop(Property*);  // Add or change property. This object become owner of the property object
  ObjectChanges & remove(PropType); // Delete property
  void remove();                    // Delete object


  inline const LongName & objName() const { return name_; }
  inline ObjType objType() const { return objType_; }
  inline const vector<PropPtr> & props() const { return props_; }
  inline const vector<PropType> & del() const { return del_; }

  bool isFirstObject() const;
  PropPtr findProp(PropType pt) const;


  static constexpr int SerialSize = 4; 

protected:

  LongName name_;

  // Type of object to create, or 
  // ObjTypeDeleted to delete object by name, or
  // ObjTypeNoChanged if no object creating nor deleting here
  ObjType objType_;

  /// added ar changed properties
  vector<PropPtr> props_;
  
  // properties to delete
  vector<PropType> del_;

private:
  ObjectChanges(const ObjectChanges&) = delete;
  void operator = (const ObjectChanges&) = delete;
};



class LinkToObject
{
public:

  void makeLink(PropType pt, UnifiedObject* owner)
  {
    if (UnifiedObject* obj = findLinkedObject(owner))
    {
      obj->state_ |= osAddLink;
      obj->linked_.emplace_back(pt, owner);
      sort(obj->linked_.begin(), obj->linked_.end(),
        [](const pair<PropType, UnifiedObject*>& p1, const pair<PropType, UnifiedObject*>& p2)
        { return int(p1.first) < int(p2.first); });
    }
  }

  void removeLink(PropType pt, UnifiedObject* owner)
  {
    if (UnifiedObject* obj = findLinkedObject(owner))
    {
      for (auto it = obj->linked_.begin(); it != obj->linked_.end(); ++it)
        if (it->first == pt && it->second == owner)
        {
          obj->linked_.erase(it);
          obj->state_ |= osDelLink;
          return;
        }
      throw ErrorCode(1265);
    }
  }

  void read(Reader& r)
  {
    vector<uint32_t> tmp;
    r.getVector(tmp);
    if (tmp.size() >= 2)
    {
      upStor_ = tmp.front();
      name_.assign(tmp.begin() + 1, tmp.end());
    }
    else
    {
      upStor_ = 0;
      name_.clear();
      ASSERT(false == (bool)*this);
    }
  }

  void write(Writer& w) const
  {
    if (!name_.empty())
    {
      if (upStor_ > 999)
        throw ErrorCode(2);

      vector<uint32_t> tmp;
      tmp.reserve(name_.size() + 1);
      tmp.push_back(upStor_);
      tmp.insert(tmp.end(), name_.begin(), name_.end());
      w.putVector(tmp);
    }
    else
      w.putArray(0);
  }


  uint32_t upStor_ = 0;

  LongName name_;

  UnifiedObject* findLinkedObject(const UnifiedObject* propertyOwner) const;


  operator bool() const { return !name_.empty(); }
};


template <PropType PT>
class PropValueTemplate<PT, LinkToObject> : public Property, protected LinkToObject
{
public:

  static constexpr PropType typeId_ = PT;

  const Definition& def() const override { static const Definition* d; return *(d ? d : d = &propDefs_.get(PT)); }

  explicit PropValueTemplate(uint32_t upStor, const LongName& name) { upStor_ = upStor; name_ = name; }
  explicit PropValueTemplate(Reader& r) { LinkToObject::read(r); }

  void write(Writer& w) const override { LinkToObject::write(w); }

  void putInto(UnifiedObject* obj) override { makeLink(PT, obj); }
  void removeFrom(UnifiedObject* obj) override { removeLink(PT, obj); }

  template <class T = UnifiedObject>
  inline T* object(const UnifiedObject* propertyOwner) const { return static_cast<T*>(findLinkedObject(propertyOwner)); }
};







class InsertedDocument : public UnifiedObjectTempl<101>
{
};

class UserProfile : public UnifiedObjectTempl<240> 
{
};

#endif

