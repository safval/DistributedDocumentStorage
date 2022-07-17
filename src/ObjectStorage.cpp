
#include "ObjectStorage.h"
#include "TranzactionStorage.h"



ObjectStorage::~ObjectStorage()
{
  clear();
}

void ObjectStorage::clear()
{
  for (UnifiedObject * obj : objects_)
    delete obj;
  objects_.clear();
}

vector<UnifiedObject*>::const_iterator ObjectStorage::findObjectIterator(ObjName objName) const
{

  static const auto compare2 = [](const UnifiedObject * obj, ObjName name)
  {
    return name > obj->name();
  };

  return std::lower_bound(objects_.begin(), objects_.end(), objName, compare2);
}

UnifiedObject * ObjectStorage::findObjectByName(ObjName objName) const
{
  auto it = findObjectIterator(objName);
  if (it == objects_.end() || (*it)->name() != objName || !(*it)->isActive())
    return nullptr;
  return *it;
}

UnifiedObject * ObjectStorage::findObjectByType(ObjType objType) const
{
  for (UnifiedObject *obj : objects_)
    if (obj->type() == objType && obj->isActive())
      return obj;
  return nullptr;
}


ObjectStorage* ObjectStorage::findStorage(ObjName name) const
{
  if (UnifiedObject* obj = findObjectByName(name))
    return obj->isStorage();
  return nullptr;
}

ObjectStorage * ObjectStorage::findStorageOf(const LongName & name) const
{
  ObjectStorage * str = const_cast<ObjectStorage*>(this);
  int i = (int)name.size() - 1;
  auto nameIt = name.begin();
  while (i--)
  {
    ObjName n = *nameIt;
    UnifiedObject * obj = str->findObjectByName(n);
    if (!obj)
      return nullptr;
    str = obj->isStorage();
    if (!str)
      return nullptr;
    ++nameIt;
  }
  return str;
}

UnifiedObject * ObjectStorage::findObject(const LongName & name) const
{
  if (name.empty())
    return nullptr;

  ObjectStorage * str = const_cast<ObjectStorage*>(this);
  int i = (int)name.size() - 1;
  auto nameIt = name.begin();
  while (i--)
  {
    ObjName n = *nameIt;
    UnifiedObject * obj = str->findObjectByName(n);
    if (!obj)
      return nullptr;
    str = obj->isStorage();
    if (!str)
      return nullptr;
    ++nameIt;
  }
  return str->findObjectByName(*nameIt);
}


string ObjectStorage::debugString() const
{
  string res;
  for (const UnifiedObject * obj : objects_)
  {
    if (obj->isActive())
    {
      const string & nameType = to_string(int(obj->type())) + "#" + to_string(obj->name());
      const string & props = obj->debugString(); 
      res += nameType + "[" + props;
      if (ObjectStorage * str = obj->isStorage())
      {
        const string & nestedObjects = str->debugString();
        if (!props.empty() && !nestedObjects.empty())
          res += ',';
        res += nestedObjects;
      }
      res += "]";
    }
  }
  return res;


}



void ObjectStorage::save(Writer& w, bool onlySelected) const
{
  w.putMap(size(false));

  string res;
  for (const UnifiedObject * obj : objects())
  {
    if (obj->isActive())
    {

      w.putInt(obj->name());
      
      ObjectStorage* objStorage = obj->isStorage();
      w.putMap(1 + obj->propertyCount() + !!objStorage);
      
      w.putStr("type");
      w.putInt(obj->type());

      for (const PropPtr& p : obj->props())
      {
        w.putStr(p->def().name_);
        p->write(w);
      }

      if (objStorage)
      {
        w.putStr("children");
        objStorage->save(w, onlySelected);
      }

    }
  }
}




size_t ObjectStorage::size(bool withChildren) const noexcept
{
  size_t count = 0;
  for (const UnifiedObject * obj : objects_)
  {
    if (obj->isActive())
    {
      count ++;
      if (withChildren)
        if (ObjectStorage * storage = obj->isStorage())
          count += storage->size(withChildren);
    }
  }
  return count;
}





UnifiedObject * ObjectStorage::create(ObjType type, ObjName name)
{
  UnifiedObject * obj = nullptr;
  ASSERT(name != 0);

  auto it = findObjectIterator(name);
  if (it != objects_.end() && (*it)->name() == name)
  {
    obj = *it;
    if (obj->isActive())
      throw ErrorCode(1250);
    obj->state_ = osActive | osCreated;
    return obj;
  }

  if (name >= nextName_)
    nextName_ = name + 1;

  obj = UnifiedObject::objDefs_.get(type).creator_();

  ASSERT(name > 0);
  obj->name_ = name;
  obj->storage_ = this;
  obj->state_ = osActive | osCreated;

  if (obj->def().isTopObject() && name != 1)
    throw ErrorCode(1253);

  objects_.push_back(obj);
  return obj;
}

void TopObjectStorage::setTranzactions(DocId docId, const vector<TrzPtr> & trzs, datetime_t current)
{
  clear();

  for (TrzPtr trz : trzs)
  {
    if (trz->created() <= current && trz->enabled())
      applyWithoutInit(trz);
  }


  for (UnifiedObject& obj : objects(true))
    obj.initIfChangedAndClearState();
}

bool TopObjectStorage::connecting(DocId docId, vector<TrzPtr> & trzs, datetime_t & current)
{
  docId_ = docId;
  setTranzactions(docId_, trzs, current);
  return false; 
}


void TopObjectStorage::applyWithoutInit(TrzPtr trz)
{

  sort(trz->changes_.begin(), trz->changes_.end(), [](const ObjectChanges* c0, const ObjectChanges* c1)
    {
      return c0->objName().size() < c1->objName().size();
    });

  for (const ObjectChanges * chgs : trz->changes_)
    if (ObjTypeUnchanged != chgs->objType() && ObjTypeDeleted != chgs->objType())
    {
      if (ObjectStorage * storage = findStorageOf(chgs->objName()))
        storage->create(chgs->objType(), chgs->objName().back());
    }

  for (const ObjectChanges * chgs : trz->changes_)
    if (UnifiedObject* obj = findObject(chgs->objName()))
      for (const PropPtr value : chgs->props())
        if (value->type() == DocIdProp::typeId_) 
          if (ObjectStorage* storage = obj->isStorage())
            if (TopObjectStorage* document = storage->isDocument()) 
              document->inserted(static_pointer_cast<DocIdProp>(value)->value());

  for (const ObjectChanges * chgs : trz->changes_)
    if (UnifiedObject * obj = findObject(chgs->objName())) 
      obj->change(*chgs);
}


void TopObjectStorage::inserted(DocId docId)
{
  ObjectStorage* stor = storage();
  while (true)
  {
    if (UnifiedObject* obj = stor->isObject())
    {
      if (const DocIdProp* dip = obj->findProp<DocIdProp>())
        if (dip->value() == docId)
          throw ErrorCode(SelfInsertedDocument); 
      stor = &obj->storage();
    }
    else
    {
      if (TopObjectStorage* doc = stor->isDocument()) 
        if (doc->docId() == docId)
          throw ErrorCode(SelfInsertedDocument);
      break;
    }
  }

  TrzHub* hub = TrzHub::opened(docId);
  if (!hub)
  {
  }
  if (hub)
  {
    hub->connect(this);
  }
}


void TopObjectStorage::notify(TrzPtr trz)
{
  if (!trz->enabled())
    return;

  applyWithoutInit(trz);


  for (UnifiedObject& obj : objects(true))
    obj.initIfChangedAndClearState();




///  initObjects(objs);
}


