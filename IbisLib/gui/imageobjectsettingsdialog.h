/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef IMAGEOBJECTSETTINGSDIALOG_H
#define IMAGEOBJECTSETTINGSDIALOG_H

#include <vector>
#include "ui_imageobjectsettingsdialog.h"

class ImageObject;

class ImageObjectSettingsDialog :  public QWidget, public Ui::ImageObjectSettingsDialog
{
    Q_OBJECT

public:

    ImageObjectSettingsDialog( QWidget* parent = 0, Qt::WindowFlags fl = 0 );
    virtual ~ImageObjectSettingsDialog();

    virtual void SetImageObject( ImageObject * obj );
    
public slots:

    virtual void SelectColorTableComboBoxActivated(int);
    virtual void RangeSlidersValuesChanged( double min, double max );
    virtual void ViewBoundingBoxCheckboxToggled(bool);
    virtual void UpdateUI();

protected slots:

	virtual void languageChange();
    
protected:
    
    ImageObject * m_imageObject;
    int m_rangeSlider;

private:

    void AllControlsBlockSignals(bool yes);
    
};

#endif
