
#include "Object.h"
#include "ObjectStorage.h"

DefinitionRegistry<UnifiedObject::Definition> UnifiedObject::objDefs_;

// Delete this object,
// called вызывается при применении транзакции
bool UnifiedObject::changeRemove()
{
  if (0 == (state_ & osDeleted)) 
  {
    while (!linked_.empty())
    {
      const auto& lobj = linked_.front();
      const PropType pt = lobj.first;
      const bool deletable = Property::propDefs_.get(pt).deletable();
      const bool result = deletable ? lobj.second->changeRemove(pt) : lobj.second->changeRemove();
      if (!result)
        throw ErrorCode(1267);
      lobj.second->state_ |= osParentDeleted;
    }
    for (PropPtr p : props_)
      p->removeFrom(this);
    props_.clear();
    state_ = (state_ & 0x7fff) | osDeleted; 
    return true;
  }
  return false;
}

bool UnifiedObject::changeRemove(PropType pt)
{
  auto it = findPropIterator(pt);
  if (it != props_.end() && (*it)->type() == pt)
  {
    if (!(*it)->def().deletable())
      throw ErrorCode(1257);
    (*it)->removeFrom(this);
    props_.erase(it);
    state_ |= osDelProp;
    return true;
  }
  return false;
}


void UnifiedObject::change(const ObjectChanges& chgs)
{
  if (chgs.objType() == ObjTypeDeleted) 
  {
    changeRemove();
    return;
  }
  for (const PropPtr value : chgs.props()) 
  {
    auto it = findPropIterator(value->type());
    if (it != props_.end() && (*it)->type() == value->type())
    { 
      if ((*it)->def().readonly())
        throw ErrorCode(1238);
      (*it)->removeFrom(this);
      *it = value;
      value->putInto(this);
      state_ |= osChgProp;
    }
    else
    { 
      props_.push_back(value);
      resortProps_ = true;
      value->putInto(this);
      state_ |= osAddProp;
    }
  }
  for (const PropType pt : chgs.del()) 
  {
    changeRemove(pt);
  }
  for (const auto& link : linked_)
    link.second->state_ |= osParentChanged;
}


string UnifiedObject::debugString() const
{
  string str;
  for (const PropPtr& p : props_)
  {
    str += to_string(int(p->type()));
    str += ":";
    {
      JsonWriter jw;
      p->write(jw);
      str += jw.data();
    }
    if (props_.back() != p)
      str += ',';
  }
  return str;
}


void UnifiedObject::initIfChangedAndClearState()
{
  if (state_ & (~osActive))
    onChange(state_ & (~osActive));
  state_ &= osActive;
}

const Property * UnifiedObject::findProp(PropType pt) const
{
  auto it = findPropIterator(pt);
  if (it != props_.end() && (*it)->type() == pt)
    return it->get();

  return nullptr;
}

vector<PropPtr>::iterator UnifiedObject::findPropIterator(PropType pt) const
{
  if (resortProps_)
  {
    static const auto compare = [](PropPtr a, PropPtr b)
    {
      return static_cast<int>(a->type()) < static_cast<int>(b->type());
    };
    resortProps_ = false;
    std::sort(props_.begin(), props_.end(), compare);
  }
  static const auto compare2 = [](PropPtr value, PropType pt)
  {
    return static_cast<int>(value->type()) < static_cast<int>(pt);
  };
  return lower_bound(props_.begin(), props_.end(), pt, compare2);
}

LongName UnifiedObject::LName() const
{
  LongName res;
  res.push_back(name_);
  ObjectStorage* s = &storage();
  for (;;)
  {
    if (UnifiedObject* obj = s->isObject())
    {
      res.push_back(obj->name_);
      s = &obj->storage();
    }
    else
    {
      reverse(res.begin(), res.end());
      return res;
    }
  }
}


ObjectChanges::ObjectChanges(ObjType type, ObjectStorage & storage)
{
  objType_ = type;
  if (UnifiedObject * obj = storage.isObject())
    name_ = obj->LName();
  name_.push_back(storage.reserveName());
}

ObjectChanges::ObjectChanges(const UnifiedObject & obj)
{
  objType_ = ObjTypeUnchanged;
  name_ = obj.LName();
}

ObjectChanges::ObjectChanges(const LongName & name)
{
  objType_ = ObjTypeUnchanged;
  name_ = name;
}

ObjectChanges::ObjectChanges(Reader & r)
{
  r.getVector(name_);
  objType_ = static_cast<ObjType>(r.getInt());

  r.getVector(del_); 

  size_t count = r.getMap(); 
  props_.reserve(count);
  while (count--)
  {
    PropType type = static_cast<PropType>(r.getInt());

    props_.emplace_back(Property::propDefs_.get(type).creator_(r));
  }
}

void ObjectChanges::write(Writer & w) const
{

  w.putVector(objName());   

  w.save(objType());        

  w.putVector(del_);        

  w.putMap(props_.size());  
  for (PropPtr pp : props_)
  {
    w.save(pp->type());
    pp->write(w);
  }
}

void ObjectChanges::merge(const ObjectChanges * other)
{
  ASSERT(name_ == other->objName());

  if (other->objType_ == ObjTypeDeleted)
  {
    ASSERT(other->props_.empty() && other->del_.empty());
    props_.clear();
    del_.clear();
    objType_ = ObjTypeDeleted;
    return;
  }

  if (other->objType_ != ObjTypeUnchanged)
  {
    throw ErrorCode(1254);
  }

  for (PropType pt : other->del_)
  {
    auto it = find_if(props_.begin(), props_.end(), [&](PropPtr p) { return p->type() == pt; });
    if (it != props_.end())
      props_.erase(it);
  }

  for (const PropPtr & opp : other->props_)
  {
    {
      auto it = find(del_.begin(), del_.end(), opp->type());
      if (it != del_.end())
        del_.erase(it);
    }
    {
      auto it = find_if(props_.begin(), props_.end(), [&](PropPtr p) { return p->type() == opp->type(); });
      if (it!=props_.end())
      {
        *it = opp; 
      }
      else
      {
        props_.push_back(opp); 
      }
    }
  }

  del_.insert(del_.end(), other->del_.begin(), other->del_.end());
  sort(del_.begin(), del_.end());
  del_.erase(unique(del_.begin(), del_.end()), del_.end());
}


ObjectChanges & ObjectChanges::prop(Property * pv)
{
  if (objType_ == ObjTypeDeleted)
  {
    delete pv;
    return *this;
  }


  for (auto it = props_.begin(); it != props_.end(); ++it)
    if ((*it)->type() == pv->type())
    {
      props_.erase(it);
      break;
    }

  props_.push_back(PropPtr(pv));

  auto it = find(del_.begin(), del_.end(), pv->type());
  if (it != del_.end())
    del_.erase(it);
  return *this;
}

ObjectChanges & ObjectChanges::remove(PropType pt)
{
  if (objType_ == ObjTypeDeleted)
    return *this;

  del_.push_back(pt);

  auto it = find_if(props_.begin(), props_.end(), [&](PropPtr p) { return p->type() == pt; });
  if (it != props_.end())
    props_.erase(it);

  return *this;
}

void ObjectChanges::remove()
{

  objType_ = ObjTypeDeleted;
  props_.clear();
  del_.clear();
}


bool ObjectChanges::isFirstObject() const
{
  return objName().size() == 1 && objName()[0] == 1;
}

PropPtr ObjectChanges::findProp(PropType pt) const
{
  auto it = find_if(props_.begin(), props_.end(), [&](PropPtr p) {return pt == p->type(); });
  if (it != props_.end())
    return *it;
  return PropPtr();
}







UnifiedObject* LinkToObject::findLinkedObject(const UnifiedObject* propertyOwner) const
{
  if (!propertyOwner || name_.empty())
    return nullptr;

  auto nameLength = upStor_;
  ObjectStorage* stor = &propertyOwner->storage();
  while (stor && nameLength--)
  {
    stor = stor->storage();
  }
  if (!stor)
    return nullptr;

  return stor->findObject(name_);
}

