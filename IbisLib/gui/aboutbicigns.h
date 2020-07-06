/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TAG_ABOUTBICIGNS_H
#define TAG_ABOUTBICIGNS_H

#include <QDialog>
#include <QString>
#include <QObject>

namespace Ui {
    class AboutBICIgns;
}

class AboutBICIgns : public QDialog
{
Q_OBJECT

public:

    AboutBICIgns( QWidget * parent = 0, const char * name = 0 );
    ~AboutBICIgns();
        
    void Initialize( QString appName, QString version, QString buildDate );

private:

    Ui::AboutBICIgns *ui;
};


#endif  // TAG_ABOUTBICIGNS_H
