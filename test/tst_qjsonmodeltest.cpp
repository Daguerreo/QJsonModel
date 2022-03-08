#include <QtTest>

#include "qjsonmodel.h"

class QJsonModelTest : public QObject
{
   Q_OBJECT

private slots:
   void initTestCase();

   void tester();
   void loadFromFile();
   void loadFromString();
   void loadFromStdString();
   void loadFromDevice();
   void loadFromDocument();
   void loadFromValue();
   void loadFromRaw();
   void clear();

private:
   QByteArray _json;
};


void QJsonModelTest::initTestCase()
{
   QFile file(":/sample.json");
   QVERIFY(file.open(QIODevice::ReadOnly));

   auto doc = QJsonDocument::fromJson(file.readAll());
   _json = doc.toJson(QJsonDocument::Compact);
   QVERIFY(!_json.isNull());
   file.close();
}

void QJsonModelTest::tester()
{
   QJsonModel model;
   auto tester = new QAbstractItemModelTester(&model, &model);
   (void)tester; // shut up warnings;
}

void QJsonModelTest::loadFromFile()
{
   QJsonModel model;
   auto tester = new QAbstractItemModelTester(&model, &model);
   (void)tester; // shut up warnings;
   const bool result = model.loadFromFile(":/sample.json");

   QVERIFY(result);
   QCOMPARE(model.json(true), _json);
}

void QJsonModelTest::loadFromString()
{
   QJsonModel model;
   auto tester = new QAbstractItemModelTester(&model, &model);
   (void)tester; // shut up warnings;
   QString qStr = "\
   {\
      \"firstName\": \"John\",\
      \"lastName\": \"Smith\",\
      \"age\": 25,\
      \"address\": {\
         \"streetAddress\": \"21 2nd Street\",\
         \"city\": \"New York\",\
         \"state\": \"NY\",\
         \"postalCode\": \"10021\",\
         \"owner\": true\
      },\
      \"phoneNumber\": [\
         {\
           \"type\": \"home\",\
           \"number\": \"212 555-1234\"\
         },\
         {\
           \"type\": \"fax\",\
           \"number\": \"646 555-4567\"\
         }\
      ]\
   }";

   const bool result = model.loadFromString(qStr);

   QVERIFY(result);
   QCOMPARE(model.json(true), _json);
}

void QJsonModelTest::loadFromStdString()
{
   QJsonModel model;
   auto tester = new QAbstractItemModelTester(&model, &model);
   (void)tester; // shut up warnings;
   std::string str = "\
   {\
      \"firstName\": \"John\",\
      \"lastName\": \"Smith\",\
      \"age\": 25,\
      \"address\": {\
         \"streetAddress\": \"21 2nd Street\",\
         \"city\": \"New York\",\
         \"state\": \"NY\",\
         \"postalCode\": \"10021\",\
         \"owner\": true\
      },\
      \"phoneNumber\": [\
         {\
           \"type\": \"home\",\
           \"number\": \"212 555-1234\"\
         },\
         {\
           \"type\": \"fax\",\
           \"number\": \"646 555-4567\"\
         }\
      ]\
   }";

   const bool result = model.loadFromStdString(str);

   QVERIFY(result);
   QCOMPARE(model.json(true), _json);
}

void QJsonModelTest::loadFromDevice()
{
   QFile file(":/sample.json", this);
   QVERIFY(file.open(QIODevice::ReadOnly));

   QJsonModel model;
   const bool result = model.loadFromDevice(&file);

   QVERIFY(result);
   QCOMPARE(model.json(true), _json);
}

void QJsonModelTest::loadFromDocument()
{
   auto doc = QJsonDocument::fromJson(_json);

   QJsonModel model;
   const bool result = model.loadFromDocument(doc);

   QVERIFY(result);
   QCOMPARE(model.json(true), _json);
}

void QJsonModelTest::loadFromValue()
{
   auto doc = QJsonDocument::fromJson(_json);

   QJsonModel model;
   const bool result = model.loadFromValue(QJsonValue(doc.object()));

   QVERIFY(result);
   QCOMPARE(model.json(true), _json);
}

void QJsonModelTest::loadFromRaw()
{
   QJsonModel model;
   const bool result = model.loadFromRaw(_json);

   QVERIFY(result);
   QCOMPARE(model.json(true), _json);
}

void QJsonModelTest::clear()
{
   QJsonModel model;
   model.loadFromRaw(_json);
   model.clear();
   auto json = model.json(true);

   QVERIFY(json.isEmpty());
}


QTEST_APPLESS_MAIN(QJsonModelTest)

#include "tst_qjsonmodeltest.moc"
