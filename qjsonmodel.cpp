/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2011 SCHUTZ Sacha
 * Copyright (c) 2021 Maurizio Ingrassia
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

#include "qjsonmodel.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>


QJsonTreeItem::QJsonTreeItem(QJsonTreeItem* parent)
   : mParent(parent)
{
}

QJsonTreeItem::~QJsonTreeItem()
{
   qDeleteAll(mChilds);
}

void QJsonTreeItem::appendChild(QJsonTreeItem* item)
{
   mChilds.append(item);
}

QJsonTreeItem* QJsonTreeItem::child(int row)
{
   return mChilds.value(row);
}

QJsonTreeItem* QJsonTreeItem::parent()
{
   return mParent;
}

int QJsonTreeItem::childCount() const
{
   return mChilds.count();
}

int QJsonTreeItem::row() const
{
   if (mParent){
      return mParent->mChilds.indexOf(const_cast<QJsonTreeItem*>(this));
   }

   return 0;
}

void QJsonTreeItem::setKey(const QString& key)
{
   mKey = key;
}

void QJsonTreeItem::setValue(const QString& value)
{
   mValue = value;
}

void QJsonTreeItem::setType(const QJsonValue::Type& type)
{
   mType = type;
}

QString QJsonTreeItem::key() const
{
   return mKey;
}

QString QJsonTreeItem::value() const
{
   return mValue;
}

QJsonValue::Type QJsonTreeItem::type() const
{
   return mType;
}

QJsonTreeItem* QJsonTreeItem::load(const QJsonValue& value, QJsonTreeItem* parent)
{
   auto rootItem = new QJsonTreeItem(parent);
   rootItem->setKey("root");

   if (value.isObject())
   {
      auto object = value.toObject();
      for(auto it=object.begin(); it!=object.end(); ++it){
         auto value = it.value();
         QJsonTreeItem* child = load(value, rootItem);
         child->setKey(it.key());
         child->setType(value.type());
         rootItem->appendChild(child);
      }
   }

   else if (value.isArray())
   {
      int index = 0;
      for (auto&& v : value.toArray())
      {
         QJsonTreeItem* child = load(v, rootItem);
         child->setKey(QString::number(index));
         child->setType(v.type());
         rootItem->appendChild(child);
         ++index;
      }
   }
   else
   {
      // Conversion to QVariant is mandatory
      rootItem->setValue(value.toVariant().toString());
      rootItem->setType(value.type());
   }

   return rootItem;
}

//=========================================================================

QJsonModel::QJsonModel(QObject* parent)
   : QAbstractItemModel(parent)
   , mRootItem{new QJsonTreeItem}
{
   mHeaders.append("key");
   mHeaders.append("value");
}

QJsonModel::QJsonModel(const QString& fileName, QObject* parent)
   : QAbstractItemModel(parent)
   , mRootItem{new QJsonTreeItem}
{
   mHeaders.append("key");
   mHeaders.append("value");
   load(fileName);
}

QJsonModel::QJsonModel(QIODevice* device, QObject* parent)
   : QAbstractItemModel(parent)
   , mRootItem{new QJsonTreeItem}
{
   if (device) {
      mHeaders.append("key");
      mHeaders.append("value");
      load(device);
   }
}

QJsonModel::QJsonModel(const QByteArray& json, QObject* parent)
   : QAbstractItemModel(parent)
   , mRootItem{new QJsonTreeItem}
{
   mHeaders.append("key");
   mHeaders.append("value");
   loadJson(json);
}

QJsonModel::~QJsonModel()
{
   delete mRootItem;
}

bool QJsonModel::load(const QString& fileName)
{
   QFile file(fileName);
   bool success = false;

   if (file.open(QIODevice::ReadOnly)) {
      success = load(&file);
      file.close();
   }

   return success;
}

bool QJsonModel::load(QIODevice* device)
{
   return loadJson(device->readAll());
}

bool QJsonModel::loadJson(const QByteArray& json)
{
   auto const& jdoc = QJsonDocument::fromJson(json);

   if (!jdoc.isNull())
   {
      beginResetModel();
      delete mRootItem;
      if (jdoc.isArray()) {
         mRootItem = QJsonTreeItem::load(QJsonValue(jdoc.array()));
         mRootItem->setType(QJsonValue::Array);

      } else {
         mRootItem = QJsonTreeItem::load(QJsonValue(jdoc.object()));
         mRootItem->setType(QJsonValue::Object);
      }
      endResetModel();
      return true;
   }

   qDebug() << Q_FUNC_INFO << "cannot load json";
   return false;
}

QVariant QJsonModel::data(const QModelIndex& index, int role) const
{
   if (!index.isValid()){
      return QVariant();
   }

   auto item = internalData(index);

   if (role == Qt::DisplayRole) {
      if (index.column() == 0){
         return item->key();
      }

      if (index.column() == 1) {
         return item->value();
      }

   } else if (Qt::EditRole == role) {
      if (index.column() == 1) {
         return item->value();
      }
   }

   return QVariant();
}

bool QJsonModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
   int col = index.column();
   if (Qt::EditRole == role) {
      if (col == 1) {
         auto item = internalData(index);
         item->setValue(value.toString());
         emit dataChanged(index, index, {Qt::EditRole});
         return true;
      }
   }

   return false;
}

QVariant QJsonModel::headerData(int section, Qt::Orientation orientation, int role) const
{
   if (role != Qt::DisplayRole){
      return QVariant();
   }

   if (orientation == Qt::Horizontal) {
      return mHeaders.value(section);
   }
   else {
      return QVariant();
   }
}

QModelIndex QJsonModel::index(int row, int column, const QModelIndex& parent) const
{
   if (!hasIndex(row, column, parent)) {
      return QModelIndex();
   }

   QJsonTreeItem* parentItem = parent.isValid() ? internalData(parent) : mRootItem;

   if (auto childItem = parentItem->child(row)) {
      return createIndex(row, column, childItem);
   }
   else {
      return QModelIndex();
   }
}

QModelIndex QJsonModel::parent(const QModelIndex& index) const
{
   if (!index.isValid()){
      return QModelIndex();
   }

   auto childItem = internalData(index);
   auto parentItem = childItem->parent();

   if (parentItem == mRootItem) {
      return QModelIndex();
   }

   return createIndex(parentItem->row(), 0, parentItem);
}

int QJsonModel::rowCount(const QModelIndex& parent) const
{
   QJsonTreeItem* parentItem;
   if (parent.column() > 0){
      return 0;
   }

   if (!parent.isValid()){
      parentItem = mRootItem;
   }
   else {
      parentItem = internalData(parent);
   }

   return parentItem->childCount();
}

int QJsonModel::columnCount(const QModelIndex& /*parent*/) const
{
   return 2;
}

Qt::ItemFlags QJsonModel::flags(const QModelIndex& index) const
{
   if(!index.isValid()){
      return QAbstractItemModel::flags(index);
   }

   int col = index.column();
   auto item = internalData(index);
   auto isArray = QJsonValue::Array == item->type();
   auto isObject = QJsonValue::Object == item->type();

   if ((col == 1) && !(isArray || isObject)) {
      return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
   } else {
      return QAbstractItemModel::flags(index);
   }
}

QJsonDocument QJsonModel::json() const
{
   auto v = genJson(mRootItem);
   QJsonDocument doc;

   if (v.isObject()) {
      doc = QJsonDocument(v.toObject());
   } else {
      doc = QJsonDocument(v.toArray());
   }

   return doc;
}

void QJsonModel::clear()
{
   beginResetModel();
   delete mRootItem;
   endResetModel();
}

QJsonValue  QJsonModel::genJson(QJsonTreeItem* item) const
{
   auto type = item->type();
   int nchild = item->childCount();

   if (QJsonValue::Object == type) {
      QJsonObject jo;
      for (int i = 0; i < nchild; ++i) {
         auto ch = item->child(i);
         auto key = ch->key();
         jo.insert(key, genJson(ch));
      }
      return jo;
   } else if (QJsonValue::Array == type) {
      QJsonArray arr;
      for (int i = 0; i < nchild; ++i) {
         auto ch = item->child(i);
         arr.append(genJson(ch));
      }
      return arr;
   } else {
      QJsonValue va(item->value());
      return va;
   }
}

QJsonTreeItem* QJsonModel::internalData(const QModelIndex& index) const
{
   return static_cast<QJsonTreeItem*>(index.internalPointer());
}

#include "moc_qjsonmodel.cpp"
