
#ifndef TRANZACTION_H20180606
#define TRANZACTION_H20180606

#include "Object.h"

struct TrzEnabler
{
  TrzEnabler(const LongName &n, DocVariantEnum s) : name_(n), state_(s) {}
  const LongName name_;
  DocVariantEnum state_;
};


class Tranzaction
{
public:

  Tranzaction(const LongName & source = LongName());

  Tranzaction(Reader&);

  void write(Writer&) const;


  LongName source_;
  TrzEnabler * enabler_ = nullptr;
  bool enabled() const { return enabler_ ? (enabler_->state_ != DocVariantEnum::Disabled) : true; }




  ObjectChanges & createObject(ObjType, ObjectStorage &);
  ObjectChanges & changeObject(const LongName&);
  ObjectChanges & changeObject(UnifiedObject&);
  ObjectChanges & changeObject(ObjName);


  vector<ObjectChanges*> changes_;


  inline const LongName & source() const { return source_; }
  inline datetime_t created() const { return created_; }

  void merge(const shared_ptr<Tranzaction>);

  inline void commit() { active_ = false; }

  inline bool active() const { return active_; }

private:

  static datetime_t currentCreated();

  bool active_;

  datetime_t created_;

  void makeCorrect();

  Tranzaction(const Tranzaction&) = delete;
  void operator=(const Tranzaction&) = delete;
};

using TrzPtr = shared_ptr<Tranzaction>;


class TrzHub;
class TrzIO
{
public:


  virtual bool connecting(DocId docId, vector<TrzPtr> & trzs, datetime_t & current) = 0;

  virtual void setTranzactions(DocId, const vector<TrzPtr> & trsz, datetime_t current) = 0;

  virtual void notify(TrzPtr) = 0;

  inline TrzHub * hub() { return hub_; }

  virtual ~TrzIO();
private:
  friend class TrzHub; 
  TrzHub * hub_ = nullptr;
};


enum class TrzFilter
{
  DocInfo,   
  Strings,   
  Symbols,   
  All        
};

















#endif 

