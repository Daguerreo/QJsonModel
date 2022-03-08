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

void QJsonTreeItem::setValue(const QVariant& value)
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

QVariant QJsonTreeItem::value() const
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
      rootItem->setValue(value.toVariant());
      rootItem->setType(value.type());
   }

   return rootItem;
}

//=========================================================================

inline uchar hexdig(uint u)
{
    return (u < 0xa ? '0' + u : 'a' + u - 0xa);
}

QByteArray escapedString(const QString &s)
{
    QByteArray ba(s.length(), Qt::Uninitialized);
    uchar *cursor = reinterpret_cast<uchar *>(const_cast<char *>(ba.constData()));
    const uchar *ba_end = cursor + ba.length();
    const ushort *src = reinterpret_cast<const ushort *>(s.constBegin());
    const ushort *const end = reinterpret_cast<const ushort *>(s.constEnd());
    while (src != end)
    {
        if (cursor >= ba_end - 6)
        {
            // ensure we have enough space
            int pos = cursor - (const uchar *)ba.constData();
            ba.resize(ba.size() * 2);
            cursor = (uchar *)ba.data() + pos;
            ba_end = (const uchar *)ba.constData() + ba.length();
        }
        uint u = *src++;
        if (u < 0x80)
        {
            if (u < 0x20 || u == 0x22 || u == 0x5c)
            {
                *cursor++ = '\\';
                switch (u)
                {
                case 0x22:
                    *cursor++ = '"';
                    break;
                case 0x5c:
                    *cursor++ = '\\';
                    break;
                case 0x8:
                    *cursor++ = 'b';
                    break;
                case 0xc:
                    *cursor++ = 'f';
                    break;
                case 0xa:
                    *cursor++ = 'n';
                    break;
                case 0xd:
                    *cursor++ = 'r';
                    break;
                case 0x9:
                    *cursor++ = 't';
                    break;
                default:
                    *cursor++ = 'u';
                    *cursor++ = '0';
                    *cursor++ = '0';
                    *cursor++ = hexdig(u >> 4);
                    *cursor++ = hexdig(u & 0xf);
                }
            }
            else
            {
                *cursor++ = (uchar)u;
            }
        }
        else if (QUtf8Functions::toUtf8<QUtf8BaseTraits>(u, cursor, src, end) < 0)
        {
            // failed to get valid utf8 use JSON escape sequence
            *cursor++ = '\\';
            *cursor++ = 'u';
            *cursor++ = hexdig(u >> 12 & 0x0f);
            *cursor++ = hexdig(u >> 8 & 0x0f);
            *cursor++ = hexdig(u >> 4 & 0x0f);
            *cursor++ = hexdig(u & 0x0f);
        }
    }
    ba.resize(cursor - (const uchar *)ba.constData());
    return ba;
}

QJsonModel::QJsonModel(QObject *parent)
    : QAbstractItemModel(parent)
    , mRootItem{new QJsonTreeItem}
    , mMode{Mode::ReadOnly}
{
}

QJsonModel::QJsonModel(const QString& fileName, QObject* parent)
   : QJsonModel(parent)
{
   loadFromFile(fileName);
}

QJsonModel::QJsonModel(QIODevice* device, QObject* parent)
   : QJsonModel(parent)
{
   if (device) {
      loadFromDevice(device);
   }
}

QJsonModel::QJsonModel(const QByteArray& json, QObject* parent)
   : QJsonModel(parent)
{
   loadFromRaw(json);
}

QJsonModel::~QJsonModel()
{
   delete mRootItem;
}

bool QJsonModel::loadFromFile(const QString& fileName)
{
   QFile file(fileName);
   bool success = false;

   if (file.open(QIODevice::ReadOnly)) {
      success = loadFromDevice(&file);
      file.close();
   }
   return success;
}

bool QJsonModel::loadFromString(const QString qStr) {
    QByteArray qba = qStr.toLocal8Bit();
    return loadFromRaw(qba);
}

bool QJsonModel::loadFromStdString(const std::string str) {
    QString qStr = QString::fromStdString(str);
    return loadFromString(qStr);
}

bool QJsonModel::loadFromDevice(QIODevice* device)
{
   return loadFromRaw(device->readAll());
}

bool QJsonModel::loadFromValue(const QJsonValue& value)
{
   if(!value.isObject() && !value.isArray()) {
      qDebug() << Q_FUNC_INFO << "value must be object or array";
      return false;
   }

   beginResetModel();
   delete mRootItem;
   mRootItem = QJsonTreeItem::load(value);
   mRootItem->setType(value.isObject() ? QJsonValue::Object : QJsonValue::Array);
   endResetModel();

   return true;
}

bool QJsonModel::loadFromDocument(const QJsonDocument& document)
{
   if(document.isNull()) {
      qDebug() << Q_FUNC_INFO << "cannot load json";
      return false;
   }

   beginResetModel();
   delete mRootItem;
   if (document.isArray()) {
      mRootItem = QJsonTreeItem::load(QJsonValue(document.array()));
      mRootItem->setType(QJsonValue::Array);

   } else {
      mRootItem = QJsonTreeItem::load(QJsonValue(document.object()));
      mRootItem->setType(QJsonValue::Object);
   }
   endResetModel();
   return true;
}

bool QJsonModel::loadFromRaw(const QByteArray& json)
{
   return loadFromDocument(QJsonDocument::fromJson(json));
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
         item->setValue(value);
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
      static const QStringList headers{{"key", "value"}};
      return headers.value(section);
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
   if(!index.isValid() || mMode == ReadOnly){
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

QByteArray QJsonModel::json(bool compact) const
{
    auto jsonValue = genJson(mRootItem);
    QByteArray json;
    if (jsonValue.isNull()) {
        return json;
    }
    if (jsonValue.isArray()) {
        arrayToJson(jsonValue.toArray(), json, 0, compact);
    }
    else {
        objectToJson(jsonValue.toObject(), json, 0, compact);
    }
    return json;
}

void QJsonModel::objectToJson(QJsonObject jsonObject, QByteArray &json, int indent, bool compact) const
{
    json += compact ? "{" : "{\n";
    objectContentToJson(jsonObject, json, indent + (compact ? 0 : 1), compact);
    json += QByteArray(4 * indent, ' ');
    json += compact ? "}" : "}\n";
}

void QJsonModel::arrayToJson(QJsonArray jsonArray, QByteArray &json, int indent, bool compact) const
{
    json += compact ? "[" : "[\n";
    arrayContentToJson(jsonArray, json, indent + (compact ? 0 : 1), compact);
    json += QByteArray(4 * indent, ' ');
    json += compact ? "]" : "]\n";
}

void QJsonModel::arrayContentToJson(QJsonArray jsonArray, QByteArray &json, int indent, bool compact) const
{
    if (jsonArray.size() <= 0)
    {
        return;
    }
    QByteArray indentString(4 * indent, ' ');
    int i = 0;
    while (1)
    {
        json += indentString;
        valueToJson(jsonArray.at(i), json, indent, compact);
        if (++i == jsonArray.size())
        {
            if (!compact)
                json += '\n';
            break;
        }
        json += compact ? "," : ",\n";
    }
}
void QJsonModel::objectContentToJson(QJsonObject jsonObject, QByteArray &json, int indent, bool compact) const
{
    if (jsonObject.size() <= 0)
    {
        return;
    }
    QByteArray indentString(4 * indent, ' ');
    int i = 0;
    while (1)
    {
        QString key = jsonObject.keys().at(i);
        json += indentString;
        json += '"';
        json += escapedString(key);
        json += compact ? "\":" : "\": ";
        valueToJson(jsonObject.value(key), json, indent, compact);
        if (++i == jsonObject.size())
        {
            if (!compact)
                json += '\n';
            break;
        }
        json += compact ? "," : ",\n";
    }
}

void QJsonModel::valueToJson(QJsonValue jsonValue, QByteArray &json, int indent, bool compact) const
{
    QJsonValue::Type type = jsonValue.type();
    switch (type)
    {
    case QJsonValue::Bool:
        json += jsonValue.toBool() ? "true" : "false";
        break;
    case QJsonValue::Double:
    {
        const double d = jsonValue.toDouble();
        if (qIsFinite(d))
        {
            json += QByteArray::number(d, 'f', QLocale::FloatingPointShortest);
        }
        else
        {
            json += "null"; // +INF || -INF || NaN (see RFC4627#section2.4)
        }
        break;
    }
    case QJsonValue::String:
        json += '"';
        json += escapedString(jsonValue.toString());
        json += '"';
        break;
    case QJsonValue::Array:
        json += compact ? "[" : "[\n";
        arrayContentToJson(jsonValue.toArray(), json, indent + (compact ? 0 : 1), compact);
        json += QByteArray(4 * indent, ' ');
        json += ']';
        break;
    case QJsonValue::Object:
        json += compact ? "{" : "{\n";
        objectContentToJson(jsonValue.toObject(), json, indent + (compact ? 0 : 1), compact);
        json += QByteArray(4 * indent, ' ');
        json += '}';
        break;
    case QJsonValue::Null:
    default:
        json += "null";
    }
}

void QJsonModel::clear()
{
   beginResetModel();
   delete mRootItem;
   mRootItem = new QJsonTreeItem();
   endResetModel();
}

QJsonModel::Mode QJsonModel::mode() const
{
   return mMode;
}

void QJsonModel::setMode(const Mode& newMode)
{
   if (mMode == newMode){
      return;
   }
   mMode = newMode;
   emit modeChanged(mMode);
}

QJsonValue QJsonModel::genJson(QJsonTreeItem* item) const
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
      return QJsonValue::fromVariant(item->value());
   }
}

QJsonTreeItem* QJsonModel::internalData(const QModelIndex& index) const
{
   return static_cast<QJsonTreeItem*>(index.internalPointer());
}

#include "moc_qjsonmodel.cpp"
