// Author: Houssem-Eddine Gueziri

#ifndef __SecondaryUSAcquisition_h_
#define __SecondaryUSAcquisition_h_

// Qt includes
#include <QString>
#include <QComboBox>
#include <QWidget>
#include <QList>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

// IBIS includes
#include "pediclescrewnavigationplugininterface.h"
#include "usacquisitionobject.h"
#include "sceneobject.h"
#include "ibisapi.h"

class PedicleScrewNavigationPluginInterface;
class USAcquisitionObject;

class SecondaryUSAcquisition : public QWidget
{
public:

    SecondaryUSAcquisition(PedicleScrewNavigationPluginInterface * interf=nullptr);
    ~SecondaryUSAcquisition();

    void setPluginInterface(PedicleScrewNavigationPluginInterface * interf) { m_pluginInterface = interf; }
    void setGlobalLayout(QBoxLayout *);
    void addNewEntry();
    void removeLastEntry();
    void removeEntryAt(int);
    void addUSAcquisition(int);
    void getValidUSAcquisitions(QList< USAcquisitionObject * > &);

private:

    QList< QHBoxLayout * > m_layouts;
    QList< QLabel * > m_labels;
    QList< QComboBox * > m_comboboxes;

    int m_numberOfAcquisitions; //TODO: not sure to keep
    QBoxLayout * m_globalLayout;
    PedicleScrewNavigationPluginInterface * m_pluginInterface;

private:

    void fillComboBox(QComboBox *);
};


#endif
