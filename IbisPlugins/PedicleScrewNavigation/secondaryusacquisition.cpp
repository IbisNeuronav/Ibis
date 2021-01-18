// Author: Houssem-Eddine Gueziri

#include "secondaryusacquisition.h"

SecondaryUSAcquisition::SecondaryUSAcquisition(PedicleScrewNavigationPluginInterface * interf) :
    m_numberOfAcquisitions(0)
{
    m_pluginInterface = interf;
    m_layouts.clear();
    m_labels.clear();
    m_comboboxes.clear();
}

SecondaryUSAcquisition::~SecondaryUSAcquisition()
{
}

void SecondaryUSAcquisition::setGlobalLayout(QBoxLayout * layout)
{
    m_globalLayout = layout;
}

void SecondaryUSAcquisition::addNewEntry()
{
    QHBoxLayout * layout = new QHBoxLayout;
    QLabel * label = new QLabel;
    label->setText(tr("Socondary US #") + QString::number(m_numberOfAcquisitions + 1));
    label->setStyleSheet("QLabel { font-style: regular }");

    QComboBox * combobox = new QComboBox;
    combobox->setStyleSheet("QComboBox { font-style: regular }");
    this->fillComboBox(combobox);
    
    layout->addWidget(label);
    layout->addWidget(combobox);

    m_labels.append(label);
    m_layouts.append(layout);
    m_comboboxes.append(combobox);

    m_numberOfAcquisitions++;

    if( m_globalLayout )
    {
        m_globalLayout->addLayout(layout);
    }
}

void SecondaryUSAcquisition::removeLastEntry()
{
    this->removeEntryAt(m_numberOfAcquisitions - 1);
}

void SecondaryUSAcquisition::removeEntryAt(int index)
{
    if( index < 0 ) 
        return;

    QHBoxLayout * layout = m_layouts.at(index);
    QLabel * label = m_labels.at(index);
    QComboBox * combobox = m_comboboxes.at(index);
    combobox->clear();

    m_labels.removeAt(index);
    m_comboboxes.removeAt(index);
    m_layouts.removeAt(index);

    layout->removeWidget(label);
    layout->removeWidget(combobox);
    if( m_globalLayout )
    {
        //m_globalLayout->removeItem(layout);
        m_globalLayout->update();
    }

    label->deleteLater();
    combobox->deleteLater();
    layout->deleteLater();

    m_numberOfAcquisitions--;
}

void SecondaryUSAcquisition::fillComboBox(QComboBox * combobox)
{
    if( m_pluginInterface )
    {
        IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
        if( ibisApi )
        {
            QList< USAcquisitionObject * > list;
            ibisApi->GetAllUSAcquisitionObjects(list);
            for( USAcquisitionObject * sceneObject : list )
            {
                if( combobox->count() == 0 )
                {
                    combobox->addItem(sceneObject->GetName(), QVariant(sceneObject->GetObjectID()));
                }
                else if( combobox->count() == 1 )
                {
                    int currentItemId = combobox->itemData(combobox->currentIndex()).toInt();
                    if( currentItemId == IbisAPI::InvalidId )
                    {
                        combobox->clear();
                    }
                    combobox->addItem(sceneObject->GetName(), QVariant(sceneObject->GetObjectID()));
                }
                else
                {
                    combobox->addItem(sceneObject->GetName(), QVariant(sceneObject->GetObjectID()));
                }   
            }
        }
        
        if( combobox->count() == 0 )
        {
            combobox->addItem(tr("None"), QVariant(IbisAPI::InvalidId));
        }
    }
}

void SecondaryUSAcquisition::addUSAcquisition(int objectId)
{
    if( !m_pluginInterface )
        return;
    
    IbisAPI * ibisApi = m_pluginInterface->GetIbisAPI();
    if( !ibisApi )
        return;

    SceneObject * sceneObject = ibisApi->GetObjectByID(objectId);
    for( QComboBox * cb : m_comboboxes )
    {
        if( cb->count() == 0 )
        {
            cb->addItem(sceneObject->GetName(), QVariant(objectId));
        }
        else if( cb->count() == 1 )
        {
            int currentItemId = cb->itemData(cb->currentIndex()).toInt();
            if( currentItemId == IbisAPI::InvalidId )
            {
                cb->clear();
            }
            cb->addItem(sceneObject->GetName(), QVariant(objectId));
        }
        else
        {
            cb->addItem(sceneObject->GetName(), QVariant(objectId));
        }
    }
}