/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#ifndef MINCINFOWIDGET_H
#define MINCINFOWIDGET_H

#include "ui_mincinfowidget.h"
#include <QString>

class MincInfoWidget : public QWidget, public Ui::MincInfoWidget
{
    Q_OBJECT

public:
    MincInfoWidget( QWidget* parent = 0, const char* name = 0 );
    ~MincInfoWidget();

    void SetInfoText(QString info);

};

#endif // MINCINFOWIDGET_H
