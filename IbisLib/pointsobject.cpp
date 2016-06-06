/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "pointsobject.h"
#include "pointpropertieswidget.h"
#include "pointcolorwidget.h"
#include "pointrepresentation.h"
#include "pickerobject.h"
#include "pointerobject.h"
#include "polydataobject.h"
#include "scenemanager.h"
#include "application.h"
#include "view.h"
#include "ignsconfig.h"
#include "ignsmsg.h"
#include <QStringList>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include "vtkTransform.h"
#include "vtkTagWriter.h"
#include "vtkActor.h"
#include "vtkMatrix4x4.h"
#include <vtkMath.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkProperty.h>
#include <vtkSmartPointer.h>


ObjectSerializationMacro( PointsObject );

const int PointsObject::InvalidPointIndex = -1;

PointsObject::PointsObject() : SceneObject()
{
    m_pointCoordinates = vtkPoints::New();
    m_selectedPointIndex = InvalidPointIndex;
    m_pointRadius3D = 2.0;
    m_pointRadius2D = 20.0;
    m_hotSpotSize = 0.25 * m_pointRadius2D;
    m_labelSize = 8.0;
    m_enabledColor[0] = 1.0;
    m_enabledColor[1] = 0.7;
    m_enabledColor[2] = 0.0;
    m_disabledColor[0] = 0.7;
    m_disabledColor[1] = 0.7;
    m_disabledColor[2] = 0.6;
    m_selectedColor[0] = 0.0;
    m_selectedColor[1] = 1.0;
    m_selectedColor[2] = 0.0;
    m_opacity = 1.0;
    m_mousePicker = 0;
    m_pickable = false;
    m_enabled = false;
    m_showLabels = true;
    m_maxNumberOfPoints = 1000;
    m_minNumberOfPoints = 0;
    m_computeDistance = false;
    m_lineToPointerTip = 0;
    m_lineToPointerProperty = vtkProperty::New();
    for ( int i = 0; i < 3; i++ )
        m_lineToPointerColor[i] = 1.0;
    m_lineToPointerProperty->SetColor( m_lineToPointerColor[0], m_lineToPointerColor[1], m_lineToPointerColor[2] );
    connect( this, SIGNAL(WorldTransformChangedSignal()), this, SLOT(UpdatePoints()) );
}

PointsObject::~PointsObject()
{
    this->DeleteAllPointsFromScene();
    m_pointCoordinates->Delete();
    if( m_mousePicker != 0 )
    {
        m_mousePicker->ConnectObservers(false);
        m_mousePicker->Delete();
    }
    if( m_lineToPointerTip )
        m_lineToPointerTip->Delete();
    m_lineToPointerProperty->Delete();
}

void PointsObject::Serialize( Serializer * ser )
{
    SceneObject::Serialize(ser);
    bool hidden = this->ObjectHidden;
    this->ObjectHidden = true; // set true in order not to display points while reading all object data is not finished yet

    ::Serialize( ser, "PointsPickable", m_enabled );
    ::Serialize( ser, "PointRadius3D", m_pointRadius3D );
    ::Serialize( ser, "PointRadius2D", m_pointRadius2D );
    ::Serialize( ser, "LabelSize", m_labelSize );
    ::Serialize( ser, "EnabledColor", m_enabledColor, 3 );
    ::Serialize( ser, "DisabledColor", m_disabledColor, 3 );
    ::Serialize( ser, "SelectededColor", m_selectedColor, 3 );
    ::Serialize( ser, "LineToPointerColor", m_lineToPointerColor, 3 );
    ::Serialize( ser, "Opacity", m_opacity );
    ::Serialize( ser, "SelectedPointIndex", m_selectedPointIndex );
    if (this->FullFileName.isEmpty() || this->DataFileName.isEmpty())
    {
        int numberOfPoints;
        double coords[3];
        QString pointName, timeStamp("n/a");
        if (!ser->IsReader())
        {
            numberOfPoints = m_pointCoordinates->GetNumberOfPoints();
            ::Serialize( ser, "NumberOfPoints", numberOfPoints );
            for(int i = 0; i < numberOfPoints; i++)
            {
                pointName = m_pointNames.at( i );
                timeStamp = m_timeStamps.at( i );
                m_pointCoordinates->GetPoint( i, coords );
                QString sectionName = QString( "Point_%1" ).arg(i);
                ser->BeginSection(sectionName.toUtf8().data());
                ::Serialize( ser, "PointName", pointName );
                ::Serialize( ser, "PointCoordinates", coords, 3 );
                ::Serialize( ser, "PointTimeStamp", timeStamp );
                ser->EndSection();
            }
        }
        else
        {
            ::Serialize( ser, "NumberOfPoints", numberOfPoints );
            m_pointNames.clear();
            m_timeStamps.clear();
            m_pointCoordinates->Reset();
            for (int i = 0; i < numberOfPoints; i++)
            {
                QString sectionName = QString( "Point_%1" ).arg(i);
                ser->BeginSection(sectionName.toUtf8().data() );
                ::Serialize( ser, "PointName", pointName );
                ::Serialize( ser, "PointCoordinates", coords, 3 );
                ::Serialize( ser, "PointTimeStamp", timeStamp );
                ser->EndSection();
                m_pointNames.push_back(pointName);
                m_timeStamps.push_back(timeStamp);
                m_pointCoordinates->InsertNextPoint(coords);
            }
            m_pickable = m_enabled;
            m_lineToPointerProperty->SetColor( m_lineToPointerColor[0], m_lineToPointerColor[1], m_lineToPointerColor[2] );
        }
    }
    this->ObjectHidden = hidden;
}

void PointsObject::PostSceneRead()
{
    if( this->ObjectHidden )
        this->Hide();
    else
        this->Show();
    UpdatePickability();
}

void PointsObject::Export()
{
    Q_ASSERT( this->GetManager() );

    if (m_pointCoordinates->GetNumberOfPoints() > 0)
    {
        QString workingDirectory(this->GetManager()->GetSceneDirectory());
        if( !QFile::exists( workingDirectory ) )
        {
            workingDirectory = QDir::homePath() + IGNS_CONFIGURATION_SUBDIRECTORY;
            QDir configDir( workingDirectory );
            if( !configDir.exists( ) )
            {
                configDir.mkdir( workingDirectory );
            }
        }
        QString name(this->Name);
        name.append(".tag");
        QStringList filenames;
        QFileInfo info;
        QFileDialog fdialog(0, "Save Tag File", workingDirectory, "*.tag");
        fdialog.selectFile(name);
        if (fdialog.exec())
            filenames = fdialog.selectedFiles();
        if(!filenames.isEmpty() && !filenames[0].isEmpty() )
        {
            info.setFile(filenames[0]);
            QString dirPath = info.dir().absolutePath();
            QFileInfo info1( dirPath );
            if (!info1.isWritable())
            {
                QString accessError = IGNS_MSG_NO_WRITE + dirPath;
                QMessageBox::warning( 0, "Error: ", accessError, 1, 0 );
                return;
            }
            if (QString::compare(this->Name, info.baseName()))
                this->SetName(info.baseName());
            if (QString::compare(this->FullFileName, info.absoluteFilePath()))
            {
                this->SetFullFileName(info.absoluteFilePath());
                this->SetDataFileName(info.fileName());
            }
        }
        std::vector<std::string> pointNames;
        std::vector<std::string> timeStamps;
        for (int i = 0; i < m_pointCoordinates->GetNumberOfPoints(); i++)
        {
            pointNames.push_back(m_pointNames.at(i).toUtf8().data());
            timeStamps.push_back(m_timeStamps.at(i).toUtf8().data());
        }
        vtkTagWriter * writer = vtkTagWriter::New();
        writer->SetFileName( info.absoluteFilePath().toUtf8().data() );
        writer->SetPointNames( pointNames );
        writer->AddVolume( m_pointCoordinates, this->Name.toUtf8().data() );
        writer->SetTimeStamps(timeStamps);
        writer->Write();
        writer->Delete();
    }
    else
        QMessageBox::warning( 0, "Error: ", "There are no points to save.", 1, 0 );
}

bool PointsObject::Release( View * view)
{
    this->DeleteAllPointsFromScene();
    this->SetPickable( false  );
    return SceneObject::Release( view );
}

void PointsObject::Hide()
{
    this->DeleteAllPointsFromScene();
    this->UpdatePickability();
}

void PointsObject::Show()
{
    if( m_pointList.isEmpty() )
        this->SetPointsInScene();
    if( this->GetManager()->GetCurrentObject() == this )
        this->UpdatePickability();
}

void PointsObject::ShallowCopy(SceneObject *source)
{
    if (this == source)
        return;
    PointsObject *pObj = PointsObject::SafeDownCast(source);
    m_pointRadius3D = pObj->Get3DRadius();
    m_pointRadius2D = pObj->Get2DRadius();
    m_labelSize = pObj->GetLabelSize();
    m_pickable = pObj->GetPickable();
    m_enabled = pObj->GetEnabled();
    pObj->GetEnabledColor(m_enabledColor);
    pObj->GetDisabledColor(m_disabledColor);
    pObj->GetSelectedColor(m_selectedColor);
    m_opacity = pObj->GetOpacity();
    m_hotSpotSize = pObj->GetHotSpotSize();
    m_pointNames.clear();
    for (int i = 0; i < pObj->GetPointsNames()->count(); i++)
        m_pointNames.push_back(pObj->GetPointsNames()->value(i));
    m_timeStamps.clear();
    for (int i = 0; i < pObj->GetTimeStamps()->count(); i++)
        m_timeStamps.push_back(pObj->GetTimeStamps()->value(i));
    m_pointCoordinates->Reset();
    m_pointCoordinates->DeepCopy(pObj->GetPoints());
}

void PointsObject::CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets)
{
    PointPropertiesWidget * props = new PointPropertiesWidget( parent );
    props->setAttribute( Qt::WA_DeleteOnClose );
    props->SetPointsObject( this );
    props->setObjectName( "Properties" );
    widgets->append( props );
    PointColorWidget * color = new PointColorWidget( parent );
    color->setAttribute( Qt::WA_DeleteOnClose );
    color->SetPointsObject( this );
    color->setObjectName( "Color" );
    widgets->append( color );
    m_pickable = m_enabled;
    if( !this->IsHidden() )
    {
        this->Show();
        this->SetPickable(m_pickable);
    }
}

// actions on the object
void PointsObject::SetPoints( vtkPoints *pt )
{
    m_pointCoordinates->Reset();
    if ( pt )
        m_pointCoordinates->DeepCopy( pt );
    // if this is not hidden, the caller will have to call Show() as we may not know point names yet
}

void PointsObject::SetPointsNames( QStringList & names )
{
    m_pointNames.clear();
    QStringList::Iterator it;
    for ( it = names.begin(); it != names.end(); ++it )
        m_pointNames.push_back( *it );
}

void PointsObject::AddPoint( const QString &name, double coords[3], bool show )
{
    //transform coords to local
    double localCoords[3];
    this->WorldToLocal( coords, localCoords );
    if( this->GetNumberOfPoints() == m_maxNumberOfPoints )
        return;
    m_pointNames.append(name);
    m_pointCoordinates->InsertNextPoint( localCoords );
    m_timeStamps.append("n/a");
    if (!this->IsHidden() && show)
    {
        this->AddPointToScene( name, localCoords );
        emit PointAdded();
    }
}

void PointsObject::SetPickable( bool pickable )
{
    m_pickable = pickable;
    if( GetManager() )
        UpdatePickability();
}

void PointsObject::SetEnabled( bool enabled )
{
    m_enabled = enabled;
    m_pickable = m_enabled;
    if( GetManager() )
        UpdatePickability();
}

void PointsObject::SetPointsInScene()
{
    int n = this->GetNumberOfPoints();
    if( n > 0 )
    {
        for (int i = 0; i < n; i++)
        {
            this->AddPointToScene( m_pointNames.at( i ), m_pointCoordinates->GetPoint(i) );
        }
        if ( m_selectedPointIndex != InvalidPointIndex )
            this->SetSelectedPoint( m_selectedPointIndex );
        emit Modified();
    }
}

void PointsObject::AddPointToScene( const QString & label, double * coords )
{
    Q_ASSERT( this->GetManager() );

    PointRepresentation *newPoint = PointRepresentation::New(); // this is not a SceneObject, so it needs WorldTransform, not Local transform
    newPoint->SetSceneManager(this->GetManager());
    newPoint->SetWorldTransform(this->GetWorldTransform());
    if (m_enabled)
        newPoint->SetPropertyColor(m_enabledColor);
    else
        newPoint->SetPropertyColor(m_disabledColor);
    newPoint->CreatePointRepresentation(m_pointRadius2D, m_pointRadius3D);
    newPoint->SetPosition(coords);
    int index = m_pointList.count();
    newPoint->SetPointIndex( index );
    m_pointList.append(newPoint);
    newPoint->SetLabel(label.toUtf8().data());
    newPoint->SetPickable(m_pickable);
    newPoint->ShowLabel( m_showLabels );
    newPoint->Update();
}

void PointsObject::DeleteAllPointsFromScene()
{
    if( m_pointList.isEmpty() )
        return;
    PointList::iterator it = m_pointList.begin();
    for(; it != m_pointList.end(); ++it)
    {
        (*it)->Delete();
    }
    m_pointList.clear();
    emit Modified();
}

void PointsObject::Reset()
{
    this->DeleteAllPointsFromScene();
    m_pointCoordinates->Reset();
    m_pointNames.clear();
    m_timeStamps.clear();
}

void PointsObject::SetSelectedPoint( int index, bool movePlanes )
{
    if( this->IsHidden() )
        return;
    int n = m_pointCoordinates->GetNumberOfPoints();
    Q_ASSERT( index != InvalidPointIndex && index < m_pointCoordinates->GetNumberOfPoints() );
    if( index != InvalidPointIndex )
    {
        int previouslySelectedPointIndex = m_selectedPointIndex;
        m_selectedPointIndex = index;
        if( previouslySelectedPointIndex == m_pointCoordinates->GetNumberOfPoints() )
            previouslySelectedPointIndex--; // point has been removed
        if( previouslySelectedPointIndex != InvalidPointIndex )
            this->UpdatePointInScene( previouslySelectedPointIndex, false );
        this->UpdatePointInScene( m_selectedPointIndex, movePlanes );
        emit Modified();
    }
}

int PointsObject::FindPoint(vtkActor **actor, double *pos)
{
    Q_ASSERT( this->GetManager() );

    if( m_pointList.isEmpty() )
        return InvalidPointIndex;

    int n = m_pointList.count();
    int pointIndex = InvalidPointIndex;
    double pointPosition[3];
    PointRepresentation *point;
    // first check if 3D actor was picked
    int i = 0;
    while(pointIndex == InvalidPointIndex && i < n)
    {
        point = m_pointList.value(i);
        if (*actor == point->GetPointActor(3))
        {
            return i;
        }
        i++;
    }

    // Now see if any of 2D actors are picked
    int viewType = this->GetManager()->GetCurrentView();
    for (int i = 0; i < n; i++)
    {
        point = m_pointList.value(i);
        point->GetPosition(pointPosition);
        switch (viewType)
        {
        case SAGITTAL_VIEW_TYPE: //x fixed
            if (fabs(pointPosition[0] - pos[0]) < m_hotSpotSize)
            {
                if (fabs(pos[1]-pointPosition[1]) < m_hotSpotSize &&
                    fabs(pos[2]-pointPosition[2]) < m_hotSpotSize)
                {
                    *actor = point->GetPointActor(SAGITTAL_VIEW_TYPE);
                    return i;
                }
            }
            break;
        case CORONAL_VIEW_TYPE: // y fixed
            if (fabs(pointPosition[1]- pos[1]) < m_hotSpotSize)
            {
                if (fabs(pos[0]-pointPosition[0]) < m_hotSpotSize &&
                    fabs(pos[2]-pointPosition[2]) < m_hotSpotSize)
                {
                    *actor = point->GetPointActor(CORONAL_VIEW_TYPE);
                    return i;
                }
            }
            break;
        case TRANSVERSE_VIEW_TYPE: // z fixed
            if (fabs(pointPosition[2] - pos[2]) < m_hotSpotSize)
            {
                if (fabs(pos[1]-pointPosition[1]) < m_hotSpotSize &&
                    fabs(pos[0]-pointPosition[0]) < m_hotSpotSize)
                {
                    return i;
                 }
            }
            break;
        case THREED_VIEW_TYPE:
        default:
            break;
        }
    }
    return InvalidPointIndex;
}

void PointsObject::RotatePointsInScene( )
{
    Q_ASSERT( this->GetManager() );
    SceneObject *refObj = (SceneObject*)this->GetManager()->GetReferenceDataObject();
    if( refObj && m_pointList.count() > 0 )
    {
        for( int i = 0; i < m_pointList.count(); i++ )
        {
            m_pointList.at(i)->SetCuttingPlaneTransform( refObj->GetWorldTransform() );
            m_pointList.at(i)->SetCylindersTransform( refObj->GetWorldTransform()->GetMatrix() );
        }
        emit Modified();
    }
}

void PointsObject::UpdatePoints()
{
    if( m_pointList.count() > 0 )
    {
        for( int i = 0; i < m_pointList.count(); i++ )
        {
            this->UpdatePointInScene( i );
        }
        if( m_selectedPointIndex != InvalidPointIndex && m_selectedPointIndex < this->GetNumberOfPoints() )
            this->SetSelectedPoint( m_selectedPointIndex );
        else
            this->SetSelectedPoint( 0 );
        emit Modified();
    }
    m_pointCoordinates->Modified();
}

void PointsObject::UpdateSettingsWidget()
{

}

// properties
void PointsObject::Set3DRadius( double r )
{
    if( m_pointRadius3D == r )
        return;
    if( r < MIN_RADIUS )
        r = MIN_RADIUS;
    if( r > MAX_RADIUS )
        r = MAX_RADIUS;
    m_pointRadius3D = r;
    this->UpdatePoints();
}

void PointsObject::Set2DRadius( double r )
{
    if( m_pointRadius2D == r )
        return;
    m_pointRadius2D = r;
    m_hotSpotSize = 0.25 * m_pointRadius2D;
    this->UpdatePoints();
}

void PointsObject::SetLabelSize( double s )
{
    if( m_labelSize == s )
        return;
    if( s < MIN_LABEl_SIZE )
        s = MIN_LABEl_SIZE;
    if( s > MAX_LABEL_SIZE )
        s = MAX_LABEL_SIZE;
    m_labelSize = s;
    this->UpdatePoints();
}

void PointsObject::SetEnabledColor( double color[3] )
{
    if( !( (m_enabledColor[0] == color[0]) && (m_enabledColor[1] == color[1]) && (m_enabledColor[2] == color[2]) ) )
    {
        for ( int i = 0; i < 3; i++ )
            m_enabledColor[i] = color[i];
        this->UpdatePoints();
    }
}

void PointsObject::GetEnabledColor(double color[3])
{
    for (int i = 0; i < 3; i++)
        color[i] = m_enabledColor[i];
}

void PointsObject::SetDisabledColor( double color[3] )
{
    if( !( (m_disabledColor[0] == color[0]) && (m_disabledColor[1] == color[1]) && (m_disabledColor[2] == color[2]) ) )
    {
        for ( int i = 0; i < 3; i++ )
            m_disabledColor[i] = color[i];
        this->UpdatePoints();
    }
}

void PointsObject::GetDisabledColor(double color[3])
{
    for (int i = 0; i < 3; i++)
        color[i] = m_disabledColor[i];
}

void PointsObject::SetSelectedColor( double color[3] )
{
    if( !( (m_selectedColor[0] == color[0]) && (m_selectedColor[1] == color[1]) && (m_selectedColor[2] == color[2]) ) )
    {
        for ( int i = 0; i < 3; i++ )
            m_selectedColor[i] = color[i];
        this->UpdatePoints();
    }
}

void PointsObject::GetSelectedColor(double color[3])
{
    for (int i = 0; i < 3; i++)
        color[i] = m_selectedColor[i];
}

void PointsObject::SetOpacity( double opacity )
{
    if (m_opacity != opacity)
        m_opacity = opacity;
    this->UpdatePoints();
}

void PointsObject::ShowLabels( bool on )
{
    m_showLabels = on;
    this->UpdatePoints();
}

// access to individual points
void PointsObject::RemovePoint(int index)
{
    if( this->GetNumberOfPoints() == m_minNumberOfPoints )
        return;
    Q_ASSERT( index >= 0 && index < m_pointCoordinates->GetNumberOfPoints() );
    vtkPoints *tmpPoints = vtkPoints::New();
    tmpPoints->DeepCopy(m_pointCoordinates);
    int i, n = m_pointCoordinates->GetNumberOfPoints();
    m_pointCoordinates->Reset();
    for( i = 0; i < index; i++ )
    {
        m_pointCoordinates->InsertNextPoint( tmpPoints->GetPoint(i) );
    }
    for( i = index; i < n-1; i++ )
    {
        m_pointCoordinates->InsertNextPoint( tmpPoints->GetPoint(i+1) );
    }
    tmpPoints->Delete();
    m_pointNames.erase(m_pointNames.begin()+index);
    m_timeStamps.erase(m_timeStamps.begin()+index);
    if( !this->IsHidden() )
    {
        PointList::iterator it = m_pointList.begin();
        for( i = 0; it != m_pointList.end(); ++it, i++ )
        {
            if( i == index )
            {
                PointRepresentation *pt = (PointRepresentation*)(*it);
                pt->Delete();
                m_pointList.erase(it);
                break;
            }
        }
    }
    if( m_selectedPointIndex == index )
        m_selectedPointIndex = InvalidPointIndex;
    this->UpdatePoints();
    emit PointRemoved( index );
}

void PointsObject::EnableDisablePoint( int index, bool enable )
{
    if( this->IsHidden() )
        return;
    Q_ASSERT( index >= 0 && index < m_pointList.count() );
    PointRepresentation *pt = m_pointList.at( index );
    pt->SetPickable( enable );
}

void PointsObject::UpdatePointInScene( int index, bool movePlanes )
{
    Q_ASSERT( this->GetManager() );
    if( this->IsHidden() || m_pointList.count() == 0 )
        return;
    Q_ASSERT( index >= 0 && index < m_pointList.count() );
    PointRepresentation *pt = m_pointList.at( index );
    pt->SetLabel( m_pointNames.at(index).toUtf8().data() );
    pt->SetPosition( m_pointCoordinates->GetPoint( index ) );
    pt->SetPointSizeIn3D( m_pointRadius3D );
    pt->SetPointSizeIn2D( m_pointRadius2D );
    pt->SetLabelScale( m_labelSize );
    pt->SetOpacity( m_opacity );
    pt->UpdateLabelPosition();
    pt->ShowLabel( m_showLabels );
    if( index == m_selectedPointIndex )
        pt->SetPropertyColor( m_selectedColor );
    else if( pt->GetPickable() )
        pt->SetPropertyColor( m_enabledColor );
    else
        pt->SetPropertyColor( m_disabledColor );
    double worldPos[3];
    double *pos = m_pointCoordinates->GetPoint( index );
    this->WorldTransform->TransformPoint( pos, worldPos );
    if( movePlanes )
        this->GetManager()->SetCursorWorldPosition(worldPos);
    emit Modified();
}

void PointsObject::SetPointLabel( int index, const QString &label )
{
    m_pointNames.replace(index, label);
    if( !this->IsHidden() )
        this->UpdatePointInScene( index );
    emit Modified();
}

const QString PointsObject::GetPointLabel(int index)
{
    return m_pointNames.at( index );
}

double * PointsObject::GetPointCoordinates( int index )
{
    return m_pointCoordinates->GetPoint( index );
}

void PointsObject::GetPointCoordinates( int index, double coords[3] )
{
    m_pointCoordinates->GetPoint( index, coords );
}

void PointsObject::SetPointCoordinates( int index, double coords[3] )
{
    Q_ASSERT( index >= 0 && index < m_pointCoordinates->GetNumberOfPoints() );
    m_pointCoordinates->SetPoint( index, coords );
    if( m_pointList.count() > 0 )
    {
        m_pointList.at( index )->SetPosition( m_pointCoordinates->GetPoint( index ) );
        if( !this->IsHidden() )
            this->UpdatePointInScene( index );
    }
    emit PointsChanged();
    emit Modified();
}

void PointsObject::SetPointTimeStamp( int index, const QString & stamp)
{
    if( index >= 0 && index < m_pointCoordinates->GetNumberOfPoints() )
        m_timeStamps.replace(index, stamp);
}

void PointsObject::UpdatePickability()
{
    if( m_pickable && !this->IsHidden() )
    {
        // we may have only one pickable set of points at all times
        QList< PointsObject* > all;
        this->GetManager()->GetAllPointsObjects( all );
        if (all.count() > 0)
        {
            for (int i = 0; i < all.count(); i++)
            {
                PointsObject *obj = all.at(i);
                if (obj != this)
                    obj->SetPickable(false);
            }
        }
        if( m_mousePicker == 0 )
        {
            m_mousePicker = PickerObject::New();
            m_mousePicker->SetManager(this->GetManager());
            m_mousePicker->SetSelectedPoints(this);
            m_mousePicker->ConnectObservers(true);
        }
    }
    else
    {
        if( m_mousePicker != 0 )
        {
            m_mousePicker->ConnectObservers(false);
            m_mousePicker->Delete();
            m_mousePicker = 0;
        }
    }
}

void PointsObject::OnCloseSettingsWidget()
{
    this->SetPickable( false );
}

// Distance will be properly computed for points picked on unregistered objects
void PointsObject::ComputeDistanceFromSelectedPointToPointerTip()
{
    SceneManager *manager = this->GetManager();
    Q_ASSERT( manager );
    PointerObject * pointer = manager->GetNavigationPointerObject();
    double pointerPos[3], *pos1 = pointer->GetTipPosition();
    double pointPos[3], *pos2 = this->GetPointCoordinates( m_selectedPointIndex );
    for( int i = 0; i < 3; i++ )
    {
        pointerPos[i] = pos1[i];
        pointPos[i] = pos2[i];
    }
    double tmpPos[3];
    vtkLinearTransform * transform = this->GetLocalTransform();
    transform->TransformPoint( pointPos, tmpPos );
    // convert to world
    double worldCoords[3];
    this->LocalToWorld( tmpPos, worldCoords );
    double distance = sqrt( vtkMath::Distance2BetweenPoints( worldCoords, pointerPos ) );
    // Setup the text and add it to the window
    QString text = QString( "%1" ).arg( distance, -5,'f', 0 );
    manager->SetGenericLabelText( text );
    this->LineToPointerTip( worldCoords, pointerPos );
}

void PointsObject::UpdateDistance()
{
    // Set button color according to navigation pointer state and availability of points to capture
    SceneManager *manager = this->GetManager();
    Q_ASSERT( manager );
    PointerObject * pObj = manager->GetNavigationPointerObject();
    if( pObj && pObj->IsOk() )
    {
        int numPoints = this->GetNumberOfPoints();
        if( numPoints > 0 )
        {
            if( m_computeDistance )
            {
                this->ComputeDistanceFromSelectedPointToPointerTip( );
                manager->EmitShowGenericLabelText( );
            }
        }
    }
}

void PointsObject::EnableComputeDistance( bool enable )
{
    m_computeDistance = enable;
    if( m_computeDistance )
    {
        m_lineToPointerTip = PolyDataObject::New();
        m_lineToPointerTip->SetListable( false );
        m_lineToPointerTip->SetCanEditTransformManually( false );
        m_lineToPointerTip->SetObjectManagedByTracker( true );
        m_lineToPointerTip->SetProperty( m_lineToPointerProperty );
        double pointPos[3];
        this->GetPointCoordinates( m_selectedPointIndex, pointPos );
        double worldCoords[3];
        this->LocalToWorld( pointPos, worldCoords );
        this->LineToPointerTip( worldCoords, worldCoords );
        this->GetManager()->AddObject( m_lineToPointerTip );
        connect( &(Application::GetInstance()), SIGNAL(IbisClockTick()), this, SLOT(UpdateDistance()) );
    }
    else
    {
        disconnect( &(Application::GetInstance()), SIGNAL(IbisClockTick()), this, SLOT(UpdateDistance()) );
        this->GetManager()->RemoveObject( m_lineToPointerTip );
        m_lineToPointerTip->Delete();
        m_lineToPointerTip = 0;
    }
}

void PointsObject::LineToPointerTip( double selectedPoint[3], double pointerTip[3] )
{
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    points->SetNumberOfPoints(2);
    points->SetPoint(0, selectedPoint[0], selectedPoint[1], selectedPoint[2] );
    points->SetPoint(1, pointerTip[0], pointerTip[1], pointerTip[2] );

    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
    vtkIdType pts[2];
    pts[0] = 0;
    pts[1] = 1;
    cells->InsertNextCell(2,pts);

    vtkSmartPointer<vtkPolyData> linesPolyData = vtkSmartPointer<vtkPolyData>::New();
    linesPolyData->SetPoints(points);
    linesPolyData->SetLines(cells);

    m_lineToPointerTip->SetPolyData( linesPolyData );
    emit Modified();
}

void PointsObject::SetLineToPointerColor( double color[3] )
{
    for ( int i = 0; i < 3; i++ )
        m_lineToPointerColor[i] = color[i];
    if( m_lineToPointerProperty )
        m_lineToPointerProperty->SetColor(color[0], color[1], color[2]);
    emit Modified();
}

void PointsObject::GetLineToPointerColor(  double color[3] )
{
    for ( int i = 0; i < 3; i++ )
        color[i] = m_lineToPointerColor[i];
}
