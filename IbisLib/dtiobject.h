/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef DTIObject_h_
#define DTIObject_h_

#include "polydataobject.h"
#include "serializer.h"
#include <QVector>

class vtkTubeFilter;
class vtkPolyData;
class DTIObjectSettingsWidget;

class DTIObject : public PolyDataObject
{
public:
        
    static DTIObject * New() { return new DTIObject; }
    vtkTypeMacro(DTIObject,PolyDataObject);
    
    DTIObject();
    virtual ~DTIObject();
    
    virtual void Serialize( Serializer * ser );
    virtual bool Setup( View * view );
    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets);

    void PrepareDTITracks();
    bool GetGenerateTubes() { return m_generateTubes; }
    void SetGenerateTubes( bool gen );
    int GetTubeResolution() {return m_tubeResolution;}
    void SetTubeResolution( int res );
    double GetTubeRadius() {return m_tubeRadius;}
    void SetTubeRadius( double );
    DTIObjectSettingsWidget * CreateDTISettingsWidget(QWidget * parent);

protected:
        
    vtkTubeFilter *m_tubeFilter;
    bool m_generateTubes;
    double m_tubeRadius;
    int m_tubeResolution;
    vtkPolyData * m_originalData;
};

ObjectSerializationHeaderMacro( DTIObject );

#endif
