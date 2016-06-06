#ifndef __vtkMatrix4x4Utilities_h_
#define __vtkMatrix4x4Utilities_h_

inline bool Serialize( Serializer * serial, const char * attrName, vtkMatrix4x4 * mat )
{
    double * elem = (double*)mat->Element;
    return Serialize( serial, attrName, elem, 16 );
}

inline bool AreMatricesEqual( vtkMatrix4x4 * mat1, vtkMatrix4x4 * mat2 )
{
    for( int i = 0; i < 4; ++i )
        for( int j = 0; j < 4; ++j )
            if( mat1->GetElement( i, j ) != mat2->GetElement( i, j ) )
                return false;
    return true;
}

#endif
