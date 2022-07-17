
#include "gtest/gtest.h"

#include "DocumentStorage.h"
#include "ObjectStorage.h"
#include "TranzactionStorage.h"

using TestPropInt1 = PropValueTemplate<551, int>;
using TestPropInt2 = PropValueTemplate<552, int>;
using TestPropInt3 = PropValueTemplate<553, int>;
using TestPropLink = PropValueTemplate<554, LinkToObject>;
using TestPropIntReadOnly = PropValueTemplate<997, int>;
using TestPropIntNoDelete = PropValueTemplate<996, int>;

using TreeFolderProp = PropValueTemplate<99, LinkToObject>;


class InitCounter
{
public:
  inline size_t initCount() const { return counter_; }
  uint16_t lastChanges_ = 0; 
protected:
  size_t counter_ = 0;
};

class TestTopObject : public UnifiedObjectTempl<500>, public InitCounter
{
protected:
  void onChange(uint16_t flags) override { counter_++; lastChanges_ = flags; }
};

class TestObject1 : public UnifiedObjectTempl<501>, public InitCounter
{
protected:
  void onChange(uint16_t flags) override { counter_++; lastChanges_ = flags; }
};

class TestObject2 : public UnifiedObjectTempl<502>, public InitCounter
{
protected:
  void onChange(uint16_t flags) override { counter_++; lastChanges_ = flags; }
};

class TestObjectStorage1 : public UnifiedObjectTempl<551>, public ObjectStorage, public InitCounter
{
public:
  ObjectStorage * isStorage() const override { return const_cast<TestObjectStorage1*>(this); }
  UnifiedObject * isObject() const override { return const_cast<TestObjectStorage1*>(this); }
protected:
  void onChange(uint16_t flags) override { counter_++; lastChanges_ = flags; }
};

class TestObjectStorage2 : public UnifiedObjectTempl<552>, public ObjectStorage
{
public:
  ObjectStorage * isStorage() const override { return const_cast<TestObjectStorage2*>(this); }
  UnifiedObject * isObject() const override { return const_cast<TestObjectStorage2*>(this); }
protected:
  void onChange(uint16_t flags) override
  {
    if (findProp<TestPropInt3>())
      throw exception(); 
  }
};










class TrzHub;      
class TestInsertedDocument : public UnifiedObjectTempl<553>, public TopObjectStorage, public InitCounter
{
public:
  ObjectStorage * isStorage() const override { return const_cast<TestInsertedDocument*>(this); }
  UnifiedObject * isObject() const override { return const_cast<TestInsertedDocument*>(this); }
protected:
  void onChange(uint16_t flags) override { counter_++; lastChanges_ = flags; }
};

int main(int argc, char** argv)
{
  DocumentStorage::addStandardTypes();

  Property::addPropertyDefinition<TestPropInt1>("TestPropInt1", 0);
  Property::addPropertyDefinition<TestPropInt2>("TestPropInt2", 0);
  Property::addPropertyDefinition<TestPropInt3>("TestPropInt3", 0);
  Property::addPropertyDefinition<TestPropLink>("TestPropLink", 0);
  Property::addPropertyDefinition<TestPropIntReadOnly>("TestPropIntReadOnly", READONLY);
  Property::addPropertyDefinition<TestPropIntNoDelete>("TestPropIntNoDelete", NO_DELETE);
  Property::addPropertyDefinition<TreeFolderProp>("TreeFolderProp", 0);

  UnifiedObject::addObjectDefinition<TestTopObject>("TestTopObject", TOPOBJECT);
  UnifiedObject::addObjectDefinition<TestObject1>("TestObject1", 0);
  UnifiedObject::addObjectDefinition<TestObject2>("TestObject2", 0);
  UnifiedObject::addObjectDefinition<TestObjectStorage1>("TestObjectStorage1", 0);
  UnifiedObject::addObjectDefinition<TestObjectStorage2>("TestObjectStorage2", 0);
  UnifiedObject::addObjectDefinition<TestInsertedDocument>("TestInsertedDocument", 0);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}





TEST(Property, StandartTypes)
{
  EXPECT_EQ(Property::propDefs_.get(UserIdProp::typeId_).name_, "userId");
  EXPECT_TRUE(Property::propDefs_.get(DocIdProp::typeId_).readonly());
  EXPECT_THROW(Property::addPropertyDefinition<TestPropInt1>("prop", 0), std::exception);
  EXPECT_THROW(Property::addPropertyDefinition<TestPropInt1>("userId", 0), std::exception);

  EXPECT_EQ(UnifiedObject::objDefs_.get(UserProfile::typeId_).name_, "user");
  EXPECT_THROW(UnifiedObject::addObjectDefinition<TopDocumentObject>("objName", 0), std::exception);
  EXPECT_THROW(UnifiedObject::addObjectDefinition<TopDocumentObject>("user", 0), std::exception);
}

TEST(Property, Registration)
{
}


TEST(Property, Edit)
{
  TopObjectStorage doc;
  TestTopObject* topObj;
  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc);
    doc.notify(trz);

    ASSERT_EQ(doc.size(true), 1);
    topObj = doc.findObject<TestTopObject>();
    ASSERT_TRUE(topObj != nullptr);
    EXPECT_EQ(topObj->propertyCount(), 0);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[]");
    EXPECT_EQ(topObj->lastChanges_, osCreated);
    EXPECT_TRUE(topObj->isActive());
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).prop(new TestPropInt1(0));
    doc.notify(trz);

    ASSERT_EQ(doc.size(true), 1);
    EXPECT_EQ(doc.findObjectByName(1), topObj);
    ASSERT_TRUE(topObj != nullptr);
    ASSERT_EQ(topObj->propertyCount(), 1);
    EXPECT_EQ(topObj->findProp<TestPropInt1>()->value(), 0);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:0]");
    EXPECT_EQ(topObj->lastChanges_, osAddProp);
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).prop(new TestPropInt1(100));
    doc.notify(trz);

    ASSERT_EQ(doc.size(true), 1);
    ASSERT_EQ(topObj->propertyCount(), 1);
    EXPECT_EQ(topObj->findProp<TestPropInt1>()->value(), 100);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:100]");
    EXPECT_EQ(topObj->lastChanges_, osChgProp);
  }

  { 
    TrzPtr trz(new Tranzaction());
    ObjectChanges & chgs = trz->changeObject(1);
    chgs.remove(TestPropInt1::typeId_);
    doc.notify(trz);

    ASSERT_EQ(doc.size(true), 1);
    EXPECT_EQ(topObj->propertyCount(), 0);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[]");
    EXPECT_EQ(topObj->lastChanges_, osDelProp);
  }

  { 
    TrzPtr trz(new Tranzaction());
    auto c1 = &trz->changeObject(1);
    auto c2 = &trz->changeObject(1);
    EXPECT_EQ(c1, c2);
    c1->prop(new TestPropInt1(200));
    c2->prop(new TestPropInt2(300));
    doc.notify(trz);

    ASSERT_EQ(doc.size(true), 1);
    ASSERT_EQ(topObj->propertyCount(), 2);
    EXPECT_EQ(topObj->findProp<TestPropInt1>()->value(), 200);
    EXPECT_EQ(topObj->findProp<TestPropInt2>()->value(), 300);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:200,552:300]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    ObjectChanges & chgs = trz->changeObject(1)
      .prop(new TestPropInt1(201))
      .remove(TestPropInt1::typeId_)
      .remove(TestPropInt2::typeId_)
      .prop(new TestPropInt2(301));
    doc.notify(trz);

    ASSERT_EQ(doc.size(true), 1);
    ASSERT_EQ(topObj->propertyCount(), 1);
    EXPECT_EQ(topObj->findProp<TestPropInt2>()->value(), 301);
    EXPECT_EQ(topObj->lastChanges_, osChgProp | osDelProp);
  }

  { 
    TrzPtr trz(new Tranzaction());
    ObjectChanges & chgs = trz->changeObject(1);

    chgs.prop(new TestPropInt2(101));
    doc.notify(trz);
    EXPECT_EQ(topObj->findProp<TestPropInt2>()->value(), 101);
    EXPECT_EQ(topObj->lastChanges_, osChgProp);

    chgs.prop(new TestPropInt2(201));
    doc.notify(trz);
    EXPECT_EQ(topObj->findProp<TestPropInt2>()->value(), 201);
    EXPECT_EQ(topObj->lastChanges_, osChgProp);

    chgs.prop(new TestPropInt2(1001));
    chgs.prop(new TestPropInt2(301));
    doc.notify(trz);
    EXPECT_EQ(topObj->findProp<TestPropInt2>()->value(), 301);
    EXPECT_EQ(topObj->lastChanges_, osChgProp);
  }
}

TEST(Property, Restrictions)
{
  TopObjectStorage doc;
  TestTopObject* topObj;
  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc).
      prop(new TestPropIntReadOnly(1)).
      prop(new TestPropIntNoDelete(1));
    doc.notify(trz);
    topObj = doc.findObject<TestTopObject>();
    ASSERT_TRUE(topObj);
    EXPECT_EQ(topObj->lastChanges_, osCreated | osAddProp);
    topObj->lastChanges_ = 0;
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).remove(TestPropIntNoDelete::typeId_);
    EXPECT_THROW(doc.notify(trz), exception);
    EXPECT_EQ(topObj->lastChanges_, 0);
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).prop(new TestPropIntReadOnly(222));
    EXPECT_THROW(doc.notify(trz), exception);
    EXPECT_EQ(topObj->lastChanges_, 0);
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).remove(TestPropIntNoDelete::typeId_).prop(new TestPropIntNoDelete(2));
    doc.notify(trz);
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).remove(TestPropIntReadOnly::typeId_).prop(new TestPropIntReadOnly(222));
    EXPECT_THROW(doc.notify(trz), exception);
  }

  { 
    TrzPtr trz1(new Tranzaction());
    trz1->changeObject(1).remove(TestPropIntReadOnly::typeId_);
    doc.notify(trz1);

    TrzPtr trz2(new Tranzaction());
    trz2->changeObject(1).prop(new TestPropIntReadOnly(2));
    doc.notify(trz2);
  }
}

TEST(Objects, CreateDelete)
{
  TopObjectStorage doc;
  EXPECT_FALSE(doc.isObject());

  { 
    TrzPtr trz(new Tranzaction());
    doc.notify(trz);
    ASSERT_EQ(doc.size(true), 0);
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc);
    doc.notify(trz);

    ASSERT_EQ(doc.size(true), 1);
    UnifiedObject * obj = doc.findObjectByName(1);
    ASSERT_TRUE(obj != nullptr);
    EXPECT_TRUE(obj->isActive());
    EXPECT_EQ(obj->propertyCount(), 0);
    EXPECT_EQ(static_cast<TestTopObject*>(obj)->initCount(), 1);
  }

  { 
    TrzPtr trz(new Tranzaction());
    doc.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestObject1::typeId_, doc);
    trz->createObject(TestObject1::typeId_, doc);
    trz->createObject(TestObject2::typeId_, doc);
    doc.notify(trz);

    EXPECT_EQ(doc.size(true), 4);
    EXPECT_NE(doc.findObjectByName(1), nullptr);
    EXPECT_NE(doc.findObjectByName(2), nullptr);
    EXPECT_NE(doc.findObjectByName(3), nullptr);
    EXPECT_NE(doc.findObjectByName(4), nullptr);
  }

  { 
    auto objToDelete = doc.findObjectByName(4);
    EXPECT_TRUE(objToDelete->isActive());

    TrzPtr trz(new Tranzaction());
    trz->createObject(TestObjectStorage1::typeId_, doc);
    trz->changeObject(4).remove();
    doc.notify(trz);

    EXPECT_EQ(doc.size(true), 4);
    EXPECT_NE(doc.findObjectByName(1), nullptr);
    EXPECT_NE(doc.findObjectByName(2), nullptr);
    EXPECT_NE(doc.findObjectByName(3), nullptr);
    EXPECT_EQ(doc.findObjectByName(4), nullptr);
    ASSERT_NE(doc.findObjectByName(5), nullptr); 

    EXPECT_EQ(doc.findObjectByName(5)->type(), TestObjectStorage1::typeId_);
    EXPECT_NE(doc.findObjectByName(5)->isStorage(), nullptr);
    EXPECT_FALSE(objToDelete->isActive());
  }

  { 
    TrzPtr trz(new Tranzaction());
    ObjectStorage & storage2 = *doc.findObjectByName(5)->isStorage();
    trz->createObject(TestObject1::typeId_, storage2);
    doc.notify(trz);

    EXPECT_EQ(doc.size(false), 4);
    EXPECT_EQ(doc.size(true), 5);
  }

  { 
    TrzPtr trz(new Tranzaction());
    ObjectStorage & storage2 = *doc.findObjectByName(5)->isStorage();

    auto & tc = trz->createObject(TestObjectStorage1::typeId_, storage2);
    doc.notify(trz);

    EXPECT_EQ(doc.size(false), 4);
    EXPECT_EQ(doc.size(true), 6);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[]501#2[]501#3[]551#5[501#1[]551#2[]]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    ObjectStorage * storage2 = doc.findObjectByName(5)->isStorage();
    ASSERT_NE(storage2, nullptr);
    ObjectStorage * storage3 = storage2->findObjectByName(2)->isStorage();
    ASSERT_NE(storage3, nullptr);
    trz->createObject(TestObject2::typeId_, *storage3);
    trz->createObject(TestObject2::typeId_, *storage3);
    trz->createObject(TestObject2::typeId_, *storage3);
    doc.notify(trz);

    EXPECT_EQ(doc.size(false), 4);
    EXPECT_EQ(doc.size(true), 9);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[]501#2[]501#3[]551#5[501#1[]551#2[502#1[]502#2[]502#3[]]]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject({ 5,2,2 }).remove();
    doc.notify(trz);

    EXPECT_STREQ(doc.debugString().c_str(), "500#1[]501#2[]501#3[]551#5[501#1[]551#2[502#1[]502#3[]]]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(4).remove();
    trz->changeObject({ 5,2,2 }).remove();
    trz->changeObject({ 2,2 }).remove();
    trz->changeObject({ 9999,2 }).remove();
    doc.notify(trz);

    EXPECT_STREQ(doc.debugString().c_str(), "500#1[]501#2[]501#3[]551#5[501#1[]551#2[502#1[]502#3[]]]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(5).remove();
    trz->changeObject({ 5,2,1 }).remove();
    trz->changeObject(5).prop(new TestPropInt1(0)); 
    doc.notify(trz);

    trz->changeObject(3).remove();
    doc.notify(trz);

    EXPECT_EQ(doc.size(false), 2);
    EXPECT_EQ(doc.size(true), 2);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[]501#2[]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    auto & tc = trz->createObject(TestObject2::typeId_, doc);
    doc.notify(trz);

    EXPECT_STREQ(doc.debugString().c_str(), "500#1[]501#2[]502#6[]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestObject1::typeId_, doc).remove();
    doc.notify(trz);

    EXPECT_STREQ(doc.debugString().c_str(), "500#1[]501#2[]502#6[]");
  }
}

TEST(PropertyLink, AutodeleteProperty)
{
  TopObjectStorage doc;
  {
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc);
    trz->createObject(TestObject1::typeId_, doc).prop(new TreeFolderProp(0, { 1 }));
    doc.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[]501#2[99:[0,1]]");
  }

  ASSERT_TRUE(Property::propDefs_.get(TreeFolderProp::typeId_).deletable());

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).remove();
    doc.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "501#2[]");
  }
}


TEST(PropertyLink, TwoObjects)
{
  TopObjectStorage doc;
  {
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc);
    trz->createObject(TestObject1::typeId_, doc).prop(new TreeFolderProp(0, { 1 }));
    doc.notify(trz);
  }
  TestTopObject* topObj = doc.findObject<TestTopObject>();
  TestObject1* testObj = doc.findObject<TestObject1>();

  EXPECT_EQ(topObj->lastChanges_, osCreated | osAddLink);
  EXPECT_EQ(testObj->lastChanges_, osCreated | osAddProp);

  {
    TrzPtr trz(new Tranzaction());
    trz->changeObject(*topObj).prop(new TestPropInt1(1));
    doc.notify(trz);
  }

  EXPECT_EQ(topObj->lastChanges_, osAddProp);
  EXPECT_EQ(testObj->lastChanges_, osParentChanged);

  {
    TrzPtr trz(new Tranzaction());
    trz->changeObject(*testObj).remove(TreeFolderProp::typeId_);
    doc.notify(trz);
  }

  EXPECT_EQ(topObj->lastChanges_, osDelLink);
  EXPECT_EQ(testObj->lastChanges_, osDelProp);
}

TEST(PropertyLink, ThreeObjects)
{
  TopObjectStorage doc;
  {
    TrzPtr trz(new Tranzaction());  
    trz->createObject(TestTopObject::typeId_, doc);
    trz->createObject(TestObject1::typeId_, doc).prop(new TreeFolderProp(0, { 1 }));
    trz->createObject(TestObject2::typeId_, doc).prop(new TreeFolderProp(0, { 2 }));
    doc.notify(trz);
  }
  EXPECT_STREQ(doc.debugString().c_str(), "500#1[]501#2[99:[0,1]]502#3[99:[0,2]]");

  TestTopObject* obj1 = doc.findObject<TestTopObject>();
  TestObject1* obj2 = doc.findObject<TestObject1>();
  TestObject2* obj3 = doc.findObject<TestObject2>();
  EXPECT_TRUE(obj1 && obj2 && obj3);

  {
    TrzPtr trz(new Tranzaction());  
    trz->changeObject(1).prop(new TestPropInt1(10));
    obj1->lastChanges_ = 0;
    obj2->lastChanges_ = 0;
    obj3->lastChanges_ = 0;
    doc.notify(trz);
  }

  EXPECT_EQ(obj1->lastChanges_, osAddProp);       
  EXPECT_EQ(obj2->lastChanges_, osParentChanged); 
  EXPECT_EQ(obj3->lastChanges_, 0); 

  {
    TrzPtr trz(new Tranzaction());  
    trz->changeObject(2).remove();
    obj1->lastChanges_ = 0;
    obj2->lastChanges_ = 0;
    obj3->lastChanges_ = 0;
    doc.notify(trz);
  }

  EXPECT_EQ(obj1->lastChanges_, osDelLink); 
  EXPECT_EQ(obj2->lastChanges_, osDeleted | osDelLink); 
  EXPECT_EQ(obj3->lastChanges_, osParentDeleted | osDelProp); 
  EXPECT_TRUE(obj1->isActive());
  EXPECT_FALSE(obj2->isActive());
  EXPECT_TRUE(obj3->isActive());
}
/*
TEST(PropertyLink, EnumerateObject)
{
  TopObjectStorage doc;
  auto countLink = [&](PropType pt) 
  {
    size_t count = 0;
    for (auto * obj : doc.findObject<TestTopObject>()->linked(pt)) { count ++; }
    return count;
  };

  {
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc);
    trz->createObject(TestObject1::typeId_, doc);
    trz->createObject(TestObject1::typeId_, doc);
    trz->createObject(TestObject2::typeId_, doc);
    doc.notify(trz);
    EXPECT_EQ(countLink(TreeFolderProp::typeId_), 0); 
  }

  {
    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).prop(new TestPropInt1(5));
    doc.notify(trz);
    EXPECT_EQ(countLink(TreeFolderProp::typeId_), 0); 
    EXPECT_EQ(countLink(0), 0);
  }

  {
    TrzPtr trz(new Tranzaction());
    trz->changeObject(2).prop(new TreeFolderProp(0, { 1 }));
    doc.notify(trz);
    EXPECT_EQ(countLink(TreeFolderProp::typeId_), 1); 
    EXPECT_EQ(countLink(0), 1);

    auto obj1 = doc.findObjectByName(1);
    auto obj2 = doc.findObjectByName(2);
    ASSERT_TRUE(obj1);
    ASSERT_TRUE(obj2);
    EXPECT_EQ(obj2->findProp<TreeFolderProp>()->object<UnifiedObject>(obj2), obj1);
  }

}
*/

TEST(TopObjectStorage, Id)
{
  {
    TrzHub hub(20);
    TopObjectStorage doc;
    hub.connect(&doc);
    EXPECT_EQ(hub.id(), 20);
    EXPECT_EQ(doc.docId(), 20);
    EXPECT_EQ(TrzHub::opened(20), &hub);
  }
  EXPECT_FALSE(TrzHub::opened(20));
}



TEST(TrzHub, Connect)
{
  TrzHub hub(11111);
  TopObjectStorage doc1;

  EXPECT_EQ(hub.linkCount(), 0);
  EXPECT_THROW(hub.disconnect(&doc1), exception); 
  EXPECT_FALSE(doc1.hub());

  hub.connect(&doc1);
  EXPECT_EQ(hub.linkCount(), 1);
  EXPECT_EQ(doc1.hub(), &hub);
  hub.connect(&doc1);
  EXPECT_EQ(hub.linkCount(), 1); 
  EXPECT_EQ(doc1.hub(), &hub);

  {
    TopObjectStorage doc2;
    hub.connect(&doc2);
    EXPECT_EQ(hub.linkCount(), 2);
    EXPECT_EQ(doc2.hub(), &hub);
  }
  EXPECT_EQ(hub.linkCount(), 1); 

  {
    TopObjectStorage doc2;
    hub.connect(&doc2);
    EXPECT_EQ(hub.linkCount(), 2);
    hub.disconnect(&doc2);
    EXPECT_EQ(hub.linkCount(), 1);
  }
  EXPECT_EQ(hub.linkCount(), 1);

  TopObjectStorage doc3;
  {
    TrzHub hub2(11111);
    EXPECT_THROW(hub2.connect(&doc1), exception); 
    hub2.connect(&doc3);
  }
  EXPECT_FALSE(doc3.hub()); 

  {
    TopObjectStorage doc;
    TrzHub hub(11111);
    hub.connect(&doc);
  }
  {
    TrzHub hub(11111);
    TopObjectStorage doc;
    hub.connect(&doc);
  }
}


TEST(TrzHub, Synchronize)
{
  TrzHub hub(11111);

  TopObjectStorage doc1;
  hub.connect(&doc1);

  TopObjectStorage doc2;
  hub.connect(&doc2);

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc1).prop(new TestPropInt1(515));
    trz->createObject(TestObjectStorage1::typeId_, doc1).prop(new TestPropInt2(1));
    hub.notify(trz);

    EXPECT_STREQ(doc1.debugString().c_str(), "500#1[551:515]551#2[552:1]");
    EXPECT_STREQ(doc2.debugString().c_str(), "500#1[551:515]551#2[552:1]");
    EXPECT_EQ(hub.trzCount(), 1);
  }

  { 
    TrzPtr trz(new Tranzaction());
    UnifiedObject * obj = doc1.findObjectByType(TestObjectStorage1::typeId_);
    ASSERT_TRUE(obj);
    ObjectStorage * os = obj->isStorage();
    ASSERT_TRUE(os);
    trz->createObject(TestObject2::typeId_, *os).prop(new TestPropInt2(2));
    trz->createObject(TestObject1::typeId_, *os);
    hub.notify(trz);

    EXPECT_STREQ(doc1.debugString().c_str(), "500#1[551:515]551#2[552:1,502#1[552:2]501#2[]]");
    EXPECT_STREQ(doc2.debugString().c_str(), "500#1[551:515]551#2[552:1,502#1[552:2]501#2[]]");
    EXPECT_EQ(hub.trzCount(), 2);
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject({2,1}).remove();
    hub.notify(trz);

    EXPECT_STREQ(doc1.debugString().c_str(), "500#1[551:515]551#2[552:1,501#2[]]");
  }

  EXPECT_EQ(hub.trzCount(), 3);
  hub.packHistory(2);
  EXPECT_EQ(hub.trzCount(), 2);
  hub.packHistory(1);
  EXPECT_EQ(hub.trzCount(), 1);
}
/*
TEST(Tranzaction, CreateTime)
{
  TrzPtr trz1(new Tranzaction());
  TrzPtr trz2(new Tranzaction());
  EXPECT_GT(trz1->created(), 0);
  EXPECT_GT(trz2->created(), 0);
  EXPECT_NE(trz1->created(), trz2->created());
}
*/
TEST(Tranzaction, AfterPack)
{
  TrzHub hub(11111);
  TopObjectStorage doc;
  hub.connect(&doc);

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc).prop(new TestPropInt1(0));
    hub.notify(trz);
  }
  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).prop(new TestPropInt1(1));
    hub.notify(trz);
  }
  EXPECT_FALSE(hub.hasRedo());
  hub.packHistory(1);
  EXPECT_FALSE(hub.hasRedo());
  EXPECT_EQ(hub.trzCount(), 1);
}


TEST(Tranzaction, UndoRedo)
{
  TrzHub hub(11111);
  EXPECT_FALSE(hub.hasUndo());
  EXPECT_FALSE(hub.hasRedo());

  TopObjectStorage doc;
  hub.connect(&doc);

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc).prop(new TestPropInt1(0));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:0]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).prop(new TestPropInt1(1));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:1]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).prop(new TestPropInt1(2));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:2]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).prop(new TestPropInt1(3));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:3]");
    EXPECT_TRUE(hub.hasUndo());
    EXPECT_FALSE(hub.hasRedo());
  }
  
  { 
    hub.undoRedo(-1);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:2]");
    EXPECT_TRUE(hub.hasUndo());
    EXPECT_TRUE(hub.hasRedo());

    hub.undoRedo(-1);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:1]");

    while (hub.hasUndo())
      hub.undoRedo(-1);

    EXPECT_THROW(hub.undoRedo(-1), exception);

    while (hub.hasRedo())
      hub.undoRedo(1);

    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:3]");

    EXPECT_THROW(hub.undoRedo(1), exception);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:3]");
  }

  { 
    hub.undoRedo(-1);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:2]");
    EXPECT_TRUE(hub.hasUndo());
    EXPECT_TRUE(hub.hasRedo());

    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).prop(new TestPropInt1(4));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:4]");
    EXPECT_TRUE(hub.hasUndo());
    EXPECT_FALSE(hub.hasRedo());

    hub.undoRedo(-1);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:2]");
    EXPECT_TRUE(hub.hasUndo());
    EXPECT_TRUE(hub.hasRedo());
  }
}

TEST(Tranzaction, Revert)
{
  TrzHub hub(11111);
  TopObjectStorage doc;
  hub.connect(&doc);

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc).prop(new TestPropInt2(1));
    hub.notify(trz);

    EXPECT_STREQ(doc.debugString().c_str(), "500#1[552:1]");
  }
  { 
    TrzPtr trz(new Tranzaction());

    trz->changeObject(1).prop(new TestPropInt1(0));
    trz->createObject(TestObjectStorage2::typeId_, doc).prop(new TestPropInt3(2));

    EXPECT_THROW(hub.notify(trz), exception);

    EXPECT_STREQ(doc.debugString().c_str(), "500#1[552:1]");
  }
}

TEST(Tranzaction, ReApply)
{
  TrzHub hub(11111);

  TopObjectStorage doc;
  hub.connect(&doc);

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc).prop(new TestPropInt2(1));
    trz->createObject(TestObject2::typeId_, doc).prop(new TestPropInt1(1));;
    hub.notify(trz);

    EXPECT_STREQ(doc.debugString().c_str(), "500#1[552:1]502#2[551:1]");
    EXPECT_EQ(hub.trzCount(), 1);
  }

  { 
    TrzPtr trz(new Tranzaction());

    trz->changeObject(2).prop(new TestPropInt1(2));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[552:1]502#2[551:2]");
    EXPECT_EQ(hub.trzCount(), 2);

    trz->changeObject(2).prop(new TestPropInt1(3));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[552:1]502#2[551:3]");
    EXPECT_EQ(hub.trzCount(), 2);

    trz->changeObject(2).prop(new TestPropInt1(4));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[552:1]502#2[551:4]");
    EXPECT_EQ(hub.trzCount(), 2);
  }

  { 
    hub.undoRedo(-1);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[552:1]502#2[551:1]");

    hub.undoRedo(1);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[552:1]502#2[551:4]");
  }
}

TEST(Tranzaction, Merge)
{

  auto check = [](TrzPtr t0, TrzPtr t1, const char * expectedResult)
  {
    TopObjectStorage doc0;
    doc0.notify(t0);
    doc0.notify(t1);

    TopObjectStorage doc1;
    t0->merge(t1);
    doc1.notify(t0);

    EXPECT_STREQ(doc0.debugString().c_str(), doc1.debugString().c_str());
    EXPECT_STREQ(doc0.debugString().c_str(), expectedResult);
  };

  { 
    TopObjectStorage doc;
    TrzPtr trz1(new Tranzaction());
    trz1->createObject(TestTopObject::typeId_, doc);

    TrzPtr trz2(new Tranzaction());
    trz2->changeObject(1).remove();

    check(trz1, trz2, "");
  }

  { 
    TopObjectStorage doc;
    TrzPtr trz1(new Tranzaction());
    trz1->createObject(TestTopObject::typeId_, doc);
    trz1->createObject(TestObjectStorage1::typeId_, doc);
    trz1->createObject(TestObject2::typeId_, doc);
    doc.notify(trz1);

    TrzPtr trz2(new Tranzaction());
    trz2->createObject(TestObject1::typeId_, *doc.findStorageOf({ 2,0 }));
    trz2->createObject(TestObject1::typeId_, *doc.findStorageOf({ 2,0 }));
    trz2->createObject(TestObject2::typeId_, doc);
    trz2->changeObject({ 2,22 }).remove();
    trz2->changeObject(22).remove();

    check(trz1, trz2, "500#1[]551#2[501#1[]501#2[]]502#3[]502#4[]");
  }

  { 
    TopObjectStorage doc;
    TrzPtr trz1(new Tranzaction());
    TrzPtr trz2(new Tranzaction());
    {
      trz1->createObject(TestTopObject::typeId_, doc)
        .prop(new TestPropInt1(1))
        .prop(new TestPropInt2(2));
    }
    {
      trz1->changeObject(1)
        .remove(TestPropInt2::typeId_)
        .prop(new TestPropInt3(3));
    }
    check(trz1, trz2, "500#1[551:1,553:3]");
  }

  { 
    TopObjectStorage doc;
    TrzPtr trz1(new Tranzaction());
    TrzPtr trz2(new Tranzaction());
    {
      trz1->createObject(TestTopObject::typeId_, doc)
        .prop(new TestPropInt1(1))
        .remove(TestPropInt2::typeId_)
        .remove(TestPropInt3::typeId_);
    }
    {
      trz1->changeObject(1)
        .prop(new TestPropInt1(11))
        .prop(new TestPropInt2(22))
        .remove(TestPropInt3::typeId_);
    }
    check(trz1, trz2, "500#1[551:11,552:22]");
  }
}

TEST(Tranzaction, Serialize)
{
  auto applySerialized = [](TrzPtr trz) -> string
  {
    MSDataT data;
    MSWriter * w = new MSWriter(data);
    trz->write(*w);
    Reader * r = w->getBackReader();
    TrzPtr trz2(new Tranzaction(*r));

    TopObjectStorage doc;
    doc.notify(trz2);
    delete r;
    delete w;
    return doc.debugString();
  };

  {
    TopObjectStorage doc;
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc).prop(new TestPropInt2(1));
    EXPECT_STREQ(applySerialized(trz).c_str(), "500#1[552:1]");
  }

  {
    TopObjectStorage doc;
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc)
      .prop(new TestPropInt2(22))
      .prop(new TestPropInt3(33))
      .prop(new TestPropInt3(32))
      .remove(TestPropInt1::typeId_);

    trz->createObject(TestObject1::typeId_, doc)
      .prop(new TestPropInt3(13))
      .remove();

    trz->createObject(TestObject2::typeId_, doc)
      .prop(new TestPropInt3(33))
      .prop(new TestPropLink(0, {1}));

    EXPECT_STREQ(applySerialized(trz).c_str(), "500#1[552:22,553:32]502#3[553:33,554:[0,1]]");
  }
}


TEST(TrzHub, Serialize)
{
  InMemoryTrzStorage file;

  {
    TrzHub hub(11111);
    TopObjectStorage doc;
    hub.connect(&doc);

    hub.connect(&file);

    {
      TrzPtr trz(new Tranzaction());
      trz->createObject(TestTopObject::typeId_, doc).prop(new TestPropInt2(1));
      hub.notify(trz);
      EXPECT_STREQ(doc.debugString().c_str(), "500#1[552:1]");
    }
    {
      TrzPtr trz(new Tranzaction());
      trz->createObject(TestObject2::typeId_, doc)
        .prop(new TestPropInt2(2))
        .prop(new TestPropInt3(3));
      hub.notify(trz);
      EXPECT_STREQ(doc.debugString().c_str(), "500#1[552:1]502#2[552:2,553:3]");
    }

    hub.save();
  }

  {
    TrzHub hub2(11111);
    TopObjectStorage doc;
    hub2.connect(&file); 
    hub2.connect(&doc);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[552:1]502#2[552:2,553:3]");
  }

  {
    TrzHub hub2(11111);
    TopObjectStorage doc;
    hub2.connect(&doc);
    hub2.connect(&file); 
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[552:1]502#2[552:2,553:3]");
  }
}

TEST(DocumentStorage, Simple)
{
  const DocId docId = 11111;
  const string path = PROJECT_DIR "/build/tmp";

  { 
    LocalDocumentStorage lds(path);
    ASSERT_TRUE(lds.list().empty());
    unique_ptr<TranzactionStorage> file(lds.open(docId, TrzFilter::All));

    TrzHub hub(docId);
    TopObjectStorage doc;
    hub.connect(&doc);
    hub.connect(file.get());

    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc).prop(new TestPropInt2(1));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[552:1]");

    hub.save(); 
  }

  { 
    LocalDocumentStorage lds(path);
    EXPECT_FALSE(lds.list().empty());
    unique_ptr<TranzactionStorage> file(lds.open(docId, TrzFilter::All));

    TrzHub hub(docId);
    TopObjectStorage doc;
    hub.connect(&doc);
    hub.connect(file.get());

    EXPECT_STREQ(doc.debugString().c_str(), "500#1[552:1]");
    EXPECT_EQ(hub.id(), docId);
  }

  { 
    LocalDocumentStorage lds(path);
    EXPECT_FALSE(lds.list().empty());
    lds.remove(docId);
    EXPECT_TRUE(lds.list().empty());
  }
}




TEST(DocumentInserting, Simple)
{
  TrzHub hub1(11111);
  TrzHub hub2(22222);
  TopObjectStorage doc1;  
  TopObjectStorage doc2;  
  hub1.connect(&doc1);
  hub2.connect(&doc2);

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc1);
    trz->createObject(TestObject1::typeId_, doc1).prop(new TestPropInt1(1));
    trz->createObject(TestObject2::typeId_, doc1);
    hub1.notify(trz);
    EXPECT_STREQ(doc1.debugString().c_str(), "500#1[]501#2[551:1]502#3[]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc2);
    trz->createObject(TestInsertedDocument::typeId_, doc2).prop(new DocIdProp(doc1.docId()));
    trz->createObject(TestInsertedDocument::typeId_, doc2).prop(new DocIdProp(doc1.docId()));
    trz->changeObject(LongName({3, 3})).prop(new TestPropInt3(3));
    hub2.notify(trz);
    EXPECT_STREQ(doc2.debugString().c_str(),
      "500#1[]553#2[1:11111,500#1[]501#2[551:1]502#3[]]553#3[1:11111,500#1[]501#2[551:1]502#3[553:3]]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(3).remove();
    hub1.notify(trz);
    EXPECT_STREQ(doc1.debugString().c_str(), "500#1[]501#2[551:1]");
    EXPECT_STREQ(doc2.debugString().c_str(),
      "500#1[]553#2[1:11111,500#1[]501#2[551:1]]553#3[1:11111,500#1[]501#2[551:1]]");
  }
}


TEST(DocumentInserting, WithChange)
{
  TrzHub hub1(11111);
  TrzHub hub2(22222);
  TopObjectStorage doc1;  
  TopObjectStorage doc2;  
  hub1.connect(&doc1);
  hub2.connect(&doc2);

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc1);
    hub1.notify(trz);
    EXPECT_STREQ(doc1.debugString().c_str(), "500#1[]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc2);
    trz->createObject(TestInsertedDocument::typeId_, doc2).prop(new DocIdProp(doc1.docId()));
    trz->changeObject(LongName({2, 1})).prop(new TestPropInt3(3));
    hub2.notify(trz);
    EXPECT_STREQ(doc2.debugString().c_str(),
      "500#1[]553#2[1:11111,500#1[553:3]]");
  }
}


TEST(DocumentInserting, Nested)
{
  TrzHub hub1(11111);
  TrzHub hub2(22222);
  TrzHub hub3(33333);
  TopObjectStorage doc1;  
  TopObjectStorage doc2;  
  TopObjectStorage doc3;  
  hub1.connect(&doc1);
  hub2.connect(&doc2);
  hub3.connect(&doc3);

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc3);
    hub3.notify(trz);
  }
  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc2);
    trz->createObject(TestInsertedDocument::typeId_, doc2).prop(new DocIdProp(doc3.docId()));
    hub2.notify(trz);
  }
  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc1);
    trz->createObject(TestInsertedDocument::typeId_, doc1).prop(new DocIdProp(doc2.docId()));
    trz->createObject(TestInsertedDocument::typeId_, doc1).prop(new DocIdProp(doc3.docId()));
    hub1.notify(trz);
  }
  EXPECT_STREQ(doc1.debugString().c_str(), "500#1[]553#2[1:22222,500#1[]553#2[1:33333,500#1[]]]553#3[1:33333,500#1[]]");
}



TEST(DocumentInserting, Recurrently)
{
  TrzHub hub1(11111);
  TopObjectStorage doc1;
  hub1.connect(&doc1);
  TrzPtr trz(new Tranzaction());
  trz->createObject(TestTopObject::typeId_, doc1);
  trz->createObject(TestInsertedDocument::typeId_, doc1).prop(new DocIdProp(doc1.docId()));
  EXPECT_THROW(hub1.notify(trz), ErrorCode);
}

TEST(DocumentInserting, RecurrentlyWithMiddleman)
{
  TrzHub hub1(11111);
  TrzHub hub2(22222);
  TopObjectStorage doc1;
  TopObjectStorage doc2;
  hub1.connect(&doc1);
  hub2.connect(&doc2);
  {
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc2);
    hub2.notify(trz);
  }
  {
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc1);
    trz->createObject(TestInsertedDocument::typeId_, doc1).prop(new DocIdProp(doc2.docId()));
    hub1.notify(trz);
  }
  {
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestInsertedDocument::typeId_, doc2).prop(new DocIdProp(doc1.docId()));
    EXPECT_THROW(hub2.notify(trz), ErrorCode);
  }
}

TEST(DocumentInserting, RecurrentlyNested)
{
  TrzHub hub1(11111);
  TrzHub hub2(22222);
  TrzHub hub3(33333);
  TopObjectStorage doc1;
  TopObjectStorage doc2;
  TopObjectStorage doc3;
  hub1.connect(&doc1);
  hub2.connect(&doc2);
  hub3.connect(&doc3);

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc3);
    hub3.notify(trz);
  }
  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc2);
    trz->createObject(TestInsertedDocument::typeId_, doc2).prop(new DocIdProp(doc3.docId()));
    hub2.notify(trz);
  }
  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc1);
    trz->createObject(TestInsertedDocument::typeId_, doc1).prop(new DocIdProp(doc2.docId()));
    hub1.notify(trz);
  }
  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestInsertedDocument::typeId_, doc3).prop(new DocIdProp(doc2.docId()));
    EXPECT_THROW(hub3.notify(trz), ErrorCode);
  }
}








TEST(TopObjectStorage, DocumentVariant)
{
  TrzHub hub(11111);
  TopObjectStorage doc;
  hub.connect(&doc);

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc).prop(new TestPropInt1(1));
    trz->createObject(TestObject1::typeId_, doc).prop(new DocVariantProp(DocVariantEnum::Enabled));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:1]501#2[155:1]");
  }

  { 
    TrzPtr trz(new Tranzaction({2}));
    trz->changeObject(1).prop(new TestPropInt1(2));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:2]501#2[155:1]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    ObjectChanges & chgs = trz->changeObject(2);
    chgs.prop(new DocVariantProp(DocVariantEnum::Disabled));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:1]501#2[155:0]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).prop(new TestPropInt2(1));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:1,552:1]501#2[155:0]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(2).prop(new DocVariantProp(DocVariantEnum::Active));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:2,552:1]501#2[155:2]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(2).remove();
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:1,552:1]");
  }

  hub.undoRedo(-1); 
  EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:2,552:1]501#2[155:2]");

  hub.undoRedo(-1); 
  EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:1,552:1]501#2[155:0]");

  hub.undoRedo(-1);
  EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:1]501#2[155:0]");

  hub.undoRedo(-1);
  EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:2]501#2[155:1]");
}


TEST(TopObjectStorage, DisabledDocumentVariant)
{
  TrzHub hub(11111);
  TopObjectStorage doc;
  hub.connect(&doc);

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc).prop(new TestPropInt1(1));
    trz->createObject(TestObject1::typeId_, doc).prop(new DocVariantProp(DocVariantEnum::Disabled));
    trz->createObject(TestObjectStorage1::typeId_, doc);
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:1]501#2[155:0]551#3[]");
  }

  { 
    TrzPtr trz(new Tranzaction({ 2 }));
    trz->changeObject(1).prop(new TestPropInt1(2));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:1]501#2[155:0]551#3[]");
  }

  { 
    TrzPtr trz(new Tranzaction({ 2 }));
    trz->createObject(TestObjectStorage2::typeId_, doc);
    trz->createObject(TestObject2::typeId_, *doc.findObjectByType(TestObjectStorage1::typeId_)->isStorage());
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:1]501#2[155:0]551#3[]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(2).prop(new DocVariantProp(DocVariantEnum::Enabled));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:2]501#2[155:1]551#3[502#1[]]552#4[]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestObject1::typeId_, *doc.findObjectByType(TestObjectStorage2::typeId_)->isStorage());
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:2]501#2[155:1]551#3[502#1[]]552#4[501#1[]]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(2).prop(new DocVariantProp(DocVariantEnum::Disabled));
    hub.notify(trz);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:1]501#2[155:0]551#3[]");
  }

  hub.undoRedo(-1);
  EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:2]501#2[155:1]551#3[502#1[]]552#4[501#1[]]");
  hub.undoRedo(-1);
  EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:2]501#2[155:1]551#3[502#1[]]552#4[]");
  hub.undoRedo(-1);
  EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:1]501#2[155:0]551#3[]");
  hub.undoRedo(-1);
  EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:1]501#2[155:0]551#3[]");
  hub.undoRedo(-1);
  EXPECT_STREQ(doc.debugString().c_str(), "500#1[551:1]501#2[155:0]551#3[]");
}


TEST(Property, Serialize)
{
  auto testFn = [](const Property& prop)
  {
    MSDataT data;
    MSWriter* w = new MSWriter(data);
    prop.write(*w);

    { 
      Reader* r = w->getBackReader();
      EXPECT_NO_THROW(r->skip()) << "property \"" << prop.def().name_ << "\" must save one element to Writer";
      EXPECT_THROW(r->skip(), exception) << "too many elements saved by property \"" << prop.def().name_ << "\" to Writer, only one allowed";
      delete r;
    }

    const auto& pdef = Property::propDefs_.get(prop.type());
    Reader* r = w->getBackReader();
    Property* prop2 = pdef.creator_(*r);
    delete r;
    delete w;


    JsonWriter jw1, jw2;
    prop.write(jw1);
    EXPECT_FALSE(jw1.data().empty()) << "property \"" << prop.def().name_ << "\" has no debug string";
    prop2->write(jw2);
    EXPECT_STREQ(jw1.data().c_str(), jw2.data().c_str());
    delete prop2;
  };


  testFn(UserIdProp(DefaultLocalUserId));
  testFn(UserIdProp(MainUserId));
  testFn(DocIdProp(createDocId(MainUserId, 1000000)));


  {
  }

}




/*

TEST(ObjDefinition, CheckAll)
{
  set<all::typeId_
  ::typeId_for (const auto & ot : ObjDefinition::all())
  {
    const bjType ::typeId_= ot.second->type_;

    auto it = allinsert::typeId_(objType);
    EXPECT_TRUE(it.second) << "duplicated object type: " << int(objType);
    EXPECT_NE(int(objType), int(Unchanged::typeId_));
    EXPECT_NE(int(objType), int(Deleted::typeId_));

    EXPECT_LT(int(objType), 1000) << "cannot be localized";

    UnifiedObject *obj = ot.second->create();
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->type(), objType);
    delete obj;
  }

  EXPECT_EQ(allsize::typeId_(), ObjDefinition::all().size());
}
*/





TEST(Property, Links)
{
  { 
    TopObjectStorage doc;
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc);
    trz->createObject(TestObject1::typeId_, doc);
    trz->changeObject(2).prop(new TestPropLink(0, LongName({1})));
    doc.notify(trz);

    ASSERT_EQ(doc.size(true), 2);
    UnifiedObject * obj1 = doc.findObjectByName(1);
    UnifiedObject * obj2 = doc.findObjectByName(2);
    ASSERT_TRUE(obj1 != nullptr);
    ASSERT_TRUE(obj2 != nullptr);
    EXPECT_EQ(obj1->propertyCount(), 0);
    EXPECT_EQ(obj2->propertyCount(), 1);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[]501#2[554:[0,1]]");
  }

  { 
    TopObjectStorage doc;
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc).prop(new TestPropLink(0, LongName({1})));
    doc.notify(trz);

    TestTopObject * obj1 = doc.findObject<TestTopObject>();
    ASSERT_TRUE(obj1 != nullptr);
    EXPECT_EQ(obj1->propertyCount(), 1);
    EXPECT_EQ(obj1->findProp<TestPropLink>()->object(obj1), obj1);
    EXPECT_EQ(obj1->initCount(), 1);
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[554:[0,1]]");
  }

  { 
    TopObjectStorage doc;
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc).prop(new TestPropLink(0, {999}));
    doc.notify(trz);

    TestTopObject * obj1 = doc.findObject<TestTopObject>();
    ASSERT_TRUE(obj1 != nullptr);
    EXPECT_EQ(obj1->propertyCount(), 1);
    EXPECT_FALSE(obj1->findProp<TestPropLink>()->object(obj1));
    EXPECT_STREQ(doc.debugString().c_str(), "500#1[554:[0,999]]");
  }

}


TEST(Property, LinkedObjectReinit)
{
  TopObjectStorage doc;
  {
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc).prop(new TestPropInt1(1));
    trz->createObject(TestObject1::typeId_, doc).prop(new TestPropLink(0, LongName({ 1 })));
    trz->createObject(TestObject2::typeId_, doc);
    doc.notify(trz);
  }
  ASSERT_STREQ(doc.debugString().c_str(), "500#1[551:1]501#2[554:[0,1]]502#3[]");
  TestTopObject* obj1 = doc.findObject<TestTopObject>();
  TestObject1*   obj2 = doc.findObject<TestObject1>();
  TestObject2*   obj3 = doc.findObject<TestObject2>();
  ASSERT_TRUE(obj1 && obj2 && obj3);
  ASSERT_EQ(obj1->initCount(), 1);
  ASSERT_EQ(obj2->initCount(), 1);
  ASSERT_EQ(obj3->initCount(), 1);

  {
    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).prop(new TestPropInt1(2));
    doc.notify(trz);
  }
  EXPECT_EQ(obj1->findProp<TestPropInt1>()->value(), 2);
  EXPECT_EQ(obj1->initCount(), 2);
  EXPECT_EQ(obj2->initCount(), 2); 
  ASSERT_EQ(obj3->initCount(), 1);

  {
    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).prop(new TestPropLink(0, { 2 }));
    doc.notify(trz);
  }
  EXPECT_EQ(obj1->initCount(), 3);
  EXPECT_EQ(obj2->initCount(), 3);
  ASSERT_EQ(obj3->initCount(), 1);
   
  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(1).prop(new TestPropInt1(3));
    doc.notify(trz);
  }
  EXPECT_EQ(obj1->findProp<TestPropInt1>()->value(), 3);
  EXPECT_EQ(obj1->initCount(), 4);
  EXPECT_EQ(obj2->initCount(), 4);
  ASSERT_EQ(obj3->initCount(), 1);
}

/*
TEST(Property, LinkToInsertObject)
{
  TrzHub hub1(11111);
  TrzHub hub2(22222);
  TopObjectStorage doc1; 
  TopObjectStorage doc2; 
  hub1.connect(&doc1);
  hub2.connect(&doc2);

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc1);
    hub1.notify(trz);
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc2).prop(new TestPropLink(0, LongName({2, 1})));
    trz->createObject(TestInsertedDocument::typeId_, doc2).prop(new DocIdProp(doc1.docId()));
    hub2.notify(trz);
    EXPECT_STREQ(doc2.debugString().c_str(), "500#1[554:[0,2,1]]553#2[1:11111,500#1[]]");
  }

  { 
    UnifiedObject* topObj = doc2.findObject(1);
    UnifiedObject* obj21 = doc2.findStorage(2)->findObject(1);
    ASSERT_TRUE(topObj && obj21);

    EXPECT_EQ(topObj->findProp<TestPropLink>()->object(topObj), obj21);
    size_t count = 0;
    for (auto* obj : obj21->linked(TestPropLink::typeId_))
    {
      EXPECT_EQ(obj, topObj);
      count++;
    }
    EXPECT_EQ(count, 1);
  }
}

TEST(Property, LinkFromInsertObject)
{
  TrzHub hub1(11111);
  TrzHub hub2(22222);
  TopObjectStorage doc1; 
  TopObjectStorage doc2; 
  hub1.connect(&doc1);
  hub2.connect(&doc2);

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc1);
    trz->createObject(TestObject1::typeId_, doc1);
    trz->changeObject(2).prop(new TestPropLink(0, LongName({1})));
    hub1.notify(trz);
    EXPECT_STREQ(doc1.debugString().c_str(), "500#1[]501#2[554:[0,1]]");
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->createObject(TestTopObject::typeId_, doc2);
    trz->createObject(TestInsertedDocument::typeId_, doc2).prop(new DocIdProp(doc1.docId()));
    trz->createObject(TestInsertedDocument::typeId_, doc2).prop(new DocIdProp(doc1.docId()));
    hub2.notify(trz);
    EXPECT_STREQ(doc2.debugString().c_str(), "500#1[]553#2[1:11111,500#1[]501#2[554:[0,1]]]553#3[1:11111,500#1[]501#2[554:[0,1]]]");
  }

  { 
    ASSERT_EQ(doc2.size(false), 3);
    ASSERT_EQ(doc2.size(true), 7);
    ObjectStorage* stor1 = doc2.findStorage(2);
    ObjectStorage* stor2 = doc2.findStorage(3);
    ASSERT_TRUE(stor1 != nullptr);
    ASSERT_TRUE(stor2 != nullptr);
  
    TestTopObject* obj10 = stor1->findObject<TestTopObject>();
    TestObject1*   obj11 = stor1->findObject<TestObject1>();
    ASSERT_TRUE(obj10 != nullptr);
    ASSERT_TRUE(obj11 != nullptr);
    EXPECT_EQ(obj11->findProp<TestPropLink>()->object(obj11), obj10);
  
    TestTopObject* obj20 = stor2->findObject<TestTopObject>();
    TestObject1*   obj21 = stor2->findObject<TestObject1>();
    ASSERT_TRUE(obj20 != nullptr);
    ASSERT_TRUE(obj21 != nullptr);
    EXPECT_EQ(obj21->findProp<TestPropLink>()->object(obj21), obj20);

    size_t count = 0;
    for (auto* obj : obj20->linked(TestPropLink::typeId_))
    {
      EXPECT_EQ(obj, obj21);
      count++;
    }
    EXPECT_EQ(count, 1);
  }


  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(LongName({3,2})).prop(new TestPropLink(1, LongName({ 2, 1 })));
    hub2.notify(trz);
    EXPECT_STREQ(doc2.debugString().c_str(), "500#1[]553#2[1:11111,500#1[]501#2[554:[0,1]]]553#3[1:11111,500#1[]501#2[554:[1,2,1]]]");
  }

  { 
    ObjectStorage* stor1 = doc2.findStorage(2);
    ObjectStorage* stor2 = doc2.findStorage(3);
    TestTopObject* obj10 = stor1->findObject<TestTopObject>();
    TestObject1*   obj11 = stor1->findObject<TestObject1>();
    TestObject1*   obj21 = stor2->findObject<TestObject1>();
    const TestPropLink* propLink = obj21->findProp<TestPropLink>();

    EXPECT_EQ(obj11->findProp<TestPropLink>()->object(obj11), obj10);
    EXPECT_EQ(propLink->object(obj21), obj10);
    size_t count = 0;
    for (auto* obj : obj10->linked(TestPropLink::typeId_))
    {
      EXPECT_TRUE(obj == obj21 || obj == obj11);
      count++;
    }
    EXPECT_EQ(count, 2);
  
  }

  { 
    TrzPtr trz(new Tranzaction());
    trz->changeObject(LongName({3,2})).prop(new TestPropLink(1, LongName({ 1 })));
    hub2.notify(trz);
    EXPECT_STREQ(doc2.debugString().c_str(), "500#1[]553#2[1:11111,500#1[]501#2[554:[0,1]]]553#3[1:11111,500#1[]501#2[554:[1,1]]]");
  }
  { 
    TestObject1* obj21 = doc2.findStorage(3)->findObject<TestObject1>();
    EXPECT_EQ(obj21->findProp<TestPropLink>()->object(obj21), doc2.findObject(1));
  }
}
*/

