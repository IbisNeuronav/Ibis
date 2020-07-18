#ifndef __SCREWPROPERTIES_H__
#define __SCREWPROPERTIES_H__

#include <vtkSmartPointer.h>
#include "serializer.h"

class vtkActor;
class vtkLineSource;
class vtkPolyData;

class Screw
{

public:
    Screw();
    Screw(double axPos[3], double axOri[3], double sagPos[3], double sagOri[3]);
    Screw( const Screw * in );
    ~Screw() {}

    virtual void Serialize( Serializer * ser );

    Screw operator= (Screw in);

    void GetAxialPosistion(double (&out)[3]);
    void GetAxialOrientation(double (&out)[3]);
    void GetSagittalPosistion(double (&out)[3]);
    void GetSagittalOrientation(double (&out)[3]);
    void GetPointerOrientation(double (&out)[3]);
    void GetPointerPosition(double (&out)[3]);
    bool IsCoordinateWorldTransform() { return m_useWorldTransformCoordinate; }
    bool IsCoordinateLocalTransform() { return !m_useWorldTransformCoordinate; }

    void SetAxialPosistion(double in[3]);
    void SetAxialOrientation(double in[3]);
    void SetSagittalPosistion(double in[3]);
    void SetSagittalOrientation(double in[3]);
    void SetPointerOrientation(double in[3]);
    void SetPointerPosition(double in[3]);
    void SetCoordinateTransformToWorld() { m_useWorldTransformCoordinate = true; }
    void SetCoordinateTransformToLocal() { m_useWorldTransformCoordinate = false; }
    void SetUseWorldTransformCoordinate(bool useWorld) { m_useWorldTransformCoordinate = useWorld; }

    std::string GetName() { return m_name; }
    void SetName(std::string name) { m_name = name; }

    // getters
    vtkLineSource * GetAxialLeftLineSource() { return m_axialLeftLineSource; }
    vtkLineSource * GetAxialRightLineSource() { return m_axialRightLineSource; }

    vtkSmartPointer<vtkActor> GetAxialActor() { return m_axialActor; }
    vtkSmartPointer<vtkActor> GetSagittalActor() { return m_sagittalActor; }

    double GetScrewLength() {return m_length; }
    double GetScrewDiameter() {return m_diameter; }
    double GetScrewTipSize() { return m_tipSize; }

    // setters
    void SetAxialLeftLineSource(vtkLineSource * line) { m_axialLeftLineSource = line; }
    void SetAxialRightLineSource(vtkLineSource * line) { m_axialRightLineSource = line; }

    void SetScrewPolyData(vtkPolyData * polyData) { m_sagittalScrewPolyData = polyData; }

    void SetAxialActor(vtkSmartPointer<vtkActor> actor) { m_axialActor = actor; }
    void SetSagittalActor(vtkSmartPointer<vtkActor> actor) { m_sagittalActor = actor; }

    void SetScrewProperties(double length, double diameter, double tipSize);
    static void GetScrewPolyData(double length, double diameter, double tipSize, vtkSmartPointer<vtkPolyData> &polyData);
    void GetScrewPolyData(vtkSmartPointer<vtkPolyData> polyData);

    void Update();

    void PrintSelf();

private:

    void UpdateInternal();

private:

    std::string m_name;

    double m_axialPosition[3];
    double m_axialOrientation[3];
    double m_sagittalPosition[3];
    double m_sagittalOrientation[3];

    double m_pointerPosition[3];
    double m_pointerOrientation[3];

    bool   m_useWorldTransformCoordinate;
    double m_length;
    double m_diameter;
    double m_tipSize;

    vtkSmartPointer<vtkActor> m_axialActor;
    vtkSmartPointer<vtkActor> m_sagittalActor;

    vtkLineSource * m_axialLeftLineSource;
    vtkLineSource * m_axialRightLineSource;

    vtkPolyData * m_sagittalScrewPolyData;

};

ObjectSerializationHeaderMacro( Screw );

#endif // __SCREWPROPERTIES_H__
