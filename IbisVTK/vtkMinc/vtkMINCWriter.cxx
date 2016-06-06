#include <string>
#include "vtkMINCWriter.h"
extern "C"
{
#include "minc.h"
}
#include "vtkMath.h"
#include "vtkMINC2.h"


const char className[30] = "vtkMINCWriter";
char const * vtkMINCWriter::GetClassName() const
{
    return className;
}


vtkMINCWriter::vtkMINCWriter()
{
    Input = NULL;
    Transform = NULL;
    Filename = NULL;
    UseTransformForStartAndStep = 0;
    Comment[0] = 0;
    TimeStamp = 0.0;
}


vtkMINCWriter::~vtkMINCWriter()
{
    if( Input )
        Input->UnRegister( this );
    if( Transform )
        Transform->UnRegister( this );
    if( Filename )
        delete Filename;
}


void vtkMINCWriter::Write()
{
    // Extract start step and direction cosines from matrix
    double istart[3] = { 0, 0, 0 };
    double istep[3] = { 1, 1, 1 };
    double idirCosine[3][3] = { { 1, 0, 0,},{ 0, 1, 0 },{ 0, 0, 1 } };
    if( Transform )
        vtkMatrixToStartStepAndDirCosine( Transform, istart, istep, idirCosine );

    if( !this->UseTransformForStartAndStep )
    {
        double tempStart[3] = { 0, 0, 0 };
        this->Input->GetOrigin( tempStart );
        double tempStep[3] = { 1, 1, 1 };
        this->Input->GetSpacing( tempStep );
        for( int i = 0; i < 3; i++ )
        {
            istart[i] = (double)(tempStart[i]);
            istep[i] = (double)(tempStep[i]);
        }
    }
    else if( this->UseTransformForStartAndStep > 1 && Transform )
    {
        double newStart[3];
        this->Input->GetOrigin(newStart);
        for( int i = 0; i < 3; i++ )
        {
            istart[i] += newStart[i];
            istep[i] = this->Input->GetSpacing()[i];
        }
    }
    
    //----------------------------------------------------------------
    // Write minc file
    //----------------------------------------------------------------

    // Create the file
    int mincFileId = micreate( Filename, NC_CLOBBER );

    // Define the dimensions
    int numberOfDimensions = 3;
    int sizes[3] = { -1, -1, -1 };
    Input->GetDimensions( sizes );
    int dimId[4] = { -1, -1, -1, -1 };
    dimId[0] = ncdimdef( mincFileId, MIzspace, sizes[2] );
    dimId[1] = ncdimdef( mincFileId, MIyspace, sizes[1] );
    dimId[2] = ncdimdef( mincFileId, MIxspace, sizes[0] );

    // define the dimension variables
    int dimVarId[4] = { -1, -1, -1, -1 };
    dimVarId[0] = micreate_std_variable( mincFileId, (char*)MIzspace, NC_INT, 0, NULL );
    dimVarId[1] = micreate_std_variable( mincFileId, (char*)MIyspace, NC_INT, 0, NULL );
    dimVarId[2] = micreate_std_variable( mincFileId, (char*)MIxspace, NC_INT, 0, NULL );
    int index = 0;
    int i = 0;
    for( i = 0; i < 3; i++ )
    {
        index = 2 - i;
        miattputdbl( mincFileId, dimVarId[index], (char*)MIstep, istep[i] );
        miattputdbl( mincFileId, dimVarId[index], (char*)MIstart, istart[i] );
        ncattput( mincFileId, dimVarId[index], MIdirection_cosines, NC_DOUBLE, 3, (void*)(&idirCosine[i]) );
    }

    // Add a vector dimension if the number of scalar components > 1
    int numberOfScalarComp = Input->GetNumberOfScalarComponents();
    if( numberOfScalarComp > 1 )
    {
        numberOfDimensions = 4;
        dimId[3] = ncdimdef( mincFileId, MIvector_dimension, numberOfScalarComp );
    }

    this->WriteAdditionalMINCAttributes(mincFileId);

    /***************************************
     * The following switch is a mod and an addon by Sean Chen on 08/29/2007
     *
     * Previosuly this part of the function assumed that everything was a char
     * which is a completely bogus assumption considering, well, minc files
     * could be written in any data type. Bad previous programmer! BAD! Because
     * of you I had to stay late to fix the prob!
     *
     * Everything here is sorta hacked together so if you volume or image
     * turns out completely white, or slighty deranged in the offset sense
     * then the problem is here.
     *
     * I made semi-educated guesses of what NC and MI type were for the VTK types
     * they might be wrong too.
     *
     * If you are wondering, this part cannot be combined with the other switch
     * structure down there because this is need in order to do calls in the 
     * sandwiched part correctly
     **********************************/

    // Define the image variables
    int imageVarId;

    switch(Input->GetScalarType())
    {
    case VTK_CHAR:
    {
    imageVarId = micreate_std_variable( mincFileId, (char*)MIimage, NC_BYTE, numberOfDimensions, dimId );
    miattputstr( mincFileId, imageVarId, (char*)MIsigntype, (char*)MI_UNSIGNED );
    }
    break;
    case VTK_SIGNED_CHAR:
    {
    imageVarId = micreate_std_variable( mincFileId, (char*)MIimage, NC_BYTE, numberOfDimensions, dimId );
    miattputstr( mincFileId, imageVarId,(char*) MIsigntype, (char*)MI_SIGNED );
    }
    break;
    case VTK_UNSIGNED_CHAR:
    {
    imageVarId = micreate_std_variable( mincFileId,(char*) MIimage, NC_BYTE, numberOfDimensions, dimId );
    miattputstr( mincFileId, imageVarId,(char*) MIsigntype, (char*)MI_UNSIGNED );
    }
    break;
    case VTK_SHORT:
    {
    imageVarId = micreate_std_variable( mincFileId, (char*)MIimage, NC_SHORT, numberOfDimensions, dimId );
    miattputstr( mincFileId, imageVarId,(char*) MIsigntype, (char*)MI_SIGNED );
    }
    break;
    case VTK_UNSIGNED_SHORT:
    {
    imageVarId = micreate_std_variable( mincFileId, (char*)MIimage, NC_SHORT, numberOfDimensions, dimId );
    miattputstr( mincFileId, imageVarId,(char*) MIsigntype, (char*)MI_UNSIGNED );
    }
    break;
    case VTK_INT:
    {
    imageVarId = micreate_std_variable( mincFileId, (char*)MIimage, NC_SHORT, numberOfDimensions, dimId );
    miattputstr( mincFileId, imageVarId,(char*) MIsigntype, (char*)MI_SIGNED );
    }
    break;
    case VTK_UNSIGNED_INT:
    {
    imageVarId = micreate_std_variable( mincFileId, (char*)MIimage, NC_SHORT, numberOfDimensions, dimId );
    miattputstr( mincFileId, imageVarId, (char*)MIsigntype, (char*)MI_UNSIGNED );
    }
    break;
    case VTK_LONG:
    {
    imageVarId = micreate_std_variable( mincFileId, (char*)MIimage, NC_LONG, numberOfDimensions, dimId );
    miattputstr( mincFileId, imageVarId, (char*)MIsigntype, (char*)MI_SIGNED );
    }
    break;
    case VTK_UNSIGNED_LONG:
    {
    imageVarId = micreate_std_variable( mincFileId, (char*)MIimage, NC_LONG, numberOfDimensions, dimId );
    miattputstr( mincFileId, imageVarId, (char*)MIsigntype, (char*)MI_UNSIGNED );
    }
    break;
    case VTK_FLOAT:
    {
    imageVarId = micreate_std_variable( mincFileId, (char*)MIimage, NC_FLOAT, numberOfDimensions, dimId );
    miattputstr( mincFileId, imageVarId, (char*)MIsigntype, (char*)MI_SIGNED );
    }
    break;
    case VTK_DOUBLE:
    {
    imageVarId = micreate_std_variable( mincFileId, (char*)MIimage, NC_DOUBLE, numberOfDimensions, dimId );
    miattputstr( mincFileId, imageVarId, (char*)MIsigntype, (char*)MI_SIGNED );
    }
    break;
    default:
	std::cerr<<"You really really screwed up"<<std::endl;
	break;
    }

    
    // Define image min and max variables
    int imageMaxVarId = micreate_std_variable( mincFileId, (char*)MIimagemax, NC_DOUBLE, 0, NULL);
    int imageMinVarId = micreate_std_variable( mincFileId, (char*)MIimagemin, NC_DOUBLE, 0, NULL);

    // End definition mode
    ncendef( mincFileId );

    // Write the image max and min
    double imageMin = 0.0;
    double imageMax = 1.0;
    int res = ncvarput1( mincFileId, imageMinVarId, NULL, &imageMin );
    res = ncvarput1( mincFileId, imageMaxVarId, NULL, &imageMax );

    // Write the image
    long start[4] = { 0, 0, 0, 0 };
    long count[4] = { 1, 1, 1, 1 };
    count[3] = numberOfScalarComp;
    count[2] = sizes[0];
    count[1] = sizes[1];
    count[0] = sizes[2];
    

    /***************************************
     * The following stuff is a mod and an addon by Sean Chen on 08/29/2007
     *
     * Previosuly this part of the function assumed that everything was a char
     * which is a completely bogus assumption considering, well, minc files
     * could be written in any data type. Bad previous programmer! BAD! Because
     * of you I had to stay late to fix the prob!
     *
     * Everything here is sorta hacked together so if you volume or image
     * turns out completely white, or slighty deranged in the offset sense
     * then the problem is here.
     *
     * I made semi-educated guesses of what NC and MI type were for the VTK types
     * they might be wrong too.
     **********************************/

    switch(Input->GetScalarType())
    {
    case VTK_CHAR:
    {
	char * ptrData = (char*)Input->GetScalarPointer( 0, 0, 0 );
    mivarput( mincFileId, imageVarId, start, count, NC_BYTE,(char*) MI_UNSIGNED, ptrData );
    }
    break;
    case VTK_SIGNED_CHAR:
    {
	signed char * ptrData = (signed char*)Input->GetScalarPointer( 0, 0, 0 );
    mivarput( mincFileId, imageVarId, start, count, NC_BYTE, (char*)MI_SIGNED, ptrData );
    }
    break;
    case VTK_UNSIGNED_CHAR:
    {
	unsigned char * ptrData = (unsigned char*)Input->GetScalarPointer( 0, 0, 0 );
    mivarput( mincFileId, imageVarId, start, count, NC_BYTE, (char*)MI_UNSIGNED, ptrData );
    }
    break;
    case VTK_SHORT:
    {
	signed short * ptrData = (signed short*)Input->GetScalarPointer( 0, 0, 0 );
    mivarput( mincFileId, imageVarId, start, count, NC_SHORT, (char*)MI_SIGNED, ptrData );
    }
    break;
    case VTK_UNSIGNED_SHORT:
    {
	unsigned short * ptrData = (unsigned short*)Input->GetScalarPointer( 0, 0, 0 );
    mivarput( mincFileId, imageVarId, start, count, NC_SHORT, (char*)MI_UNSIGNED, ptrData );
    }
    break;
    case VTK_INT:
    {
	int * ptrData = (int*)Input->GetScalarPointer( 0, 0, 0 );
    mivarput( mincFileId, imageVarId, start, count, NC_SHORT, (char*)MI_SIGNED, ptrData );
    }
    break;
    case VTK_UNSIGNED_INT:
    {
	unsigned int * ptrData = (unsigned int*)Input->GetScalarPointer( 0, 0, 0 );
    mivarput( mincFileId, imageVarId, start, count, NC_SHORT, (char*)MI_UNSIGNED, ptrData );
    }
    break;
    case VTK_LONG:
    {
	long * ptrData = (long*)Input->GetScalarPointer( 0, 0, 0 );
    mivarput( mincFileId, imageVarId, start, count, NC_LONG, (char*)MI_SIGNED, ptrData );
    }
    break;
    case VTK_UNSIGNED_LONG:
    {
	unsigned long * ptrData = (unsigned long*)Input->GetScalarPointer( 0, 0, 0 );
    mivarput( mincFileId, imageVarId, start, count, NC_LONG,(char*) MI_UNSIGNED, ptrData );
    }
    break;
    case VTK_FLOAT:
    {
	float * ptrData = (float*)Input->GetScalarPointer( 0, 0, 0 );
    mivarput( mincFileId, imageVarId, start, count, NC_FLOAT, (char*)MI_SIGNED, ptrData );
    }
    break;
    case VTK_DOUBLE:
    {
	double * ptrData = (double*)Input->GetScalarPointer( 0, 0, 0 );
    mivarput( mincFileId, imageVarId, start, count, NC_DOUBLE, (char*)MI_SIGNED, ptrData );
    }
    break;
    default:
	std::cerr<<"You really really screwed up"<<std::endl;
	break;
    }

    // Close the file
    miclose( mincFileId );
}


void vtkMINCWriter::vtkMatrixToStartStepAndDirCosine( vtkMatrix4x4 * mat, double start[3], double step[3], double dirCosine[3][3] )
{
    vtkMatrix4x4 * columnMat = vtkMatrix4x4::New();
    vtkMatrix4x4::Transpose( mat, columnMat );
    for( int i = 0; i < 3; i++ )
    {
        step[i] = vtkMath::Dot( (*columnMat)[i], (*columnMat)[i] );
        step[i] = sqrt( step[i] );
        for( int j = 0; j < 3; j++ )
        {
            dirCosine[i][j] = (*columnMat)[i][j] / step[i];
        }
    }

    double rotation[3][3];
    vtkMath::Transpose3x3( dirCosine, rotation );
    vtkMath::LinearSolve3x3( rotation, (*columnMat)[3], start );
}
void vtkMINCWriter::SetAdditionalMINCAttributes(STUDY_ATTRIBUTES *s, ACQUISITION_ATTRIBUTES *a, PATIENT_ATTRIBUTES *p, const char* c)
{
    if (!s->empty())
        Study = *s;
    if (!a->empty())
        Acquisition = *a;
    if (!p->empty())
        Patient = *p;
    if (c && *c)
        strcpy(Comment, c);
}

void vtkMINCWriter::WriteAdditionalMINCAttributes(int mincFileId)
{
    int patientVarId, studentVarId, acquisitionVarId;
    //patient
    if (!Patient.empty())
    {
        patientVarId = micreate_std_variable( mincFileId, (char*)MIpatient, NC_INT, 0, NULL );
        PATIENT_ATTRIBUTES::iterator it = Patient.begin();
        PATIENT_ATTRIBUTES::iterator itEnd = Patient.end();
        for( ; it != itEnd; ++it )
        {
            miattputstr( mincFileId, patientVarId, (char*)(*it).first, (char*)(*it).second.c_str()  );
        }
    }
    if (!Study.empty())
    {
        studentVarId = micreate_std_variable( mincFileId, (char*)MIstudy, NC_INT, 0, NULL );
        STUDY_ATTRIBUTES::iterator it = Study.begin();
        STUDY_ATTRIBUTES::iterator itEnd = Study.end();
        for( ; it != itEnd; ++it )
        {
            miattputstr( mincFileId, studentVarId, (char*)(*it).first, (char*)(*it).second.c_str()  );
        }
    }
    if (!Acquisition.empty())
    {
        acquisitionVarId = micreate_std_variable( mincFileId, (char*)MIacquisition, NC_INT, 0, NULL );
        ACQUISITION_ATTRIBUTES::iterator it = Acquisition.begin();
        ACQUISITION_ATTRIBUTES::iterator itEnd = Acquisition.end();
        for( ; it != itEnd; ++it )
        {
            miattputstr( mincFileId, acquisitionVarId, (char*)(*it).first, (char*)(*it).second.c_str()  );
        }
    }
    if (Comment[0])
        miattputstr( mincFileId, -1, (char*)MIcomments, Comment );
    miattputdbl( mincFileId, -1, (char*)MItime_stamp, TimeStamp );
}

