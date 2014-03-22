#include "AppStateData.h"

QJsonObject CategoryData::serializeToJson()
{
    QJsonArray subcatArray;
    for ( int i = 0; i < subcategories->count(); i++ )
        subcatArray.append( subcategories->at( i ).toString() );

    QJsonObject jsonObj;
    jsonObj["name"] = name;
    jsonObj["subcategories"] = subcatArray;

    return jsonObj;
}


QJsonObject ImageData::serializeToJson()
{
    QJsonObject jsonObj;
    jsonObj["name"] = name;
    jsonObj["path"] = path;
    jsonObj["category"] = category;
    jsonObj["subcategory"] = subCategory;
    jsonObj["color"] = color;
    jsonObj["size"] = size;
    jsonObj["year"] = year;
    jsonObj["broken"] = broken;
    jsonObj["missingParts"] = missingParts;
    jsonObj["batteries"] = batteries;

    return jsonObj;
}

QJsonObject AppStateData::serializeToJson()
{
}

AppStateData::AppStateData(QObject *parent) : QObject( parent )
{
    listCategories = new QList<CategoryData*>();
    listImageData  = new QList<ImageData*>();
}

const CategoryData* AppStateData::getCategoryByName( const QString name )
{
    for ( int i = 0; i < listCategories->count(); i++ )
    {
        if ( listCategories->at( i )->name == name )
            return listCategories->at( i );
    }

    return new CategoryData();
}

const ImageData* AppStateData::getImageDataByName( const QString name )
{
    for ( int i = 0; i < listImageData->count(); i++ )
    {
        if ( listImageData->at( i )->name == name )
            return listImageData->at( i );
    }

    return new ImageData();
}
