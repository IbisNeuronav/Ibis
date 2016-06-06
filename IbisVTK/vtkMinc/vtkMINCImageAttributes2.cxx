/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMINCImageAttributes2.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2006 Atamai, Inc.

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

#include "vtkMINCImageAttributes2.h"

#include "vtkObjectFactory.h"

#include "vtkStringArray.h"
#include "vtkCharArray.h"
#include "vtkSignedCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkSmartPointer.h"

#include "vtkMINC.h"
#include "vtkMINC2.h"
#include "vtknetcdf/include/netcdf.h"
#include <vtksys/stl/string>
#include <vtksys/stl/map>

#include <stdlib.h>
#include <ctype.h>
#include <float.h>
#include <vtksys/ios/sstream>

//--------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkMINCImageAttributes2, "$Revision: 1.2 $");
vtkStandardNewMacro(vtkMINCImageAttributes2);

//-------------------------------------------------------------------------
vtkMINCImageAttributes2::vtkMINCImageAttributes2()
{
}

//-------------------------------------------------------------------------
vtkMINCImageAttributes2::~vtkMINCImageAttributes2()
{
}

//-------------------------------------------------------------------------
int vtkMINCImageAttributes2::ValidateStudyAttribute(
  const char *vtkNotUsed(varname), const char *attname,
  vtkDataArray *vtkNotUsed(array))
{
  // Attributes for MIstudy variable (vartype = MI_GROUP)
  static const char *studyAttributes[] = {
    MIstudy_id,
    MIstart_time,    // "YYYYMMDDHHMMSS.SS"
    MIstart_year,    // as int (use start_time instead)
    MIstart_month,   // as int (use start_time instead)
    MIstart_day,     // as int (use start_time instead)
    MIstart_hour,    // as int (use start_time instead)
    MIstart_minute,  // as int (use start_time instead)
    MIstart_seconds, // as double or int (use start_time instead)
    MImodality,      // "PET__", "SPECT", "GAMMA", "MRI__", "MRS__",
                     // "MRA__", "CT___", "DSA__", "DR___", "label"
    MImanufacturer,
    MIdevice_model,
    MIinstitution,
    MIdepartment,
    MIstation_id,
    MIreferring_physician,
    MIattending_physician,
    MIradiologist,
    MIoperator,
    MIadmitting_diagnosis,
    MIprocedure,
    // from vtkMinc2.h set for ibis, all as string
    MIapplication,
    MIapplication_version,
    MItracker,
    MItracker_model,
    MItracker_version,
    MItransducer_model,
    MItransducer_rom,
    MItransducer_calibration,
    MIpointer_model,
    MIpointer_rom,
    MIpointer_calibration,
    MIreference_model,
    MIreference_rom,
    MIcalibration_method,
    MIcalibration_version,
    MIcalibration_date,
    // from vtkMinc2.h, found in files
    MIcalibration_time,
    MIfield_value,
    MIperforming_physician,
    MIserial_no,
    MIsoftware_version,
    MIstart_date,
    MIacquisition_id,

    0
  };

  int itry = 0;
  for (itry = 0; studyAttributes[itry] != 0; itry++)
    {
    if (strcmp(attname, studyAttributes[itry]) == 0)
      {
      break;
      }
    }
  if (studyAttributes[itry] != 0)
    {
    // Add checks for correct data type?
    }
  else
    {
    return 2;
    }

  return 1;
}

//-------------------------------------------------------------------------
int vtkMINCImageAttributes2::ValidateAcquisitionAttribute(
  const char *vtkNotUsed(varname), const char *attname,
  vtkDataArray *vtkNotUsed(array))
{
  // Attributes for MIacquisition variable (vartype = MI_GROUP)
  static const char *acquisitionAttributes[] = {
    MIprotocol,
    MIscanning_sequence, // "GR", "SPGR", etc.
    MIrepetition_time,   // as double, milliseconds
    MIecho_time,         // as double, milliseconds
    MIinversion_time,    // as double, milliseconds
    MInum_averages,      // as int
    MIimaging_frequency, // in Hz, as double
    MIimaged_nucleus,    // "H1", "C13", etc. for MRI
    MIradionuclide,      // for PET and SPECT
    MIradionuclide_halflife,
    MIcontrast_agent,
    MItracer,
    MIinjection_time,
    MIinjection_year,
    MIinjection_month,
    MIinjection_day,
    MIinjection_hour,
    MIinjection_minute,
    MIinjection_seconds,
    MIinjection_length,
    MIinjection_dose,
    MIdose_units,
    MIinjection_volume,
    MIinjection_route,
    
    // from vtkMinc2.h set for ibis, all as string
    MIultrasound_depth,
    MIultrasound_scale, 
    MIultrasound_acquisition_type,
    MIultrasound_acquisition_color,
    
    // from vtkMinc2.h, found in files
    MISAR,
    MIacquisition_time,
    MIdelay_in_TR,
    MIecho_number,
    MIecho_train_length,
    MIflip_angle,
    MIimage_time,
    MIimage_type,
    MImr_acq_type,
    MInum_dyn_scans,
    MInum_phase_enc_steps,
    MInum_slices,
    MIpercent_phase_fov,
    MIpercent_sampling,
    MIphase_enc_dir,
    MIpixel_bandwidth,
    MIreceive_coil,
    MIseries_description,
    MIseries_time,
    MIslice_order,
    MIslice_thickness,
    MIstart_time,
    MItransmit_coil,
    MIwindow_center,
    MIwindow_width,
    0
  };

  int itry = 0;
  for (itry = 0; acquisitionAttributes[itry] != 0; itry++)
    {
    if (strcmp(attname, acquisitionAttributes[itry]) == 0)
      {
      break;
      }
    }
  if (acquisitionAttributes[itry] != 0)
    {
    // Add checks for correct data type?
    }
  else
    {
    return 2;
    }

  return 1;
}

int vtkMINCImageAttributes2::ValidatePatientAttribute(
  const char *vtkNotUsed(varname), const char *attname,
  vtkDataArray *vtkNotUsed(array))
{
  // Attributes for MIpatient variable (vartype = MI_GROUP)
  static const char *patientAttributes[] = {
    MIfull_name,     // "LASTNAME^FIRSTNAME SECONDNAME"
    MIother_names,   // newline-separated string
    MIidentification,
    MIother_ids,
    MIbirthdate,     // "YYYYMMDD"
    MIsex,           // "male__", "female", "other_"
    MIage,           // "XXXD", "XXXM", or "XXXY" (days, months, years)
    MIweight,        // "XXkg", "X.Xkg" (assume kg if no units given)
    MIsize,          // "XXXcm" (assume metres if no units given)
    MIaddress,       // newline-separated string
    MIinsurance_id,
    // from vtkMinc2.h, found in files
    MIposition,
    0
  };

  int itry = 0;
  for (itry = 0; patientAttributes[itry] != 0; itry++)
    {
    if (strcmp(attname, patientAttributes[itry]) == 0)
      {
      break;
      }
    }
  if (patientAttributes[itry] != 0)
    {
    // Add checks for correct data type?
    }
  else
    {
    return 2;
    }

  return 1;
}


//-------------------------------------------------------------------------
void vtkMINCImageAttributes2::ShallowCopy(vtkMINCImageAttributes2 *source)
{
    this->Superclass::ShallowCopy((vtkMINCImageAttributes*)source);
}
