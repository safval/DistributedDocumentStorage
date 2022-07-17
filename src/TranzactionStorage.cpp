
#include "TranzactionStorage.h"
#include "Serialize.h"


bool TrzHub::updateDocumentVariants(TrzPtr trz)
{
  for (ObjectChanges * ch : trz->changes_)
  {
    if (ch->objType() == ObjTypeDeleted)
      for (TrzEnabler* te : variants_)
        if (te->name_ == ch->objName())
        {
          te->state_ = DocVariantEnum::Disabled;
          return true;
        }
  }

  bool needRecreate = false;
  for (ObjectChanges * ch : trz->changes_)
  {
    for (PropPtr p : ch->props())
      if (p->type() == DocVariantProp::typeId_)
      {
        if (!trz->source().empty())
          throw ErrorCode(1256);

        bool found = false;
        const DocVariantEnum newValue = static_cast<const DocVariantProp*>(p.get())->value();
        for (TrzEnabler* te : variants_)
        {
          if (te->name_ == ch->objName())
          {
            const DocVariantEnum oldValue = te->state_;
            if ((DocVariantEnum::Disabled == oldValue) != (newValue == DocVariantEnum::Disabled))
              needRecreate = true;
            te->state_ = newValue;
            found = true;
          }
        }
        if (!found)
          variants_.push_back(new TrzEnabler(ch->objName(), newValue));
      }
  }
  return needRecreate;
}

void TrzHub::updateAllDocumentVariants()
{
  for (TrzPtr trz : trzs_)
    trz->enabler_ = nullptr;
  clearVariants();

  for (TrzPtr trz : trzs_)
  {
    if (trz->created() <= current_)
    {
      if (trz->source().empty())
      {
        updateDocumentVariants(trz);
      }
      trz->enabler_ = enabler(trz);
    }
  }
}


TrzEnabler * TrzHub::enabler(const TrzPtr trz) const
{
  const LongName & name = trz->source();
  if (name.empty())
    return nullptr;

  for (TrzEnabler* te : variants_)
  {
    if (te->name_ == name)
      return te;
  }

  for (TrzEnabler* te : variants_)
  {
    if (te->name_ == name)
      return te;
  }

  throw ErrorCode(1255);
}

TrzHub::TrzHub(DocId docId) : docId_(docId)
{
  ASSERT(docId_ > 0);
  openedHubs_.emplace_back(this);
}

TrzHub::~TrzHub()
{
  auto it = find(openedHubs_.begin(), openedHubs_.end(), this);
  openedHubs_.erase(it);

  clearVariants();
  for (TrzIO * link : links_)
  {
    ASSERT(link->hub_ == this);
    link->hub_ = nullptr;
  }
}


vector<TrzHub*> TrzHub::openedHubs_;

TrzHub* TrzHub::opened(DocId docId)
{
  for (const auto& hub : openedHubs_)
    if (hub->docId_ == docId)
      return hub;
  return nullptr;
}



void TrzHub::notify(TrzPtr trz)
{
  if (updateDocumentVariants(trz))
  {

    trz->enabler_ = enabler(trz);
    trzs_.push_back(trz);
    current_ = trz->created();

    for (TrzIO * linked : links_) 
      linked->setTranzactions(docId_, trzs_, current_);

    return;
  }

  try
  {
    trz->enabler_ = enabler(trz);
    for (TrzIO * linked : links_)
      linked->notify(trz);

    if (current_ != trz->created())
    {
      while (!trzs_.empty() && current_ != trzs_.back()->created())
        trzs_.erase(trzs_.end() - 1);

      trzs_.push_back(trz);
      current_ = trz->created();
    }
    else
    {

    }
  }
  catch (const std::exception&) 
  {
    undoRedo(0); 
    throw;       
  }
}




void TrzHub::connect(TrzIO * storage)
{
  auto it = find(links_.begin(), links_.end(), storage);
  if (it != links_.end())
    return;

  if (storage->hub_ != nullptr)
    throw ErrorCode(1262); 
  links_.push_back(storage);
  storage->hub_ = this;

  if (storage->connecting(docId_, trzs_, current_))
  {
    updateAllDocumentVariants();

    for (TrzIO * linked : links_)
      if (storage != linked)
        linked->setTranzactions(docId_, trzs_, current_);
  }
}

void TrzHub::disconnect(TrzIO * storage)
{
  for (auto it = links_.begin(); it!=links_.end(); ++it)
  {
    if (*it == storage)
    {
      storage->hub_ = nullptr;
      links_.erase(it);
      return;
    }
  }
  throw ErrorCode(1263);
}


void TrzHub::save()
{
  {
    for (TrzIO * linked : links_)
      linked->setTranzactions(docId_, trzs_, current_);
  }
}


bool TrzHub::hasUndo() const
{
  return !trzs_.empty() && trzs_.front()->created() != current_;
}
bool TrzHub::hasRedo() const
{
  return !trzs_.empty() && trzs_.back()->created() != current_;
}


void TrzHub::undoRedo(int delta)
{

  int index;
  for (index = 0; index < (int)trzs_.size(); index ++)
    if (trzs_[index]->created() >= current_)
      break;

  index += delta;
  if (index < 0)
    throw ErrorCode(NoUndoRedoData);
  
  if (index >= (int)trzs_.size())
    throw ErrorCode(NoUndoRedoData);


  current_ = trzs_[index]->created();

  updateAllDocumentVariants();

  for (TrzIO * linked : links_)
    linked->setTranzactions(docId_, trzs_, current_);
}

datetime_t TrzHub::latest() const
{
  if (trzs_.empty())
    return 0;
  return trzs_.back()->created();
}

void TrzHub::packHistory(size_t maxCount)
{
  if (!hasRedo() && !trzs_.empty())
  {
    TrzPtr t0 = trzs_.front();
    while (trzs_.size() > maxCount)
    {
      TrzPtr t1 = trzs_[1];
      if (t0->source_ != t1->source_)
        break;                  

      t0->merge(t1);
      trzs_.erase(trzs_.begin() + 1);
    }
    current_ = trzs_.back()->created();
  }
}


void TranzactionStorage::setTranzactions(DocId, const vector<TrzPtr> & trzs, datetime_t current)
{
  Writer & writer = *createWriter();
  writer.putArray(trzs.size() + 3);  
  writer.putInt(SerializationFormatVersion);   
  writer.putInt(current);            
  writer.putMap(0); 
  for (TrzPtr t : trzs)
    t->write(writer);
  delete &writer;
}

bool TranzactionStorage::connecting(DocId docId, vector<TrzPtr> & trzs, datetime_t & current)
{
  try
  {
    unique_ptr<Reader> reader(createReader());
    size_t count = reader->getArray();
    trzs.reserve(count - 3);
    
    if (SerializationFormatVersion != reader->getInt()) 
    {
    }

    current = reader->getInt<datetime_t>(); 
    reader->getMap();
    for (size_t i = 3; i < count; i++)
      trzs.emplace_back(new Tranzaction(*reader));
  }
  catch (const ErrorCode& err) 
  {
    switch (err.code_)
    {
      case SerializationFormatError:
      case SerializationInternalError:
      case SerializationFileOpenError:
        break;
      default:
        throw;
    }
  }

  return true;
}

void TranzactionStorage::notify(TrzPtr)
{
}



LocalDocumentFile::LocalDocumentFile(const filesystem::path& path, TrzFilter filter)
: TranzactionStorage(filter),
  path_(path)
{
}

Reader* LocalDocumentFile::createReader()
{
  return new CborFileReader(path_);
}

Writer * LocalDocumentFile::createWriter()
{
  return new CborFileWriter(path_);
}



Reader * InMemoryTrzStorage::createReader()
{
  return new MSReader(data_);
}

Writer * InMemoryTrzStorage::createWriter()
{
  data_.clear(); 
  return new MSWriter(data_);
}


