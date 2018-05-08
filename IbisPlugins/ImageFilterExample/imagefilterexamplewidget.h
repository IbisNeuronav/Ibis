/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#ifndef __ImageFilterExampleWidget_h_
#define __ImageFilterExampleWidget_h_

#include <QWidget>

class ImageFilterExamplePluginInterface;

namespace Ui
{
    class ImageFilterExampleWidget;
}


class ImageFilterExampleWidget : public QWidget
{

    Q_OBJECT

public:

    explicit ImageFilterExampleWidget(QWidget *parent = 0);
    ~ImageFilterExampleWidget();

    void SetPluginInterface( ImageFilterExamplePluginInterface * interf );

private:

    void UpdateUi();

    Ui::ImageFilterExampleWidget * ui;
    ImageFilterExamplePluginInterface * m_pluginInterface;


private slots:

    void on_startButton_clicked();

};

#endif
