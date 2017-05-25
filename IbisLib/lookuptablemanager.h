/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __LookupTableManager_h_
#define __LookupTableManager_h_

#include <QString>
#include "vtkSmartPointer.h"

class vtkLookupTable;
class vtkPiecewiseFunctionLookupTable;

class LookupTableManager
{

public:

    LookupTableManager();

    void CreateLookupTable(const QString tableName, double range[2], vtkSmartPointer<vtkPiecewiseFunctionLookupTable> lut );
    int GetNumberOfTemplateLookupTables() const;
    const QString GetTemplateLookupTableName(int index);
    void CreateLabelLookupTable( vtkSmartPointer<vtkLookupTable> lut );

};

#endif
