/*=========================================================================

        Program:   Visualization Toolkit
        Module:    vtkFrameToTimeStampConverter.cxx

    David Gobbi 
    Imaging Research Laboratories
        John P. Robarts Research Institute
        Box 5015, 100 Perth Drive
    London, Ontario

        dgobbi@irus.rri.on.ca
    (519)663-5777x34213

==========================================================================

Copyright (c) 2000 Atamai, Inc.

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

3) Modified copies of the source code must be clearly marked as such,
   and must not be misrepresented as verbatim copies of the source code.

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

#include <limits.h>
#include <float.h>
#include <math.h>
#include "vtkFrameToTimeStampConverter.h"
#include "vtkTimerLog.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkFrameToTimeStampConverter* vtkFrameToTimeStampConverter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkFrameToTimeStampConverter");
  if(ret)
    {
    return (vtkFrameToTimeStampConverter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkFrameToTimeStampConverter;
}

//----------------------------------------------------------------------------
vtkFrameToTimeStampConverter::vtkFrameToTimeStampConverter()
{
  this->NominalFrequency = 100.0;
  this->LastTimeStamp = 0;
  this->LastFrameCount = 0;
  this->LastLastFrameCount = 0;
  this->EstimatedFramePeriod = 1.0/this->NominalFrequency;
  this->NextFramePeriod = this->EstimatedFramePeriod;  
}

//----------------------------------------------------------------------------
vtkFrameToTimeStampConverter::~vtkFrameToTimeStampConverter()
{
}

//----------------------------------------------------------------------------
void vtkFrameToTimeStampConverter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);
  
  os << indent << "NominalFrequency: " << this->NominalFrequency << "\n";
  os << indent << "LastFrame: " << this->LastFrameCount << "\n";
  os << indent << "InstantaneousFrequency:" <<
    this->GetInstantaneousFrequency() << "\n";
}

//----------------------------------------------------------------------------
void vtkFrameToTimeStampConverter::Initialize()
{
  this->LastTimeStamp = 0;
  this->LastFrameCount = 0;
  this->LastLastFrameCount = 0;
  this->EstimatedFramePeriod = 1.0/this->NominalFrequency;
  this->NextFramePeriod = this->EstimatedFramePeriod;  
}

//----------------------------------------------------------------------------
void vtkFrameToTimeStampConverter::SetLastFrame(unsigned long framecount)
{
  if (framecount <= this->LastFrameCount)
    {
    return;
    }

  // read the system clock
  double timestamp = vtkTimerLog::GetUniversalTime();

  double frameperiod = ((timestamp - this->LastTimeStamp)/
                        (framecount - this->LastFrameCount));
  double deltaperiod = frameperiod - this->EstimatedFramePeriod;

  this->LastTimeStamp += ((framecount - this->LastFrameCount)*
                          this->NextFramePeriod);
  this->LastLastFrameCount = this->LastFrameCount;
  this->LastFrameCount = framecount;

  // check the difference between the system clock and the
  // 'predicted' time for this framecount
  double diffperiod = (timestamp - this->LastTimeStamp);

  if (diffperiod < -0.2 || diffperiod > 0.2 || 
      framecount > 10 + this->LastLastFrameCount)
    { // time is off by more than 0.2 seconds: reset the clock
    this->NextFramePeriod = this->EstimatedFramePeriod;
    this->LastTimeStamp = timestamp;
    return;
    }

  // update our estimate of the current frame period based on the
  // measured period (the measured period will have a large error
  // associated with it, hence we only use 1% of the difference)
  this->EstimatedFramePeriod += deltaperiod*0.01;

  // vary the period for the next time round, but only let it
  // fluctuate by 1ms relative to the estimated frame period
  diffperiod *= 0.1;
  double maxdiff = 0.001;
  if (diffperiod < -maxdiff)
    {
    diffperiod = -maxdiff;
    }
  else if (diffperiod > maxdiff)
    {
    diffperiod = maxdiff;
    }
 
  this->NextFramePeriod = this->EstimatedFramePeriod + diffperiod;
  /*
  fprintf(stderr, "T %4i %i %.4f %.4f %.3f %.3f %f %f\n",
          framecount, framecount - this->LastLastFrameCount,
          this->EstimatedFramePeriod, this->NextFramePeriod,
          this->LastTimeStamp, timestamp, deltaperiod, diffperiod);
  */
}

//----------------------------------------------------------------------------
double vtkFrameToTimeStampConverter::GetTimeStampForFrame(unsigned long frame)
{
  return this->LastTimeStamp - 
    this->EstimatedFramePeriod*(this->LastFrameCount - frame);
}

//----------------------------------------------------------------------------
double vtkFrameToTimeStampConverter::GetInstantaneousFrequency()
{
  return 1.0/this->EstimatedFramePeriod;
}
 





