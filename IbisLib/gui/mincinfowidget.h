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

#include <QWidget>

class ImageObject;

class MincInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MincInfoWidget(QWidget *parent = 0);
    ~MincInfoWidget();

    void SetImageObject( ImageObject *img );

private:
    void UpdateUI();

    ImageObject *m_imageObj;
};

#endif // MINCINFOWIDGET_H
