/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "aboutpluginswidget.h"

#include "application.h"
#include "ibisplugin.h"
#include "ui_aboutpluginswidget.h"

AboutPluginsWidget::AboutPluginsWidget( QWidget * parent ) : QWidget( parent ), ui( new Ui::AboutPluginsWidget )
{
    ui->setupUi( this );
    UpdateUI();
}

AboutPluginsWidget::~AboutPluginsWidget() { delete ui; }

void AboutPluginsWidget::UpdateUI()
{
    ui->pluginListWidget->clear();
    QList<QTreeWidgetItem *> items;
    QList<IbisPlugin *> allPlugins;
    Application::GetInstance().GetAllPlugins( allPlugins );
    foreach( IbisPlugin * plugin, allPlugins )
    {
        QStringList cols;
        cols.append( plugin->GetPluginName() );
        cols.append( plugin->GetPluginTypeAsString() );
        QTreeWidgetItem * it = new QTreeWidgetItem( (QTreeWidget *)0, cols );
        it->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled );
        items.append( it );
    }
    ui->pluginListWidget->insertTopLevelItems( 0, items );
    if( allPlugins.size() > 0 ) ui->pluginListWidget->setCurrentItem( items[0] );
    ui->pluginListWidget->resizeColumnToContents( 0 );
}

void AboutPluginsWidget::on_pluginListWidget_currentItemChanged( QTreeWidgetItem * current, QTreeWidgetItem * previous )
{
    QString pluginName = current->text( 0 );
    IbisPlugin * p     = Application::GetInstance().GetPluginByName( pluginName );
    Q_ASSERT( p );
    ui->descriptionWidget->setText( p->GetPluginDescription() );
}
