
#include "DocumentStorage.h"
#include "TranzactionStorage.h"
#include "ObjectStorage.h"


vector<DocId> LocalDocumentStorage::list() const 
{
  vector<DocId> docIds;
  for (const auto& entry : filesystem::directory_iterator(dir_))
  {
    if (entry.is_regular_file())
    {
      string fname = entry.path().filename().string();
      auto id = strtoll(fname.c_str(), nullptr, 16);
      if (0 < id && id < 0x7fff'ffff'ffff'ffff) // overfow procection
        docIds.push_back(static_cast<DocId>(id));
    }
  }
  return docIds;
}

TranzactionStorage * LocalDocumentStorage::open(DocId docId, TrzFilter filter) 
{
  char str[24];
  sprintf(str, "%lx", docId);
  return new LocalDocumentFile(dir_ / str, filter);
}

void LocalDocumentStorage::remove(DocId docId) 
{
  char str[24];
  sprintf(str, "%lx", docId);
  filesystem::remove(dir_ / str);
}


void DocumentStorage::addStandardTypes()
{
  Property::addPropertyDefinition<DocIdProp>("docId", READONLY | NO_DELETE);
  Property::addPropertyDefinition<UserIdProp>("userId", READONLY | NO_DELETE);
  Property::addPropertyDefinition<DocVariantProp>("docVariant", NO_DELETE);


  UnifiedObject::addObjectDefinition<InsertedDocument> ("insert", 0);
  UnifiedObject::addObjectDefinition<TopDocumentObject>("document", TOPOBJECT);
  UnifiedObject::addObjectDefinition<UserProfile>      ("user", TOPOBJECT);
  UnifiedObject::addObjectDefinition<InvitedUser>      ("invitedUser", 0);
  UnifiedObject::addObjectDefinition<DocStorageInfo>   ("documents", 0);
}

