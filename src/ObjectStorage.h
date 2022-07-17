#pragma once

#ifndef OBJECT_STORAGE_H20190207
#define OBJECT_STORAGE_H20190207

#include "Tranzaction.h" 
class TopObjectStorage;



class ObjectStorage
{
public:

  UnifiedObject * create(ObjType, ObjName);

  virtual UnifiedObject * isObject() const { return nullptr; }

  virtual TopObjectStorage * isDocument() const { return nullptr; }

  UnifiedObject * findObjectByName(ObjName) const; 
  UnifiedObject * findObject(const LongName &) const;

  UnifiedObject * findObjectByType(ObjType) const; 
  template<class ObjTypeName> inline ObjTypeName * findObject() const { return static_cast<ObjTypeName*>(findObjectByType(ObjTypeName::typeId_)); }


  ObjectStorage * findStorageOf(const LongName & name) const;

  ObjectStorage * findStorage(ObjName) const;

  ObjectStorage* storage() const { return isObject() ? &isObject()->storage() : nullptr; }



  string debugString() const;
  void save(Writer&, bool onlySelected) const; 

  size_t size(bool withChildren) const noexcept;

  struct IterContainer 
  {
    struct Iter 
    {
      const IterContainer & container_;
      size_t pos_;

      bool operator!= (const Iter& other) const { return pos_ != other.pos_; }
      const Iter& operator++ () { pos_++; return *this; }
      UnifiedObject & operator* () const
      {
        UnifiedObject & obj = *container_.objects_[pos_];
        if (container_.withChildren_)
          if (ObjectStorage * storage = obj.isStorage())
          { } 
        return obj;
      }
    };

    explicit IterContainer(const vector<UnifiedObject*> & objects, bool withChildren) : objects_(objects), withChildren_(withChildren) {}
    Iter begin() const { return{ *this, 0 }; }
    Iter end() const { return{ *this, objects_.size() }; }
    const vector<UnifiedObject*> & objects_;
    bool withChildren_;
  };
  IterContainer objects(bool withChildren) const { return IterContainer(objects_, withChildren); }


  const vector<UnifiedObject*>& objects() const { return objects_; }


  virtual ~ObjectStorage();

  inline ObjName nextName() const { return nextName_; }

  inline ObjName reserveName() { return nextName_++; }

protected:
  ObjName nextName_ = 1;

  void clear();

  vector<UnifiedObject*> objects_;

  vector<UnifiedObject*>::const_iterator findObjectIterator(ObjName) const;
};




class TopObjectStorage : public ObjectStorage, public TrzIO
{
public:

  DocId docId() const { return docId_; }
  inline UserId userId() const { return static_cast<UserId>(docId_ >> 32); }

  TopObjectStorage* isDocument() const override { return const_cast<TopObjectStorage*>(this); }

  void inserted(DocId);

  void notify(TrzPtr) override;

  bool connecting(DocId docId, vector<TrzPtr>&, datetime_t & current) override;

  void setTranzactions(DocId, const vector<TrzPtr> & trsz, datetime_t current) override;

  void initDocument();
  void initUserProfile(UserId);

protected:
  void applyWithoutInit(TrzPtr);

  DocId docId_ = 0;
};


class TopDocumentObject : public UnifiedObjectTempl<1>
{

protected:
  void onChange(uint16_t) override {}

public:



  /// доступен всем пользователям в Read Only режиме





};



class InvitedUser : public UnifiedObjectTempl<2>, public ObjectStorage
{
protected:
  void onChange(uint16_t) override {}
  
};

#endif 

