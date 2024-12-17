/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef ABOUTPLUGINSWIDGET_H
#define ABOUTPLUGINSWIDGET_H

#include <QObject>
#include <QWidget>

namespace Ui
{
class AboutPluginsWidget;
}

class QTreeWidgetItem;

class AboutPluginsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AboutPluginsWidget( QWidget * parent = 0 );
    ~AboutPluginsWidget();

private slots:

    void UpdateUI();
    void on_pluginListWidget_currentItemChanged( QTreeWidgetItem * current, QTreeWidgetItem * previous );

private:
    Ui::AboutPluginsWidget * ui;
};

#endif
