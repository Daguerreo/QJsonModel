#include <QtTest>

#include "qjsonmodel.h"

class QJsonModelTest : public QObject
{
   Q_OBJECT

private slots:
   void initTestCase();

   void tester();
   void loadFromFile();
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
