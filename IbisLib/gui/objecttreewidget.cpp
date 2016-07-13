/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "objecttreewidget.h"
#include "ui_objecttreewidget.h"

#include <QTreeView>
#include <QTabWidget>
#include <QAction>
#include <QMenu>
#include <QContextMenuEvent>

#include "scenemanager.h"
#include "imageobject.h"
#include "mincinfowidget.h"
#include "objecttreemodel.h"

ObjectTreeWidget::ObjectTreeWidget( QWidget* parent )
    : QWidget(parent)
    , ui(new Ui::ObjectTreeWidget)
    , m_sceneManager(0)
    , m_treeModel(0)
    , m_workingOnSelection(false)
    , m_currentObjectSettingsWidget(0)
{
    ui->setupUi( this );

    ui->deleteObjectButton->setEnabled(false);
}

ObjectTreeWidget::~ObjectTreeWidget()
{
    if (m_treeModel)
        delete m_treeModel;
}

void ObjectTreeWidget::SetSceneManager( SceneManager * man )
{
    if( m_sceneManager )
    {
        m_sceneManager->disconnect( this );
    }

    m_sceneManager = man;

    if( m_sceneManager )
    {
        m_treeModel = new ObjectTreeModel( m_sceneManager );
        ui->treeView->setModel( m_treeModel );
        ui->treeView->expandToDepth( 2 );
        connect( ui->treeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(SelectionChanged(QItemSelection,QItemSelection)));
        connect( ui->treeView->model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(DataChanged(QModelIndex,QModelIndex)));
        connect( m_sceneManager, SIGNAL(CurrentObjectChanged()), this, SLOT(CurrentObjectChanged()) );
        connect( m_sceneManager, SIGNAL(StartAddingObject(SceneObject*,int)), this, SLOT(AddingObject(SceneObject*,int)) );
    }
    else
        ui->treeView->setModel( 0 );
}

void ObjectTreeWidget::SelectionChanged( QItemSelection sel, QItemSelection desel )
{
    if( m_workingOnSelection )
        return;

    // make sure there is not more than one item selected
    Q_ASSERT( sel.indexes().size() <= 1 );

    m_workingOnSelection = true;

    if( sel.indexes().size() == 0 )
        m_sceneManager->SetCurrentObject( 0 );
    else
    {
        QModelIndex currentItem = sel.indexes().at( 0 );
        SceneObject * currentObject = m_treeModel->IndexToObject( currentItem );
        m_sceneManager->SetCurrentObject( currentObject );
    }

    UpdateObjectSettingsWidget();

    m_workingOnSelection = false;
}

void ObjectTreeWidget::DataChanged( QModelIndex index1, QModelIndex index2 )
{
     if( index1.isValid() &&  index2.isValid() )
     {
        QItemSelection sel;
        SceneObject * item = static_cast<SceneObject*>(index1.internalPointer());
        if( !item->IsHidden() )
        {
            sel.select( index1, index2 );
            ui->treeView->selectionModel()->select( sel, QItemSelectionModel::ClearAndSelect );
        }
     }
}

void ObjectTreeWidget::CurrentObjectChanged()
{
    if( m_workingOnSelection )
        return;

    m_workingOnSelection = true;

    // Select current object in tree view
    SceneObject * currentObject = m_sceneManager->GetCurrentObject();
    QItemSelection sel;
    if( currentObject )
    {
        QModelIndex ind = m_treeModel->ObjectToIndex( currentObject );
        if( ind.isValid() )
            sel.select( ind, ind );
    }
    ui->treeView->selectionModel()->select( sel, QItemSelectionModel::ClearAndSelect );

    // change settings dialog
    UpdateObjectSettingsWidget();

    m_workingOnSelection = false;
}

void ObjectTreeWidget::UpdateObjectSettingsWidget()
{
    if( m_currentObjectSettingsWidget )
    {
        m_currentObjectSettingsWidget->close();
        delete m_currentObjectSettingsWidget;
        m_currentObjectSettingsWidget = 0;
    }

    SceneObject * object = m_sceneManager->GetCurrentObject( );
    if( object )
    {
        m_currentObjectSettingsWidget = object->CreateSettingsDialog( this->parentWidget() );
        QTabWidget *settingsWidget = qobject_cast<QTabWidget *>(m_currentObjectSettingsWidget);
        if (settingsWidget)
        {
            connect(settingsWidget, SIGNAL(currentChanged(int)), this , SLOT(SettingsWidgetTabChanged(int)));
            if (!lastOpenTabLabel.isEmpty())
            {
                bool notFound = true;
                for (int i  = 0; i < settingsWidget->count(); i++ )
                {
                    if (QString::compare(settingsWidget->tabText(i), lastOpenTabLabel) == 0)
                    {
                        settingsWidget->setCurrentIndex(i);
                        notFound  = false;
                    }
                }
                if (notFound)
                    settingsWidget->setCurrentIndex(0);
            }
            else
                settingsWidget->setCurrentIndex(0);
        }
        emit ObjectSettingsWidgetChanged( m_currentObjectSettingsWidget );

        ui->deleteObjectButton->setEnabled( object->IsObjectDeletable() && !object->IsManagedBySystem());
        ui->addTransformButton->setEnabled( object->CanAppendChildren() );

        bool canTransformAllChildren = true;
        for( int i = 0; i < object->GetNumberOfChildren(); ++i )
        {
            if( !object->GetChild(i)->CanChangeParent() )
            {
                canTransformAllChildren = false;
                break;
            }
        }
        ui->addTransformAllChildrenButton->setEnabled( object->GetNumberOfChildren() != 0 && canTransformAllChildren );

        SceneObject * parent = object->GetParent();
        ui->addParentTransformButton->setEnabled( parent && parent->CanAppendChildren() );
    }
    else
    {
        ui->deleteObjectButton->setEnabled( false );
        ui->addTransformButton->setEnabled( false );
        ui->addTransformAllChildrenButton->setEnabled( false );
        ui->addParentTransformButton->setEnabled( false );
    }
}

void ObjectTreeWidget::SettingsWidgetTabChanged(int index)
{
    QTabWidget *settingsWidget = qobject_cast<QTabWidget *>(m_currentObjectSettingsWidget);
    if (settingsWidget)
    {
        lastOpenTabLabel = settingsWidget->tabText(index);
    }
}

void ObjectTreeWidget::AddingObject( SceneObject * parent, int index )
{
    QModelIndex ind = m_treeModel->ObjectToIndex( parent );
    if( ind.isValid() )
    {
        ui->treeView->expand( ind );
    }
}

void ObjectTreeWidget::contextMenuEvent( QContextMenuEvent * event )
{
    SceneObject * currentObj = m_sceneManager->GetCurrentObject();
    if( currentObj )
    {
        // Make sur there is at least one of the menu entry that will be valid
        if( currentObj->IsObjectDeletable() || currentObj->IsExportable() ||  m_sceneManager->CanBeReference( currentObj) )
        {
            QMenu contextMenu;
            contextMenu.addAction(tr(""));
            if( currentObj->IsExportable() )
                contextMenu.addAction( tr("Export"), this, SLOT(ExportObject()) );
            if( currentObj->IsObjectDeletable() && !currentObj->IsManagedBySystem() )
                contextMenu.addAction( tr("Delete"), this, SLOT(DeleteSelectedObject()) );
            if( currentObj->IsHidable() && currentObj->GetNumberOfChildren() > 0 )
                contextMenu.addAction( tr("Hide/Show with Children"), this, SLOT(HideWithChildren()) );
            if( m_sceneManager->CanBeReference( currentObj ) && currentObj != m_sceneManager->GetReferenceDataObject() )
            {
                    contextMenu.addAction( tr("Set as Reference"), this, SLOT(MarkAsReferenceObject()) );
            }
            if( currentObj->IsA( "ImageObject") )
                contextMenu.addAction( tr("Show MINC Info"), this, SLOT(ShowMincInfo()) );
            contextMenu.exec( event->globalPos() );
        }
    }
}

void ObjectTreeWidget::DeleteSelectedObject()
{
    SceneObject * obj = m_sceneManager->GetCurrentObject();
    if( obj )
    {
        m_sceneManager->RemoveObject( obj );
    }
}

void ObjectTreeWidget::ExportObject()
{
    SceneObject * obj = m_sceneManager->GetCurrentObject();
    if( obj && obj->IsExportable() )
        obj->Export();
}


void ObjectTreeWidget::HideWithChildren()
{
    SceneObject * obj = m_sceneManager->GetCurrentObject();
    if( obj && obj->IsHidable() )
        obj->SetHiddenWithChildren( obj->IsHidden()? false:true );
}

void ObjectTreeWidget::AddTransformButtonClicked()
{
    SceneObject * obj = m_sceneManager->GetCurrentObject();
    if( obj && obj->CanAppendChildren() )
    {
        SceneObject * newChild = SceneObject::New();
        newChild->SetName( "Transform" );
        m_sceneManager->AddObject( newChild, obj );
        m_sceneManager->SetCurrentObject( newChild );
    }
}

void ObjectTreeWidget::AddTransformAllChildrenButtonClicked()
{
    SceneObject * obj = m_sceneManager->GetCurrentObject();
    if( obj && obj->CanAppendChildren() )
    {
        SceneObject * newChild = SceneObject::New();
        newChild->SetName( "Transform" );
        m_sceneManager->AddObject( newChild, obj );
        m_sceneManager->SetCurrentObject( newChild );

        // attach all prev children to the new transform
        while( obj->GetNumberOfChildren() > 1 )
        {
            m_sceneManager->ChangeParent( obj->GetChild(0), newChild, newChild->GetNumberOfChildren() );
        }
    }
}

void ObjectTreeWidget::AddParentTransformButtonClicked()
{
    SceneObject * obj = m_sceneManager->GetCurrentObject();
    SceneObject * parent = obj->GetParent();
    if( parent && obj && parent->CanAppendChildren() )
    {
        SceneObject * newTransform = SceneObject::New();
        newTransform->SetName( "Transform" );
        m_sceneManager->AddObject( newTransform, parent );
        m_sceneManager->SetCurrentObject( newTransform );
        m_sceneManager->ChangeParent( obj, newTransform, 0 );
    }
}

void ObjectTreeWidget::MarkAsReferenceObject( )
{
    SceneObject * currentObj = m_sceneManager->GetCurrentObject();
    if( m_sceneManager->CanBeReference( currentObj ) )
    {
        m_sceneManager->SetReferenceDataObject( currentObj );
    }
}

void ObjectTreeWidget::ShowMincInfo( )
{
    ImageObject *img = ImageObject::SafeDownCast( m_sceneManager->GetCurrentObject() );
    if( img )
        img->ShowMincInfo();
}
