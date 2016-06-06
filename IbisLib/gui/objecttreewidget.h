/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#ifndef __ObjectTreeWidget_h_
#define __ObjectTreeWidget_h_

#include <QWidget>
#include <QItemSelection>

namespace Ui {
    class ObjectTreeWidget;
}

class SceneManager;
class SceneObject;
class ObjectTreeModel;

class ObjectTreeWidget : public QWidget
{
    Q_OBJECT

public:

    ObjectTreeWidget( QWidget * parent = 0 );
    virtual  ~ObjectTreeWidget();
    
    void SetSceneManager( SceneManager * man );

signals:

	void ObjectSettingsWidgetChanged( QWidget * );

private slots:

    virtual void DeleteSelectedObject( );
    virtual void AddTransformButtonClicked();
    virtual void AddTransformAllChildrenButtonClicked();
    virtual void AddParentTransformButtonClicked();
    virtual void MarkAsReferenceObject( );
    virtual void ExportObject( );
    virtual void HideWithChildren( );

    void SelectionChanged( QItemSelection sel, QItemSelection desel );
    void DataChanged( QModelIndex index1, QModelIndex index2 );
    void CurrentObjectChanged();
    void AddingObject( SceneObject * parent, int index );
    void SettingsWidgetTabChanged(int);
    
protected:

    void UpdateObjectSettingsWidget();
    void contextMenuEvent( QContextMenuEvent * event );
    
    QWidget * m_currentObjectSettingsWidget;
    SceneManager * m_sceneManager;
    ObjectTreeModel * m_treeModel;
    bool m_workingOnSelection;
    QString lastOpenTabLabel;

private:

    Ui::ObjectTreeWidget * ui;
};

#endif
