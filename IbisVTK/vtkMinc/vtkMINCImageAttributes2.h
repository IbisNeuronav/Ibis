/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMINCImageAttributes2.h,v $

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
// .NAME vtkMINCImageAttributes - A container for a MINC image header.
// .SECTION Description
// This class provides methods to access all of the information
// contained in the MINC header.  If you read a MINC file into
// VTK and then write it out again, you can use 
// writer->SetImageAttributes(reader->GetImageAttributes) to
// ensure that all of the medical information contained in the
// file is tranferred from the reader to the writer.  If you
// want to change any of the header information, you must 
// use ShallowCopy to make a copy of the reader's attributes
// and then modify only the copy.
// .SECTION See Also
// vtkMINCImageReader vtkMINCImageWriter vtkMINCImageAttributes
// .SECTION Thanks
// Thanks to David Gobbi for writing this class and Atamai Inc. for
// contributing it to VTK.

#ifndef __vtkMINCImageAttributes2_h
#define __vtkMINCImageAttributes2_h

#include "vtkMINCImageAttributes.h"

class vtkDataArray;

class VTK_IO_EXPORT vtkMINCImageAttributes2 : public vtkMINCImageAttributes
{
public:
  vtkTypeRevisionMacro(vtkMINCImageAttributes2,vtkMINCImageAttributes);

  static vtkMINCImageAttributes2 *New();

  // Description:
  // Do a shallow copy.  This will copy all the attributes
  // from the source.  It is much more efficient than a DeepCopy
  // would be, since it only copies pointers to the attribute values
  // instead of copying the arrays themselves.  You must use this
  // method to make a copy if you want to modify any MINC attributes
  // from a MINCReader before you pass them to a MINCWriter.
  virtual void ShallowCopy(vtkMINCImageAttributes2 *source);


protected:
  vtkMINCImageAttributes2();
  virtual ~vtkMINCImageAttributes2();

  virtual int ValidateStudyAttribute(const char *varname,
                                     const char *attname,
                                     vtkDataArray *array);
  virtual int ValidateAcquisitionAttribute(const char *varname,
                                           const char *attname,
                                           vtkDataArray *array);
  virtual int ValidatePatientAttribute(const char *varname,
                                           const char *attname,
                                           vtkDataArray *array);


private:
  vtkMINCImageAttributes2(const vtkMINCImageAttributes2&); // Not implemented
  void operator=(const vtkMINCImageAttributes2&);  // Not implemented

};

#endif /* __vtkMINCImageAttributes2_h */
