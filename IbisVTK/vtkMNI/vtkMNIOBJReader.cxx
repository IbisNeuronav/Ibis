/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class
#include "vtkMNIOBJReader.h"

#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"


vtkStandardNewMacro(vtkMNIOBJReader);

// Description:
// Instantiate object with NULL filename.
vtkMNIOBJReader::vtkMNIOBJReader()
{
    this->SetNumberOfInputPorts(0);
    this->FileName = NULL;
    this->Property = vtkSmartPointer<vtkProperty>::New();
	this->NbPoints = 0;
	this->UseAlpha = true;
}

vtkMNIOBJReader::~vtkMNIOBJReader()
{
    if (this->FileName)
    {
        delete [] this->FileName;
        this->FileName = NULL;
    }
}

int vtkMNIOBJReader::CanReadFile( const char * fname )
{
    FILE * in = fopen( fname, "r" );

    if (in == 0 )
    {
        return 0;
    }

    char p;

    // check file type
    fscanf( in, "%c", &p );
    fclose( in );

    if ( p != 'P' && p != 'L' )
    {
        return 0;
    }

    return 1;
}

/*-------------------------------------------------------------*/
enum MniObjType { mniPoly, mniLines, mniUnsupported, mniNotObj };

int vtkMNIOBJReader::RequestData( vtkInformation *vtkNotUsed(request),
                                  vtkInformationVector **vtkNotUsed(inputVector),
                                  vtkInformationVector *outputVector)
{
    // get the info object
    vtkInformation *outInfo = outputVector->GetInformationObject(0);

    // get the ouptut
    vtkPolyData *output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    // all of the data in the first piece.
    if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
    {
        return 0;
    }

    // read the file
    return this->ReadFile(output);
}

int vtkMNIOBJReader::ReadFile(vtkPolyData *output)
{
    // Initialize the property to default values
    vtkProperty *property = vtkProperty::New();
    this->Property->DeepCopy(property);
    property->Delete();

    if (!this->FileName)
    {
        vtkErrorMacro(<< "A FileName must be specified.");
        return 0;
    }

    // open the file
    FILE * in = fopen(this->FileName,"r");
    if (in == NULL)
    {
        vtkErrorMacro(<< "File " << this->FileName << " not found");
        return 0;
    }

    // check file type
    char p;
    fscanf(in, "%c", &p);
    MniObjType type = mniPoly;
    switch( p )
    {
        case 'l':
        case 'm':
        case 'f':
        case 'x':
        case 'p':
        case 'q':
        case 't':
            vtkErrorMacro(<<"Binary .obj files are not supported.");
            break;
        case 'L':
            type = mniLines;
            break;
        case 'P':
            type = mniPoly;
            break;
        case 'M':
        case 'F':
        case 'X':
        case 'Q':
        case 'T':
            type = mniUnsupported;
            vtkErrorMacro(<<" This is not a MNI .obj polygon file.");
            break;
        default:
            type = mniNotObj;
            vtkErrorMacro(<<" This is not a MNI .obj polygon file.");
            break;
    }

    if( type == mniLines )
    {
        ReadLines( in, output );
    }
    else if( type == mniPoly )
    {
        ReadPolygons( in, output );
    }
    else
        return 0;

    // close the file
    fclose(in);
    return 1;
}


//------------------------------------------------------------
// Filetype is 'lines'
// Description from object_io in bicpl:
// 		- 'L'
// 		- (float) thickness
// 		- (int) n_points
// 		- (newline)
// 		- points 
// 		  - three floats per line
// 		- (newline)
// 		- (int) n_items (#lines)
// 		- (newline)
// 		- colours
// 		  - (int) colour_flag 0=ONE_COLOUR, 1=PER_ITEM, 2=PER_VERTEX
// 		  - the specified number of colours, one per line
// 		- (newline)
// 		- end_indices
// 		- (newline)
// 		- indices
//------------------------------------------------------------
void vtkMNIOBJReader::ReadLines( FILE * in, vtkPolyData *output )
{
	float thickness = 1;
	fscanf( in, "%f", &thickness );
	
    ReadPoints( in, output );

	fscanf( in, "%d", &NbItems );

    ReadColors( in, output );

	// Read lines
	vtkCellArray * cells = vtkCellArray::New();
	ReadItems( in, cells );
    output->SetLines( cells );
    cells->Delete();
}

//------------------------------------------------------------
// Filetype is 'lines'
// Description from object_io in bicpl:
//		  - 'P'
//		  - five floats representing 'surfprop'
//		  - n_points if normal format,
//		    or -n_items (#polygons) if compressed format
//		  - newline
//		  - table of point coordinate, one point per line
//		  - newline (blank line)
//		  - if not compressed
//		    - table of point normals, one per line 
//		    - newline (blank line)
//		    - n_items (#polygons)
//		    - newline
//		  - colour flag
//		  - for each allocated colour (1, n_points, or n_items)
//		    - colour and newline
//		  - newline (blank line)
//		  - if not compressed
//		    - end_indices (two formats, see Objects/o.c)
//		    - newline (blank line)
//		    - indices
//		    - newline (blank line)
//		
//		In compressed format, the polygons form either a tetrahedron (4
//		polygons), two tetrahedrons glued together (6 polygons), two pyramids
//		glued together (8 polygons) or a regular subdivision of one of these.
//		Surface normals for each point are computed, after reading file.
//------------------------------------------------------------
void vtkMNIOBJReader::ReadPolygons( FILE * in, vtkPolyData *output )
{
	// fill in Property coefficients
	float ambient, diffuse, specular, specular_exp, opacity;
    fscanf(in, "%f", &ambient);
    fscanf(in, "%f", &diffuse);
    fscanf(in, "%f", &specular);
    fscanf(in, "%f", &specular_exp);
    fscanf(in, "%f", &opacity);

    this->Property->SetAmbient(ambient);
    this->Property->SetDiffuse(diffuse);
    this->Property->SetSpecular(specular);
    this->Property->SetSpecularPower(specular_exp);
    this->Property->SetOpacity(opacity);

    ReadPoints( in, output );

	// Read normals
    double norms[3];
    vtkDoubleArray * normals = vtkDoubleArray::New();
    normals->SetNumberOfComponents(3);
    normals->SetNumberOfTuples(NbPoints);
    for( int j = 0; j < NbPoints; j++ )
    {
        fscanf(in, " %lf ",norms);
        fscanf(in, " %lf ",norms+1);
        fscanf(in, " %lf ",norms+2);
        normals->SetTuple(j, norms);
    }
	this->GetOutput()->GetPointData()->SetNormals(normals);
	normals->Delete();

	// Read number of items
	fscanf( in, "%d", &NbItems );
	
	// Read colors
    ReadColors( in, output );

	// Read polygons
	vtkCellArray * indexCells = vtkCellArray::New();
	ReadItems( in, indexCells );
    output->SetPolys(indexCells);
    indexCells->Delete();
}

void vtkMNIOBJReader::ReadPoints( FILE * in, vtkPolyData *output )
{
	 // fill point coordinates in points
	fscanf(in, "%d", &NbPoints);
    double x, y, z;
    vtkPoints * points = vtkPoints::New();
    points->SetDataTypeToDouble();
    points->SetNumberOfPoints(NbPoints);

    for( int i = 0; i < NbPoints; i++ )
    {
        fscanf(in, " %lf ",&x);
        fscanf(in, " %lf ",&y);
        fscanf(in, " %lf ",&z);

        points->SetPoint(i, x, y, z);
    }

    output->SetPoints(points);

	points->Delete();
}

void vtkMNIOBJReader::ReadColors(FILE * in , vtkPolyData *output)
{
	// determine type of coloration
    int colorperpoint;
    fscanf(in,"%d", &colorperpoint);

    float rgba[4];
    if(colorperpoint == 0) // 1 color for all points
    {
        fscanf(in, " %f ",rgba);
        fscanf(in, " %f ",rgba+1);
        fscanf(in, " %f ",rgba+2);
        fscanf(in, " %f ",rgba+3);
        
        // set the color in Property
        this->Property->SetColor(rgba[0], rgba[1], rgba[2]);
    }
	else if(colorperpoint == 1) // 1 color per item (line segment, triangle, etc. )
	{
		vtkErrorMacro("Color per item not yet supported.");
		return;
	}
    else if(colorperpoint == 2) // 1 color per vertex
    {
		vtkUnsignedCharArray *charColors = vtkUnsignedCharArray::New();
		if( UseAlpha )
			charColors->SetNumberOfComponents(4);
		else
			charColors->SetNumberOfComponents(3);
        charColors->SetNumberOfTuples(NbPoints);

        for (int k=0; k<NbPoints; k++)
        {
            fscanf(in, " %f ",rgba);
            fscanf(in, " %f ",rgba+1);
            fscanf(in, " %f ",rgba+2);
            fscanf(in, " %f ",rgba+3);
            // transorm float values into approximate unsigned char values
            if( UseAlpha )
				charColors->SetTuple4(k, rgba[0]*255, rgba[1]*255, rgba[2]*255, rgba[3]*255 );
			else
				charColors->SetTuple3(k, rgba[0]*255, rgba[1]*255, rgba[2]*255 );
			
        }
        output->GetPointData()->SetScalars(charColors);
		charColors->Delete();
    }
}

void vtkMNIOBJReader::ReadItems( FILE * in, vtkCellArray * indexCells )
{
	// read last index of each item
    vtkIntArray * endIndices = vtkIntArray::New();
    endIndices->SetNumberOfValues(NbItems);
    int bufferInt;
    for( int m = 0; m < NbItems; m++ )
    {
        fscanf(in, "%d", &bufferInt);
        endIndices->SetValue(m, bufferInt);
    }

    // Read First item
    int poly_indx;
	int end_indx;
    indexCells->InsertNextCell(endIndices->GetValue(0));
    end_indx = endIndices->GetValue(0);
    for( int s = 0; s < end_indx; s++ )
    {
        fscanf(in, "%d", &poly_indx);
        indexCells->InsertCellPoint(poly_indx);
    }

	// Read other items
    for( int n = 1; n < NbItems; n++ )
    {
        // get dimension of polygon n
        end_indx = (endIndices->GetValue(n) - endIndices->GetValue(n-1));
        // end_indx gives dimension of cell n
        indexCells->InsertNextCell(end_indx);

        // insert [end_indx] points in cell p of indexCells
        for(int p=0; p< end_indx ; p++)
        {
            fscanf(in, "%d", &poly_indx);
            indexCells->InsertCellPoint(poly_indx);
        }
    }
    endIndices->Delete();
}

void vtkMNIOBJReader::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os,indent);

    os << indent << "File Name: "
    << (this->FileName ? this->FileName : "(none)") << "\n";

}

vtkProperty * vtkMNIOBJReader::GetProperty()
{
    return this->Property.GetPointer();
}
