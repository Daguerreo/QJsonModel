#include <QtTest>

#include "qjsonmodel.h"

class QJsonModelTest : public QObject
{
   Q_OBJECT

private slots:
   void test_init();
   void test_load();
   void test_save();
};


void QJsonModelTest::test_init()
{
   QJsonModel model;
   auto tester = new QAbstractItemModelTester(&model, &model);
}

void QJsonModelTest::test_load()
{
   QJsonModel model;
   auto tester = new QAbstractItemModelTester(&model, &model);
   model.load(":/sample.json");

   qDebug() << model.json();
}

void QJsonModelTest::test_save()
{
   QFile file(":/sample.json");
   QVERIFY(file.open(QIODevice::ReadOnly));

   auto jdoc = QJsonDocument::fromJson(file.readAll());
   auto jsonString = jdoc.toJson(QJsonDocument::Compact);

   QJsonModel model;
   model.load(":/sample.json");
   auto jsonStringCompare = jdoc.fromJson(model.json()).toJson(QJsonDocument::Compact);

   QCOMPARE(jsonString, jsonStringCompare);
}

QTEST_APPLESS_MAIN(QJsonModelTest)

#include "tst_qjsonmodeltest.moc"
