#include <QtTest>

#include "qjsonmodel.h"

class QJsonModelTest : public QObject
{
   Q_OBJECT

private slots:
   void test_init();
   void test_load();
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
}

QTEST_APPLESS_MAIN(QJsonModelTest)

#include "tst_qjsonmodeltest.moc"
