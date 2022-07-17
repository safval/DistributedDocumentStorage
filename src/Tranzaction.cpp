
#include <chrono>

#include "Tranzaction.h"
#include "TranzactionStorage.h"
#include "ObjectStorage.h"


// Return current time in milliseconds.
datetime_t GetTimeInMillis() noexcept
{
  auto now = std::chrono::system_clock::now();
  auto t = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
  return t;
}


Tranzaction::Tranzaction(const LongName & source)
: source_(source),
  created_(currentCreated()),
  active_(true)
{
}

Tranzaction::Tranzaction(Reader & r)
: active_(false)
{
  size_t chgCount = r.getArray(); 
  created_ = r.getInt<datetime_t>(); 

  if (chgCount % ObjectChanges::SerialSize > 1)
    r.getVector(source_);

  chgCount = chgCount / ObjectChanges::SerialSize;
  changes_.reserve(chgCount);
  while (chgCount--)
  {
    changes_.push_back(new ObjectChanges(r));
  }
}

TrzIO::~TrzIO()
{
  if (hub_)
    hub_->disconnect(this);
}

// Tranzaction serialization
void Tranzaction::write(Writer & w) const
{
  w.putArray(changes_.size() * ObjectChanges::SerialSize + (source_.empty() ? 1 : 2)); 
  w.putInt(created_); 
  if (!source_.empty())
    w.putVector(source_); 
  for (ObjectChanges * ch : changes_)
  {
    ch->write(w);
  }
}

datetime_t Tranzaction::currentCreated()
{
  static datetime_t latest;
  datetime_t t = GetTimeInMillis();
  if (latest >= t)
    t = ++latest;
  else
    latest = t;
  return t;
}

void Tranzaction::merge(const TrzPtr other)
{
  for (ObjectChanges * och : other->changes_)
  {
    bool found = false;
    for (ObjectChanges * ch : changes_)
    {
      if (ch->objName() == och->objName())
      {
        ch->merge(och);
        found = true;
      }
    }
    if (!found)
      changes_.push_back(och);
  }
}


ObjectChanges & Tranzaction::createObject(ObjType t, ObjectStorage & s)
{
  if (ObjTypeUnchanged == t || ObjTypeDeleted == t)
    throw ErrorCode(1277);

  ObjectChanges * changes = new ObjectChanges(t, s);
  changes_.push_back(changes);
  return *changes;
}

ObjectChanges & Tranzaction::changeObject(const LongName & n)
{
  auto it = find_if(changes_.begin(), changes_.end(),
                    [&](const ObjectChanges * ch) { return ch->objName() == n; });
  if (it != changes_.end())
    return **it;

  ObjectChanges * changes = new ObjectChanges(n);
  changes_.push_back(changes);
  return *changes;
}

ObjectChanges & Tranzaction::changeObject(UnifiedObject & o)
{
  return changeObject(o.LName());
}

ObjectChanges & Tranzaction::changeObject(ObjName n)
{
  LongName ln;
  ln.push_back(n);
  return changeObject(ln);
}

void Tranzaction::makeCorrect()
{

}




