/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2011 SCHUTZ Sacha
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef QJSONMODEL_H
#define QJSONMODEL_H

#include <QAbstractItemModel>
#include <QJsonArray>
#include <QJsonObject>

class QJsonModel;
class QJsonItem;

class QJsonTreeItem
{
public:
   QJsonTreeItem(QJsonTreeItem* parent = nullptr);
   ~QJsonTreeItem();

   void appendChild(QJsonTreeItem* item);
   QJsonTreeItem* child(int row);
   QJsonTreeItem* parent();
   int childCount() const;
   int row() const;

   void setKey(const QString& key);
   QString key() const;

   void setValue(const QVariant& value);
   QVariant value() const;

   void setType(const QJsonValue::Type& type);
   QJsonValue::Type type() const;


   static QJsonTreeItem* load(const QJsonValue& value, QJsonTreeItem * parent = nullptr);

private:
   QString mKey;
   QVariant mValue;
   QJsonValue::Type mType;
   QList<QJsonTreeItem*> mChilds;
   QJsonTreeItem* mParent;
};

//---------------------------------------------------

class QJsonModel : public QAbstractItemModel
{
   Q_OBJECT
public:
   explicit QJsonModel(QObject* parent = nullptr);
   QJsonModel(const QString& fileName, QObject* parent = nullptr);
   QJsonModel(QIODevice* device, QObject* parent = nullptr);
   QJsonModel(const QByteArray& json, QObject* parent = nullptr);
   ~QJsonModel();

   // QAbstractItemModel Interface
public:
   QVariant data(const QModelIndex& index, int role) const override;
   bool setData(const QModelIndex& index, const QVariant &value, int role = Qt::EditRole) override;
   QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
   QModelIndex index(int row, int column,const QModelIndex& parent = QModelIndex()) const override;
   QModelIndex parent(const QModelIndex& index) const override;
   int rowCount(const QModelIndex& parent = QModelIndex()) const override;
   int columnCount(const QModelIndex& parent = QModelIndex()) const override;
   Qt::ItemFlags flags(const QModelIndex& index) const override;

public:
   bool load(const QString& fileName);
   bool load(QIODevice* device);
   bool loadJson(const QByteArray& json);
   QJsonDocument json() const;
   void clear();

private:
   QJsonValue genJson(QJsonTreeItem* item) const;
   QJsonTreeItem* internalData(const QModelIndex& index) const;

   QJsonTreeItem* mRootItem;
   QStringList mHeaders;
};

#endif // QJSONMODEL_H
