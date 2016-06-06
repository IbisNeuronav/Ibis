/*=========================================================================

  Program:   AtamaiTracking for VTK
  Module:    vtkTrackerBuffer.cxx
  Creator:   David Gobbi <dgobbi@atamai.com>
  Language:  C++

==========================================================================

Copyright (c) 2000-2005 Atamai, Inc.

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
   form, must retain the above copyright notice, this license,
   the following disclaimer, and any notices that refer to this
   license and/or the following disclaimer.  

2) Redistribution in binary form must include the above copyright
   notice, a copy of this license and the following disclaimer
   in the documentation or with other materials provided with the
   distribution.

3) The name of the Atamai Inc., nor of its principals or owners,
   past or present, may be used to ensorse or promote products derived
   from this software without specific prior written permission.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/
#include "vtkTrackerBuffer.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkCriticalSection.h"
#include "vtkObjectFactory.h"

#include <stdio.h>
#include <stdlib.h>

//----------------------------------------------------------------------------
vtkTrackerBuffer* vtkTrackerBuffer::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTrackerBuffer");
  if(ret)
    {
    return (vtkTrackerBuffer*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTrackerBuffer;
}

//----------------------------------------------------------------------------
vtkTrackerBuffer::vtkTrackerBuffer()
{
  this->MatrixArray = vtkDoubleArray::New();
  this->MatrixArray->SetNumberOfComponents(16);
  this->FlagArray = vtkIntArray::New();
  this->TimeStampArray = vtkDoubleArray::New();

  this->BufferSize = 1000;
  this->MatrixArray->SetNumberOfTuples(this->BufferSize);
  this->FlagArray->SetNumberOfValues(this->BufferSize);
  this->TimeStampArray->SetNumberOfValues(this->BufferSize);

  this->NumberOfItems = 0;
  this->CurrentIndex = 0;
  this->CurrentTimeStamp = 0.0;

  this->Mutex = vtkCriticalSection::New();
  
  this->ToolCalibrationMatrix = NULL;
  this->WorldCalibrationMatrix = NULL;
}

//----------------------------------------------------------------------------
void vtkTrackerBuffer::DeepCopy(vtkTrackerBuffer *buffer)
{
  this->SetBufferSize(buffer->GetBufferSize());

  for (int i = 0; i < this->BufferSize; i++)
    {
    this->MatrixArray->SetTuple(i, buffer->MatrixArray->GetTuple(i));
    this->FlagArray->SetValue(i, buffer->FlagArray->GetValue(i));
    this->TimeStampArray->SetValue(i, buffer->TimeStampArray->GetValue(i));
    }

  this->CurrentIndex = buffer->CurrentIndex;
  this->NumberOfItems = buffer->NumberOfItems;
  this->CurrentTimeStamp = buffer->CurrentTimeStamp;

  vtkMatrix4x4 *tmatrix = vtkMatrix4x4::New();
  tmatrix->DeepCopy(buffer->GetToolCalibrationMatrix());
  this->SetToolCalibrationMatrix(tmatrix);
  tmatrix->Delete();
  vtkMatrix4x4 *wmatrix = vtkMatrix4x4::New();
  wmatrix->DeepCopy(buffer->GetWorldCalibrationMatrix());
  this->SetWorldCalibrationMatrix(wmatrix);
  wmatrix->Delete();
}

//----------------------------------------------------------------------------
vtkTrackerBuffer::~vtkTrackerBuffer()
{
  this->MatrixArray->Delete();
  this->FlagArray->Delete();
  this->TimeStampArray->Delete();

  this->Mutex->Delete();

  if (this->WorldCalibrationMatrix)
    {
    this->WorldCalibrationMatrix->Delete();
    }
  if (this->ToolCalibrationMatrix)
    {
    this->ToolCalibrationMatrix->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkTrackerBuffer::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);
  
  os << indent << "BufferSize: " << this->BufferSize << "\n";
  os << indent << "NumberOfItems: " << this->NumberOfItems << "\n";
  os << indent << "ToolCalibrationMatrix: " << this->ToolCalibrationMatrix << "\n";
  if (this->ToolCalibrationMatrix)
    {
    this->ToolCalibrationMatrix->PrintSelf(os,indent.GetNextIndent());
    }  
  os << indent << "WorldCalibrationMatrix: " << this->WorldCalibrationMatrix << "\n";
  if (this->WorldCalibrationMatrix)
    {
    this->WorldCalibrationMatrix->PrintSelf(os,indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
void vtkTrackerBuffer::SetBufferSize(int n)
{
  if (n == this->BufferSize)
    {
    return;
    }

  // right now, there is no effort made to save the previous contents
  this->NumberOfItems = 0;
  this->CurrentIndex = 0;
  this->CurrentTimeStamp = 0.0;
 
  this->BufferSize = n;
  this->MatrixArray->SetNumberOfTuples(this->BufferSize);
  this->FlagArray->SetNumberOfValues(this->BufferSize);
  this->TimeStampArray->SetNumberOfValues(this->BufferSize);

  this->Modified();
}  

//----------------------------------------------------------------------------
void vtkTrackerBuffer::AddItem(vtkMatrix4x4 *matrix, long flags, double time)
{
  if (time <= this->CurrentTimeStamp)
    {
    return;
    }
  this->CurrentTimeStamp = time;

  if (++this->CurrentIndex >= this->BufferSize)
    {
    this->CurrentIndex = 0;
    this->NumberOfItems = this->BufferSize;
    }

  if (this->CurrentIndex > this->NumberOfItems)
    {
    this->NumberOfItems = this->CurrentIndex;
    }
  
  this->MatrixArray->SetTuple(this->CurrentIndex, *matrix->Element);
  this->FlagArray->SetValue(this->CurrentIndex, flags);
  this->TimeStampArray->SetValue(this->CurrentIndex, time);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkTrackerBuffer::GetMatrix(vtkMatrix4x4 *matrix, int i)
{
  i = ((this->CurrentIndex - i) % this->BufferSize);

  if (i < 0)
    {
    i += this->BufferSize;
    }

  this->MatrixArray->GetTuple(i,*matrix->Element);
  matrix->Modified();

  this->ApplyCalibration( matrix );

}

//----------------------------------------------------------------------------
void vtkTrackerBuffer::GetUncalibratedMatrix(vtkMatrix4x4 *matrix, int i)
{
  i = ((this->CurrentIndex - i) % this->BufferSize);

  if (i < 0)
    {
    i += this->BufferSize;
    }

  this->MatrixArray->GetTuple(i,*matrix->Element);
  matrix->Modified();
}

//----------------------------------------------------------------------------
long vtkTrackerBuffer::GetFlags(int i)
{
  i = ((this->CurrentIndex - i) % this->BufferSize);

  if (i < 0)
    {
    i += this->BufferSize;
    }

  return this->FlagArray->GetValue(i);
}

//----------------------------------------------------------------------------
double vtkTrackerBuffer::GetTimeStamp(int i)
{
  i = ((this->CurrentIndex - i) % this->BufferSize);

  if (i < 0)
    {
    i += this->BufferSize;
    }
  return this->TimeStampArray->GetValue(i);
}

//----------------------------------------------------------------------------
// do a simple divide-and-conquer search for the transform
// that best matches the given timestamp
int vtkTrackerBuffer::GetIndexFromTime(double time)
{
  int lo = this->NumberOfItems-1;
  int hi = 0;

  double tlo = this->GetTimeStamp(lo);
  double thi = this->GetTimeStamp(hi);
  if (time <= tlo)
    {
    return lo;
    }
  else if (time >= thi)
    {
    return hi;
    }

  for (;;)
    {
    if (lo-hi == 1)
      {
      if (time - tlo > thi - time)
    {
    return hi;
    }
      else
    {
    return lo;
    }
      }

    int mid = (lo+hi)/2;
    double tmid = this->GetTimeStamp(mid);
  
    if (time < tmid)
      {
      hi = mid;
      thi = tmid;
      }
    else
      {
      lo = mid;
      tlo = tmid;
      }
    }
}

//----------------------------------------------------------------------------
// a mathematical helper function
static void vtkMatrix3x3ToQuaternion(const double A[3][3], double quat[4])
{
  double trace = 1.0 + A[0][0] + A[1][1] + A[2][2];
  double s;

  if (trace > 0.0001)
    {
    s = sqrt(trace);
    quat[0] = 0.5*s;
    s = 0.5/s;
    quat[1] = (A[2][1] - A[1][2])*s;
    quat[2] = (A[0][2] - A[2][0])*s;
    quat[3] = (A[1][0] - A[0][1])*s;
    }
  else if (A[0][0] > A[1][1] && A[0][0] > A[2][2])
    {
    s = sqrt(1.0 + A[0][0] - A[1][1] - A[2][2]);
    quat[1] = 0.5*s;
    s = 0.5/s;
    quat[0] = (A[2][1] - A[1][2])*s;
    quat[2] = (A[1][0] + A[0][1])*s;
    quat[3] = (A[0][2] + A[2][0])*s;
   }
  else if (A[1][1] > A[2][2])
    {
    s = sqrt(1.0 - A[0][0] + A[1][1] - A[2][2]);
    quat[2] = 0.5*s;
    s = 0.5/s;
    quat[0] = (A[0][2] - A[2][0])*s;
    quat[1] = (A[1][0] + A[0][1])*s;
    quat[3] = (A[2][1] + A[1][2])*s;
    }
  else
    {
    s = sqrt(1.0 - A[0][0] - A[1][1] + A[2][2]);
    quat[3] = 0.5*s;
    s = 0.5/s;
    quat[0] = (A[1][0] - A[0][1])*s;
    quat[1] = (A[0][2] + A[2][0])*s;
    quat[2] = (A[2][1] + A[1][2])*s;
    }
}

static void vtkQuaternionToMatrix3x3(const double quat[4], double A[3][3])
{
  double ww = quat[0]*quat[0];
  double wx = quat[0]*quat[1];
  double wy = quat[0]*quat[2];
  double wz = quat[0]*quat[3];

  double xx = quat[1]*quat[1];
  double yy = quat[2]*quat[2];
  double zz = quat[3]*quat[3];

  double xy = quat[1]*quat[2];
  double xz = quat[1]*quat[3];
  double yz = quat[2]*quat[3];

  double rr = xx + yy + zz;
  // normalization factor, just in case quaternion was not normalized
  double f = 1.0/(ww + rr);
  double s = (ww - rr)*f;
  f *= 2;

  A[0][0] = xx*f + s;
  A[1][0] = (xy + wz)*f;
  A[2][0] = (xz - wy)*f;

  A[0][1] = (xy - wz)*f;
  A[1][1] = yy*f + s;
  A[2][1] = (yz + wx)*f;

  A[0][2] = (xz + wy)*f;
  A[1][2] = (yz - wx)*f;
  A[2][2] = zz*f + s;
}

//----------------------------------------------------------------------------
// Interpolate the matrix for the given timestamp from the two nearest
// transforms in the buffer.
// The rotation is interpolated with SLERP interpolation, and the
// position is interpolated with linear interpolation.
// The flags are the logical 'or' of the two transformations that
// are used in the interpolation.
long vtkTrackerBuffer::GetFlagsAndMatrixFromTime( vtkMatrix4x4 * matrix, vtkMatrix4x4 * uncalibratedMatrix, double time )
{
    int index0 = this->GetIndexFromTime(time);
    int index1 = index0;
    double indexTime = this->GetTimeStamp(index0);
    double f = indexTime - time;
    double matrix0[3][3];
    double matrix1[3][3];
    double xyz0[3];
    double xyz1[3];
    double quaternion[4];
    long flags0;
    long flags1;
    int i;

    // time difference should be 500 milliseconds or less
    if( f < -0.5 || f > 0.5 )
    {
        vtkErrorMacro( << "Time difference between requested time (" << time << " sec.) and closer available transform time ( " << indexTime << " sec.) is " << f << "> 0.5 sec. " << endl );
        f = 0.0;
    }
    // figure out what values to interpolate between,
    // convert f into a value between 0 and 1
    else if (f > 0)
    {
        f = f/(indexTime - this->GetTimeStamp(index0 + 1));
        index1 = index0 + 1;
    }
    else if (f < 0)
    {
        if (index0 == 0)
        {
            index1 = 0;
            f = 0.0;
        }
        else
        {
            f = 1.0 + f/( this->GetTimeStamp(index0 - 1) - indexTime );
            index1 = index0;
            index0 = index0 - 1;
        }
    }

    flags0 = this->GetFlags(index0);
    flags1 = this->GetFlags(index1);

    this->GetUncalibratedMatrix( matrix, index0 );
    for (i = 0; i < 3; i++)
    {
        matrix0[i][0] = matrix->GetElement(i,0);
        matrix0[i][1] = matrix->GetElement(i,1);
        matrix0[i][2] = matrix->GetElement(i,2);
        xyz0[i] = matrix->GetElement(i,3);
    }
    this->GetUncalibratedMatrix( matrix, index1 );
    for (i = 0; i < 3; i++)
    {
        matrix1[i][0] = matrix->GetElement(i,0);
        matrix1[i][1] = matrix->GetElement(i,1);
        matrix1[i][2] = matrix->GetElement(i,2);
        xyz1[i] = matrix->GetElement(i,3);
    }
    
    vtkMath::Transpose3x3(matrix0, matrix0);
    vtkMath::Multiply3x3(matrix1, matrix0, matrix1);
    vtkMath::Transpose3x3(matrix0, matrix0);
    vtkMatrix3x3ToQuaternion(matrix1, quaternion);

    double s = sqrt(quaternion[1]*quaternion[1] +
                    quaternion[2]*quaternion[2] +
                    quaternion[3]*quaternion[3]);
    double angle = atan2(s, quaternion[0]) * f;
    quaternion[0] = cos(angle);

    if (s > 0.00001)
    {
        s = sin(angle)/s;
        quaternion[1] = quaternion[1]*s;
        quaternion[2] = quaternion[2]*s;
        quaternion[3] = quaternion[3]*s;
    }
    else
    { // use small-angle approximation for sin to avoid
      //  division by very small value
        quaternion[1] = quaternion[1]*f;
        quaternion[2] = quaternion[2]*f;
        quaternion[3] = quaternion[3]*f;
    }

    vtkQuaternionToMatrix3x3(quaternion, matrix1);
    vtkMath::Multiply3x3(matrix1, matrix0, matrix1);

    for (i = 0; i < 3; i++)
    {
        matrix->Element[i][0] = matrix1[i][0];
        matrix->Element[i][1] = matrix1[i][1];
        matrix->Element[i][2] = matrix1[i][2];
        matrix->Element[i][3] = xyz0[i]*(1.0 - f) + xyz1[i]*f;
    }

    matrix->Modified();
    if( uncalibratedMatrix )
        uncalibratedMatrix->DeepCopy( matrix );

    this->ApplyCalibration( matrix );

    return (flags0 | flags1);
}

void vtkTrackerBuffer::ApplyCalibration( vtkMatrix4x4 * matrix )
{
    if (this->ToolCalibrationMatrix)
    {
        vtkMatrix4x4::Multiply4x4( matrix, this->ToolCalibrationMatrix, matrix );
    }

    if (this->WorldCalibrationMatrix)
    {
        vtkMatrix4x4::Multiply4x4( this->WorldCalibrationMatrix, matrix, matrix );
    }
}
