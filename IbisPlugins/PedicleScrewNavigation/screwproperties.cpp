#include "screwproperties.h"
#include <iostream>
#include <sstream>

#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkLineSource.h>
#include <vtkAppendPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

ObjectSerializationMacro( Screw );

Screw::Screw()
{
    m_axialActor = vtkSmartPointer<vtkActor>::New();
    m_sagittalActor = vtkSmartPointer<vtkActor>::New();

    m_useWorldTransformCoordinate = false;
    m_axialPosition[0] = 0; m_axialPosition[1] = 0; m_axialPosition[2] = 0;
    m_axialOrientation[0] = 0; m_axialOrientation[1] = 0; m_axialOrientation[2] = 0;
    m_sagittalPosition[0] = 0; m_sagittalPosition[1] = 0; m_sagittalPosition[2] = 0;
    m_sagittalOrientation[0] = 0; m_sagittalOrientation[1] = 0; m_sagittalOrientation[2] = 0;
    m_pointerPosition[0] = 0; m_pointerPosition[1] = 0; m_pointerPosition[2] = 0;
    m_pointerOrientation[0] = 0; m_pointerOrientation[1] = -1; m_pointerOrientation[2] = 0;
}

Screw::Screw(double axPos[3], double axOri[3], double sagPos[3], double sagOri[3])
{
    m_axialActor = vtkSmartPointer<vtkActor>::New();
    m_sagittalActor = vtkSmartPointer<vtkActor>::New();

    m_useWorldTransformCoordinate = false;
    m_axialPosition[0] = axPos[0]; m_axialPosition[1] = axPos[1]; m_axialPosition[2] = axPos[2];
    m_axialOrientation[0] = axOri[0]; m_axialOrientation[1] = axOri[1]; m_axialOrientation[2] = axOri[2];
    m_sagittalPosition[0] = sagPos[0]; m_sagittalPosition[1] = sagPos[1]; m_sagittalPosition[2] = sagPos[2];
    m_sagittalOrientation[0] = sagOri[0]; m_sagittalOrientation[1] = sagOri[1]; m_sagittalOrientation[2] = sagOri[2];
    m_pointerPosition[0] = 0; m_pointerPosition[1] = 0; m_pointerPosition[2] = 0;
    m_pointerOrientation[0] = 0; m_pointerOrientation[1] = -1; m_pointerOrientation[2] = 0;
}

Screw::Screw(const Screw *in)
{
    m_axialActor = vtkSmartPointer<vtkActor>::New();
    m_sagittalActor = vtkSmartPointer<vtkActor>::New();

    m_useWorldTransformCoordinate = in->m_useWorldTransformCoordinate;
    m_length = in->m_length;
    m_diameter = in->m_diameter;

    m_axialPosition[0] = in->m_axialPosition[0]; m_axialPosition[1] = in->m_axialPosition[1]; m_axialPosition[2] = in->m_axialPosition[2];
    m_axialOrientation[0] = in->m_axialOrientation[0]; m_axialOrientation[1] = in->m_axialOrientation[1]; m_axialOrientation[2] = in->m_axialOrientation[2];
    m_sagittalPosition[0] = in->m_sagittalPosition[0]; m_sagittalPosition[1] = in->m_sagittalPosition[1]; m_sagittalPosition[2] = in->m_sagittalPosition[2];
    m_sagittalOrientation[0] = in->m_sagittalOrientation[0]; m_sagittalOrientation[1] = in->m_sagittalOrientation[1]; m_sagittalOrientation[2] = in->m_sagittalOrientation[2];
    m_pointerPosition[0] = in->m_pointerPosition[0]; m_pointerPosition[1] = in->m_pointerPosition[1]; m_pointerPosition[2] = in->m_pointerPosition[2];
    m_pointerOrientation[0] = in->m_pointerOrientation[0]; m_pointerOrientation[1] = in->m_pointerOrientation[1]; m_pointerOrientation[2] = in->m_pointerOrientation[2];

    this->UpdateName();
}

void Screw::Serialize( Serializer * ser )
{
    ::Serialize( ser, "UseWorldTransform", m_useWorldTransformCoordinate );
    ::Serialize( ser, "PointerPosition", m_pointerPosition, 3 );
    ::Serialize( ser, "PointerOrientation", m_pointerOrientation, 3 );

    ::Serialize( ser, "ScrewLength", m_length );
    ::Serialize( ser, "ScrewDiameter", m_diameter );
    ::Serialize( ser, "ScrewTipSize", m_tipSize );

    if( ser->IsReader() )
    {
        this->UpdateName();
    }
}

Screw Screw::operator=(Screw in)
{
    return Screw(in);
}

void Screw::GetAxialPosistion(double (&out)[3])
{
    out[0] = m_axialPosition[0]; out[1] = m_axialPosition[1]; out[2] = m_axialPosition[2];
}

void Screw::GetAxialOrientation(double (&out)[3])
{
    out[0] = m_axialOrientation[0]; out[1] = m_axialOrientation[1]; out[2] = m_axialOrientation[2];
}

void Screw::GetSagittalPosistion(double (&out)[3])
{
    out[0] = m_sagittalPosition[0]; out[1] = m_sagittalPosition[1]; out[2] = m_sagittalPosition[2];
}

void Screw::GetSagittalOrientation(double (&out)[3])
{
    out[0] = m_sagittalOrientation[0]; out[1] = m_sagittalOrientation[1]; out[2] = m_sagittalOrientation[2];
}

void Screw::SetAxialPosistion(double in[3])
{
    m_axialPosition[0] = in[0]; m_axialPosition[1] = in[1]; m_axialPosition[2] = in[2];
}

void Screw::SetAxialOrientation(double in[3])
{
    m_axialOrientation[0] = in[0];  m_axialOrientation[1] = in[1]; m_axialOrientation[2] = in[2];
}

void Screw::SetSagittalPosistion(double in[3])
{
    m_sagittalPosition[0] = in[0]; m_sagittalPosition[1] = in[1]; m_sagittalPosition[2] = in[2];
}

void Screw::SetSagittalOrientation(double in[3])
{
    m_sagittalOrientation[0] = in[0]; m_sagittalOrientation[1] = in[1]; m_sagittalOrientation[2] = in[2];
}

void Screw::GetPointerOrientation(double (&out)[3])
{
    out[0] = m_pointerOrientation[0]; out[1] = m_pointerOrientation[1]; out[2] = m_pointerOrientation[2];
}

void Screw::SetPointerOrientation(double in[3])
{
    m_pointerOrientation[0] = in[0]; m_pointerOrientation[1] = in[1]; m_pointerOrientation[2] = in[2];
}

void Screw::GetPointerPosition(double (&out)[3])
{
    out[0] = m_pointerPosition[0]; out[1] = m_pointerPosition[1]; out[2] = m_pointerPosition[2];
}

void Screw::SetPointerPosition(double in[3])
{
    m_pointerPosition[0] = in[0]; m_pointerPosition[1] = in[1]; m_pointerPosition[2] = in[2];
}

void Screw::SetScrewProperties(double length, double diameter, double tipSize)
{
    m_length = length;
    m_diameter = diameter;
    m_tipSize = tipSize;

    this->UpdateName();
}

void Screw::UpdateName()
{
    m_name = Screw::GetName(m_length, m_diameter);
}

std::string Screw::GetName(double length, double diameter)
{
    // Update name
    std::stringstream stream;
    stream.precision(1);
    stream << std::fixed;

    stream << length << " mm X " << diameter << " mm";
    return stream.str();
}

std::string Screw::GetScrewID(double length, double diameter)
{
    // Update name
    std::stringstream stream;
    stream.precision(1);
    stream << std::fixed;

    stream << length << "x" << diameter;
    return stream.str();
}


void Screw::GetScrewPolyData(double length, double diameter, double tipSize, vtkSmartPointer<vtkPolyData> &polyData)
{
    /* Screw cross-section representation
     *
     *       p0  ----  p1
     *          |    |
     *          |    |
     *       p4 |    | p2
     *          \    /
     *           \  /
     *            \/
     *            p3
     * */
    double p0[3] = { -diameter/2.0, 0.0, 0.0 };
    double p1[3] = {  diameter/2.0, 0.0, 0.0 };
    double p2[3] = {  diameter/2.0, -length + tipSize, 0.0 };
    double p3[3] = { 0.0, -length, 0.0 };
    double p4[3] = { -diameter/2.0, -length + tipSize, 0.0 };

    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    pts->InsertNextPoint(p0);
    pts->InsertNextPoint(p1);
    pts->InsertNextPoint(p2);
    pts->InsertNextPoint(p3);
    pts->InsertNextPoint(p4);
    pts->InsertNextPoint(p0);

    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
    lines->InsertNextCell(6);
    lines->InsertCellPoint(0);
    lines->InsertCellPoint(1);
    lines->InsertCellPoint(2);
    lines->InsertCellPoint(3);
    lines->InsertCellPoint(4);
    lines->InsertCellPoint(0);

    polyData->DeleteCells();
    polyData->SetPoints(pts);
    polyData->SetLines(lines);

}

void Screw::GetScrewPolyData(vtkSmartPointer<vtkPolyData> polyData)
{
    this->GetScrewPolyData(m_length, m_diameter, m_tipSize, polyData);
}

void Screw::PrintSelf()
{
    std::cout << "Screw: " << this << std::endl;
    std::cout << "\t" << "Axial Position: "
              << m_axialPosition[0] << " , " << m_axialPosition[1] << " , " << m_axialPosition[2]
              << std::endl;
    std::cout << "\t" << "Axial Orientation: "
              << m_axialOrientation[0] << " , " << m_axialOrientation[1] << " , " << m_axialOrientation[2]
              << std::endl;
    std::cout << "\t" << "Sagittal Position: "
              << m_sagittalPosition[0] << " , " << m_sagittalPosition[1] << " , " << m_sagittalPosition[2]
              << std::endl;
    std::cout << "\t" << "Sagittal Orientation: "
              << m_sagittalOrientation[0] << " , " << m_sagittalOrientation[1] << " , " << m_sagittalOrientation[2]
              << std::endl;
    std::cout << "----" << std::endl;
}
