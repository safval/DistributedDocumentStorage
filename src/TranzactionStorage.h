
#ifndef TRANZACTION_STORAGE_H20190207
#define TRANZACTION_STORAGE_H20190207

#include "Tranzaction.h"

class TranzactionStorage : public TrzIO
{
public:
  TranzactionStorage(TrzFilter filter = TrzFilter::All) : trzFilter_(filter) {}

  bool connecting(DocId docId, vector<TrzPtr>&, datetime_t & current) override;
  
  void setTranzactions(DocId, const vector<TrzPtr> & trsz, datetime_t current) override;

  void notify(TrzPtr) override;

  enum class Access 
  {
    Owner,     
    Full,      
    ReadWrite, 
    ReadOnly   
  };
  virtual Access access() const { return Access::Owner; }

protected:

  [[nodiscard]] virtual Reader * createReader() = 0;  
  [[nodiscard]] virtual Writer * createWriter() = 0;

  const TrzFilter trzFilter_;

  datetime_t saved_ = 0;
};

class LocalDocumentFile : public TranzactionStorage
{
public:
  LocalDocumentFile(const filesystem::path&, TrzFilter);
  [[nodiscard]] Reader* createReader() override;
  [[nodiscard]] Writer* createWriter() override;
protected:
  const filesystem::path path_;
};

class InMemoryTrzStorage : public TranzactionStorage
{
public:
  [[nodiscard]] Reader* createReader() override;
  [[nodiscard]] Writer* createWriter() override;
protected:
  MSDataT data_;
};


class DocumentOnServer : public TranzactionStorage
{
};



class TrzHub
{
public:
  TrzHub(DocId);

  ~TrzHub();

  inline DocId id() const { return docId_; }

  void notify(TrzPtr);

  void connect(TrzIO*);
  void disconnect(TrzIO*);

  void save();

  bool hasUndo() const;
  bool hasRedo() const;


  void undoRedo(int delta);

  datetime_t latest() const; 
  datetime_t current() const { return current_; }

  inline size_t trzCount() const { return trzs_.size(); }
  inline size_t linkCount() const { return links_.size(); }

  void packHistory(size_t maxCount);

  static TrzHub* opened(DocId);

private:

  static vector<TrzHub*> openedHubs_;

  bool updateDocumentVariants(TrzPtr);
  void updateAllDocumentVariants();
  TrzEnabler * enabler(const TrzPtr trz) const;

  vector<TrzIO*> links_;

  vector<TrzPtr> trzs_;

  datetime_t current_ = 0;

  const DocId docId_;

  vector<TrzEnabler*> variants_;

  void inline clearVariants()
  {
    for (TrzEnabler* te : variants_)
      delete te;
    variants_.clear();
  }
private:
  TrzHub(const TrzHub&) = delete;
  void operator=(const TrzHub&) = delete;
};

#endif 

