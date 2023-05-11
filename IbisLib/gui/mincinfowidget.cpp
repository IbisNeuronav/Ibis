/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "mincinfowidget.h"

#include <itkMetaDataDictionary.h>
#include <itkMetaDataObject.h>
#include <vtkImageData.h>

#include <QGridLayout>
#include <QHeaderView>
#include <QTableWidget>
#include <typeinfo>

#include "imageobject.h"

MincInfoWidget::MincInfoWidget( QWidget * parent ) : QWidget( parent )
{
    m_imageObj = 0;
    setWindowTitle( "MINC Info" );
}

MincInfoWidget::~MincInfoWidget() {}

void MincInfoWidget::SetImageObject( ImageObject * img )
{
    if( m_imageObj == img ) return;
    m_imageObj = img;
    this->UpdateUI();
}

void MincInfoWidget::UpdateUI()
{
    Q_ASSERT( m_imageObj );
    QString varName;
    QString varValue;
    QList<QPair<QString, QString> > list;

    itk::MetaDataDictionary dictionary;
    if( !m_imageObj->IsLabelImage() )
        dictionary = m_imageObj->GetItkImage()->GetMetaDataDictionary();
    else
        dictionary = m_imageObj->GetItkLabelImage()->GetMetaDataDictionary();

    itk::MetaDataDictionary::ConstIterator it = dictionary.Find( "storage_data_type" );
    if( it != dictionary.End() )
    {
        varName                                = "storage type";
        itk::MetaDataObjectBase * bs           = ( *it ).second;
        itk::MetaDataObject<std::string> * str = dynamic_cast<itk::MetaDataObject<std::string> *>( bs );
        std::string storage_data_type          = str->GetMetaDataObjectValue().c_str();
        if( storage_data_type == typeid( char ).name() )
            varValue = "char";
        else if( storage_data_type == typeid( unsigned char ).name() )
            varValue = "unsigned char";
        else if( storage_data_type == typeid( short ).name() )
            varValue = "short";
        else if( storage_data_type == typeid( unsigned short ).name() )
            varValue = "unsigned short";
        else if( storage_data_type == typeid( int ).name() )
            varValue = "int";
        else if( storage_data_type == typeid( unsigned int ).name() )
            varValue = "unsigned int";
        else if( storage_data_type == typeid( float ).name() )
            varValue = "float";
        else if( storage_data_type == typeid( double ).name() )
            varValue = "double";
        list.append( QPair<QString, QString>( varName, varValue ) );
    }
    double range[ 2 ];
    m_imageObj->GetImage()->GetScalarRange( range );
    varName  = "scalar Range";
    varValue = QString::number( (int)floor( range[ 0 ] ) ) + "  " + QString::number( (int)ceil( range[ 1 ] ) );
    list.append( QPair<QString, QString>( varName, varValue ) );

    it = dictionary.Find( "dimension_order" );
    if( it != dictionary.End() )
    {
        varName                                = "image dimensions";
        itk::MetaDataObjectBase * bs           = ( *it ).second;
        itk::MetaDataObject<std::string> * str = dynamic_cast<itk::MetaDataObject<std::string> *>( bs );
        varValue                               = str->GetMetaDataObjectValue().c_str();
        varValue.remove( 0, 1 );
        varValue.replace( "+", "  " );
        varValue.replace( "-", "  " );
        varValue.replace( "X", "xspace" );
        varValue.replace( "Y", "yspace" );
        varValue.replace( "Z", "zspace" );
        varValue.replace( "V", "vector_dimension" );
        list.append( QPair<QString, QString>( varName, varValue ) );
    }

    // get variables from ImageObject and add elements to list
    double * spacing = m_imageObj->GetSpacing();
    int * dims       = m_imageObj->GetImage()->GetDimensions();
    double bounds[ 6 ];
    m_imageObj->GetBounds( bounds );
    varName  = "xspace length";
    varValue = QString::number( dims[ 0 ] );
    list.append( QPair<QString, QString>( varName, varValue ) );
    varName  = "yspace length";
    varValue = QString::number( dims[ 1 ] );
    list.append( QPair<QString, QString>( varName, varValue ) );
    varName  = "zspace length";
    varValue = QString::number( dims[ 2 ] );
    list.append( QPair<QString, QString>( varName, varValue ) );
    varName  = "x step";
    varValue = QString::number( spacing[ 0 ] );
    list.append( QPair<QString, QString>( varName, varValue ) );
    varName  = "y step";
    varValue = QString::number( spacing[ 1 ] );
    list.append( QPair<QString, QString>( varName, varValue ) );
    varName  = "z step";
    varValue = QString::number( spacing[ 2 ] );
    list.append( QPair<QString, QString>( varName, varValue ) );
    varName  = "x start";
    varValue = QString::number( bounds[ 0 ] );
    list.append( QPair<QString, QString>( varName, varValue ) );
    varName  = "y start";
    varValue = QString::number( bounds[ 2 ] );
    list.append( QPair<QString, QString>( varName, varValue ) );
    varName  = "z start";
    varValue = QString::number( bounds[ 4 ] );
    list.append( QPair<QString, QString>( varName, varValue ) );

    for( itk::MetaDataDictionary::ConstIterator it = dictionary.Begin(); it != dictionary.End(); ++it )
    {
        itk::MetaDataObjectBase * bs           = ( *it ).second;
        itk::MetaDataObject<std::string> * str = dynamic_cast<itk::MetaDataObject<std::string> *>( bs );
        if( strstr( ( *it ).first.c_str(), "dicom" ) == 0 && strcmp( ( *it ).first.c_str(), "dimension_order" ) != 0 &&
            strcmp( ( *it ).first.c_str(), "storage_data_type" ) != 0 )
        {
            varName = ( *it ).first.c_str();
            if( str )
            {
                varValue = str->GetMetaDataObjectValue().c_str();
                list.append( QPair<QString, QString>( varName, varValue ) );
            }
        }
    }

    QTableWidget * table = new QTableWidget( list.count(), 2 );
    table->setHorizontalHeaderLabels( QStringList() << tr( "Name" ) << tr( "Value" ) );
    table->verticalHeader()->setVisible( false );
    table->resize( 300, 300 );

    for( int i = 0; i < list.count(); ++i )
    {
        QPair<QString, QString> pair = list.at( i );

        QTableWidgetItem * nameItem = new QTableWidgetItem( pair.first );
        QTableWidgetItem * valItem  = new QTableWidgetItem( pair.second );

        table->setItem( i, 0, nameItem );
        table->setItem( i, 1, valItem );
    }

    table->resizeColumnToContents( 0 );
    table->horizontalHeader()->setStretchLastSection( true );
    QGridLayout * layout = new QGridLayout;
    layout->addWidget( table, 0, 0 );

    setLayout( layout );
    this->resize( 600, 400 );
}
