#include "screwtablewidget.h"

#include <iostream>

#include "ui_screwtablewidget.h"

ScrewTableWidget::ScrewTableWidget( QWidget * parent, QString configDir )
    : QWidget( parent ), m_configDir( configDir ), ui( new Ui::ScrewTableWidget )
{
    ui->setupUi( this );

    m_screwList = this->GetTable( m_configDir );
    this->UpdateTable();
}

ScrewTableWidget::~ScrewTableWidget()
{
    on_closeButton_clicked();
    delete ui;
}

QList<ScrewTableWidget::ScrewProperties> ScrewTableWidget::GetTable( QString configDir )
{
    QList<ScrewProperties> screwList;
    QString filename( QDir( configDir ).filePath( "PedicleScrewNavigationData/ScrewTable.csv" ) );
    QFile file( filename );
    if( QFileInfo::exists( filename ) )
    {
        if( !file.open( QIODevice::ReadOnly ) )
        {
            std::cerr << file.errorString().toUtf8().constData() << std::endl;
            ;
        }

        screwList.clear();
        int nline = 0;
        while( !file.atEnd() )
        {
            QString line = file.readLine();
            QStringList row;
            for( QString item : line.split( "," ) )
            {
                row.append( item.trimmed() );
            }
            double length   = row.at( 0 ).toDouble();
            double diameter = row.at( 1 ).toDouble();
            ScrewProperties screw( length, diameter );
            screwList.append( screw );
            nline++;
        }

        file.close();
    }
    else
    {
        screwList.clear();
    }
    return screwList;
}

void ScrewTableWidget::UpdateTable()
{
    ui->tableWidget->clear();
    ui->tableWidget->setColumnCount( 2 );
    ui->tableWidget->setRowCount( m_screwList.size() );
    ui->tableWidget->setHorizontalHeaderLabels( QStringList() << "Length (mm)"
                                                              << "Diameter (mm)" );

    for( int i = 0; i < m_screwList.size(); ++i )
    {
        QTableWidgetItem * lengthItem   = new QTableWidgetItem( QString::number( m_screwList.at( i ).first ) );
        QTableWidgetItem * diameterItem = new QTableWidgetItem( QString::number( m_screwList.at( i ).second ) );
        ui->tableWidget->setItem( i, 0, lengthItem );
        ui->tableWidget->setItem( i, 1, diameterItem );
    }
}

bool ScrewTableWidget::ScrewComparator( const ScrewProperties & s1, const ScrewProperties & s2 )
{
    if( s1.first == s2.first )
    {
        return s1.second < s2.second;
    }
    return s1.first < s2.first;
}

void ScrewTableWidget::on_addScrewButton_clicked()
{
    ScrewProperties screw( ui->lengthSpinBox->value(), ui->diameterSpinBox->value() );
    m_screwList.append( screw );
    qSort( m_screwList.begin(), m_screwList.end(), this->ScrewComparator );
    this->UpdateTable();
}

void ScrewTableWidget::on_closeButton_clicked()
{
    QString fileName( QDir( m_configDir ).filePath( "PedicleScrewNavigationData/ScrewTable.csv" ) );
    QFile file( fileName );

    if( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )
    {
        std::cerr << file.errorString().toUtf8().constData() << std::endl;
        ;
    }

    for( int i = 0; i < m_screwList.size(); ++i )
    {
        QByteArray line;
        ScrewProperties screw = m_screwList.at( i );
        line.append( QString::number( screw.first ) );
        line.append( tr( "," ) );
        line.append( QString::number( screw.second ) );
        line.append( tr( "\n" ) );
        file.write( line );
    }

    file.close();

    emit WidgetAboutToClose();
    this->close();
}

void ScrewTableWidget::on_removeScrewButton_clicked()
{
    std::vector<int> listIds;

    QModelIndexList selection = ui->tableWidget->selectionModel()->selectedRows();
    for( int i = 0; i < selection.count(); ++i )
    {
        listIds.push_back( selection.at( i ).row() );
    }
    std::sort( listIds.begin(), listIds.end() );
    int compensationIndex = 0;
    for( size_t i = 0; i < listIds.size(); ++i )
    {
        m_screwList.removeAt( listIds.at( i ) - compensationIndex );
        compensationIndex++;
    }

    this->UpdateTable();
}

void ScrewTableWidget::on_tableWidget_itemSelectionChanged()
{
    QList<QTableWidgetItem *> itemList = ui->tableWidget->selectedItems();
    if( itemList.size() > 0 )
        ui->removeScrewButton->setEnabled( true );
    else
        ui->removeScrewButton->setEnabled( false );
}
