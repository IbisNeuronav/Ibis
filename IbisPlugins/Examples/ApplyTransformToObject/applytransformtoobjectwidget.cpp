#include "applytransformtoobjectwidget.h"
#include "ui_applytransformtoobjectwidget.h"

#include "vtkTransform.h"
#include "vtkMatrix4x4.h"
#include "vtkSmartPointer.h"
#include "ibisapi.h"
#include "guiutilities.h"
#include "sceneobject.h"
#include "vtkQtMatrixDialog.h"

ApplyTransformToObjectWidget::ApplyTransformToObjectWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApplyTransformToObjectWidget)
{
    ui->setupUi(this);
    m_matrixDialog = nullptr;
    m_selectedObject = nullptr;
}

ApplyTransformToObjectWidget::~ApplyTransformToObjectWidget()
{
    if( m_matrixDialog )
    {
        m_matrixDialog->close();
        m_matrixDialog = 0;
    }
    delete ui;
}

void ApplyTransformToObjectWidget::on_transformPushButton_clicked()
{
    Q_ASSERT(m_selectedObject);
    vtkTransform * localTransform = m_selectedObject->GetLocalTransform();
    bool readOnly = !m_selectedObject->CanEditTransformManually();
    m_matrixDialog = new vtkQtMatrixDialog( readOnly, 0 );
    m_matrixDialog->setWindowTitle( "Edit matrix" );
    m_matrixDialog->setAttribute( Qt::WA_DeleteOnClose );
    m_matrixDialog->SetMatrix( localTransform->GetMatrix() );
    m_matrixDialog->show();
    connect( m_matrixDialog, SIGNAL(MatrixModified()), m_selectedObject, SLOT(NotifyTransformChanged()) );
    connect( m_matrixDialog, SIGNAL(destroyed()), this, SLOT(EditMatrixDialogClosed()) );
}

void ApplyTransformToObjectWidget::on_sceneObjectsComboBox_currentIndexChanged(int index)
{
    int objectId = GuiUtilities::ObjectIdFromObjectComboBox( ui->sceneObjectsComboBox, index );
    Q_ASSERT( objectId > IbisAPI::InvalidId );
    m_selectedObject = m_pluginInterface->GetIbisAPI()->GetObjectByID( objectId );
}

void ApplyTransformToObjectWidget::SetInterface( ApplyTransformToObjectPluginInterface *intface )
{
    m_pluginInterface = intface;
    this->UpdateUI();
}

void ApplyTransformToObjectWidget::UpdateUI()
{
    Q_ASSERT(m_pluginInterface);
    IbisAPI *ibisAPI = m_pluginInterface->GetIbisAPI();
    Q_ASSERT(ibisAPI);

    QList<SceneObject*> objects;
    ibisAPI->GetAllUserObjects( objects );
    GuiUtilities::UpdateSceneObjectComboBox( ui->sceneObjectsComboBox, objects, ibisAPI->GetCurrentObject()->GetObjectID() );
    m_selectedObject = ibisAPI->GetCurrentObject();
}

void ApplyTransformToObjectWidget::EditMatrixDialogClosed()
{
    m_matrixDialog = 0;
}
