/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "automaticgui.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include "vtkGenericParam.h"
#include <string>

AutomaticGui::AutomaticGui( vtkGenericParamInterface * params, QWidget * parent )
        : m_paramInterface(params)
{
    QVBoxLayout * layout = new QVBoxLayout( this );

    for( int paramIndex = 0; paramIndex < m_paramInterface->GetNumberOfParams(); ++paramIndex )
    {
        vtkGenericParam * p = m_paramInterface->GetParam( paramIndex );

        // Try it as a combo param first
        vtkComboParam * combop = vtkComboParam::SafeDownCast( p );
        if( combop )
        {
            ComboParamGui * comboGui = new ComboParamGui( combop, this );
        }
    }

    layout->addStretch();
    layout->setContentsMargins( 0, 0, 0, 0 );
}

ComboParamGui::ComboParamGui( vtkComboParam * param, QWidget * parent )
    : ParamGui( parent )
    , m_param( param )
{
    QHBoxLayout * layout = new QHBoxLayout();
    parent->layout()->addItem( layout );
    QLabel * label = new QLabel( parent );
    label->setText( m_param->GetParamName() );
    m_comboBox = new QComboBox( parent );
    std::vector< std::string > validChoices;
    m_param->GetValidChoices( validChoices );
    for( unsigned i = 0; i < validChoices.size(); ++i )
    {
        m_comboBox->addItem( QString( validChoices[i].c_str() ) );
    }
    layout->addWidget( label );
    layout->addWidget( m_comboBox );
    connect( m_comboBox, SIGNAL(activated(int)), this, SLOT(OnComboBoxActivated(int)) );
    Update();
}

void ComboParamGui::OnComboBoxActivated( int index )
{
    m_param->SetCurrentChoiceIndex( index );
    Update();
}

void ComboParamGui::Update()
{
    m_comboBox->blockSignals( true );
    m_comboBox->setCurrentIndex( m_param->GetCurrentChoiceIndex() );
    m_comboBox->blockSignals( false );
}
