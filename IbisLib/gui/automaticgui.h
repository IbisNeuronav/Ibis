/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __GuiGenerator_h_
#define __GuiGenerator_h_

#include <QWidget>

class QComboBox;
class vtkGenericParamInterface;
class vtkGenericParam;
class vtkComboParam;

// Base class for all classes that generate and manage GUI for a
// single generic property of an object
class ParamGui : public QObject
{
public:
    ParamGui( QObject * parent ) : QObject( parent ) {}
protected:
    virtual void Update() {}
};

class ComboParamGui : public ParamGui
{
    Q_OBJECT
public:
    ComboParamGui( vtkComboParam * param, QWidget * parent = 0 );
private slots:
    void OnComboBoxActivated( int );
protected:
    virtual void Update();
    QComboBox * m_comboBox;
    vtkComboParam * m_param;
};


class AutomaticGui : public QWidget
{
    Q_OBJECT
public:
    AutomaticGui( vtkGenericParamInterface * params, QWidget * parent = 0 );
protected:
    vtkGenericParamInterface * m_paramInterface;
};

#endif
