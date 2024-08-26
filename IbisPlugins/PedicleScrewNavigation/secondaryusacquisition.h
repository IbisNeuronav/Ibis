// Author: Houssem-Eddine Gueziri

#ifndef SECONDARYUSACQUISITION_H
#define SECONDARYUSACQUISITION_H

// Qt includes
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

// IBIS includes
#include "ibisapi.h"
#include "pediclescrewnavigationplugininterface.h"
#include "sceneobject.h"
#include "usacquisitionobject.h"

class PedicleScrewNavigationPluginInterface;
class USAcquisitionObject;

class SecondaryUSAcquisition : public QWidget
{
public:
    SecondaryUSAcquisition( PedicleScrewNavigationPluginInterface * interf = nullptr );
    ~SecondaryUSAcquisition();

    void setPluginInterface( PedicleScrewNavigationPluginInterface * interf ) { m_pluginInterface = interf; }
    void setGlobalLayout( QBoxLayout * );
    void addNewEntry();
    void removeLastEntry();
    void removeEntryAt( int );
    void addUSAcquisition( int );
    void getValidUSAcquisitions( QList<USAcquisitionObject *> & );

private:
    QList<QHBoxLayout *> m_layouts;
    QList<QLabel *> m_labels;
    QList<QComboBox *> m_comboboxes;

    int m_numberOfAcquisitions;  // TODO: not sure to keep
    QBoxLayout * m_globalLayout;
    PedicleScrewNavigationPluginInterface * m_pluginInterface;

private:
    void fillComboBox( QComboBox * );
};

#endif
