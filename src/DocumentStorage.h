
#ifndef DOCUMENT_STORAGE_H
#define DOCUMENT_STORAGE_H




#include "Tranzaction.h"
class TranzactionStorage;

class DocumentStorage
{
public:



  enum class Access 
  {
    None,      
    User,      
    Admin      
  };


  virtual vector<DocId> list() const = 0;

  virtual TranzactionStorage * open(DocId, TrzFilter) = 0;

  virtual void remove(DocId) = 0;

  static void addStandardTypes();
};

class DocStorageInfo : public UnifiedObjectTempl<241>
{
  string title_;
  string comment_;

  bool multiUser() const;

  DeviceId id() const;

  bool forPrivateProfile() const;


  vector<string> tags_;  
};

class LocalDocumentStorage : public DocumentStorage
{
public:
  LocalDocumentStorage(const filesystem::path& dir) : dir_(dir) { }

  vector<DocId> list() const override;
  [[nodiscard]] TranzactionStorage * open(DocId, TrzFilter) override;
  void remove(DocId) override;

protected:
  const filesystem::path dir_;
};

class RemoteDocumentStorage : public DocumentStorage
{
public:
};


class ServerDocumentStorage
{
public:
  ServerDocumentStorage(LocalDocumentStorage&) {}
};

#endif 

