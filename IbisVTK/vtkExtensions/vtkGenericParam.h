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

#ifndef __vtkGenericParam_h_
#define __vtkGenericParam_h_

#include "vtkObject.h"
#include "vtkSetGet.h"
#include <vector>
#include <string>
#include <utility>
#include <assert.h>

class vtkGenericParamInterface;

// Generic way to access values stored by subclasses of vtkGenericParam, mainly used for serialization
class vtkGenericParamValues
{
public:
    vtkGenericParamValues() {}
    void SetParamName( const char * pName ) { m_paramName = pName; }
    const char * GetParamName() { return m_paramName.c_str(); }
    int GetNumberOfItems() { return m_items.size(); }
    const char * GetItemName( int index ) { assert( index < (int)(m_items.size()) ); return m_items[index].first.c_str(); }
    const char * GetItemValue( int index ) { assert( index < (int)(m_items.size()) ); return m_items[index].second.c_str(); }
    void AddItem( const char * itemName, const char * itemValue ) { m_items.push_back( make_pair( std::string(itemName), std::string(itemValue) ) ); }

    typedef std::vector<std::pair<std::string,std::string> > ContainerType;
    ContainerType & GetAllItems() { return m_items; }
protected:
    ContainerType m_items;
    std::string m_paramName;
};

// base class that needs to be implemented by the target to generate it GUI automatically
class vtkGenericParam : public vtkObject
{

public:
    vtkTypeMacro( vtkGenericParam, vtkObject );
    const char * GetParamName() { return m_parameterName.c_str(); }
    virtual void GetParamValues( vtkGenericParamValues & v ) = 0;
    virtual void SetParamValues( vtkGenericParamValues & v ) = 0;
protected:

    void SetParamName( const char * name ) { m_parameterName = name; }
    std::string m_parameterName;

    friend class vtkGenericParamInterface;
};

class vtkComboParam : public vtkGenericParam
{
public:
    vtkTypeMacro( vtkComboParam, vtkGenericParam );
    virtual const char * GetCurrentChoice() = 0;
    virtual void SetCurrentChoice( const char * choice ) = 0;
    virtual void GetValidChoices( std::vector< std::string > & choices ) = 0;
    void SetCurrentChoiceIndex( int index )
    {
        std::vector< std::string > choices;
        GetValidChoices( choices );
        assert( index < (int)(choices.size()) );
        SetCurrentChoice( choices[index].c_str() );
    }
    int GetCurrentChoiceIndex()
    {
        std::string currentChoice( GetCurrentChoice() );
        std::vector< std::string > validChoices;
        GetValidChoices( validChoices );
        for( unsigned i = 0; i < validChoices.size(); ++i )
            if( validChoices[i] == currentChoice )
                return (int)i;
        return -1;
    }
    virtual void GetParamValues( vtkGenericParamValues & v )
    {
        v.SetParamName( GetParamName() );
        v.AddItem( "CurrentChoice", GetCurrentChoice() );
    }
    virtual void SetParamValues( vtkGenericParamValues & v )
    {
        std::string choice( v.GetItemValue( 0 ) );
        SetCurrentChoice( choice.c_str() );
    }
};

template < class Target >
class vtkComboParamImpl : public vtkComboParam
{
public:
    vtkComboParamImpl( Target * t, const char * (Target::*GetCurrentChoicePtr)(), void (Target::*SetCurrentChoicePtr) (const char*), void (Target::*GetValidChoicesPtr) ( std::vector< std::string > & choices ) )
        : m_target( t )
        , m_GetCurrentChoicePtr( GetCurrentChoicePtr )
        , m_SetCurrentChoicePtr( SetCurrentChoicePtr )
        , m_GetValidChoicesPtr( GetValidChoicesPtr ) {}
    virtual const char * GetCurrentChoice() { return ( m_target->*m_GetCurrentChoicePtr )(); }
    virtual void SetCurrentChoice( const char * choice ) { return ( m_target->*m_SetCurrentChoicePtr )( choice ); }
    virtual void GetValidChoices( std::vector< std::string > & choices ) { ( m_target->*m_GetValidChoicesPtr )( choices ); }
private:
    Target * m_target;
    const char * (Target::*m_GetCurrentChoicePtr)();
    void (Target::*m_SetCurrentChoicePtr) ( const char * );
    void (Target::*m_GetValidChoicesPtr) ( std::vector< std::string > & choices );
};

// Classes that want to expose a generic interface should derive from this class.
class vtkGenericParamInterface
{

public:

    virtual ~vtkGenericParamInterface()
    {
        for( unsigned i = 0; i < m_params.size(); ++ i )
            m_params[i]->Delete();
    }
    int GetNumberOfParams() { return m_params.size(); }
    vtkGenericParam * GetParam( int index ) { return m_params[ index ]; }
    vtkGenericParam * GetParam( const char * name )
    {
        std::string nameStr( name );
        for( unsigned i = 0; i < m_params.size(); ++i )
        {
            if( std::string( m_params[i]->GetParamName() ) == nameStr )
                return m_params[i];
        }
        return 0;
    }
    void GetAllParamsValues( std::vector< vtkGenericParamValues > & allValues )
    {
        for( int i = 0; i < GetNumberOfParams(); ++i )
        {
            vtkGenericParamValues values;
            GetParam( i )->GetParamValues( values );
            allValues.push_back( values );
        }
    }
    void SetAllParamsValues( std::vector< vtkGenericParamValues > & allValues )
    {
        for( unsigned i = 0; i < allValues.size(); ++i )
        {
            vtkGenericParamValues & vals = allValues[i];
            vtkGenericParam * p = GetParam( vals.GetParamName() );
            if( p )
                p->SetParamValues( vals );
        }
    }

protected:

    template < class Target >
    void AddComboParam( const char * paramName,
              const char * (Target::*GetCurrentChoicePtr)(),
              void (Target::*SetCurrentChoicePtr) ( const char * ),
              void (Target::*GetValidChoicesPtr) ( std::vector< std::string > & choices ) )
    {
        vtkGenericParam * p = new vtkComboParamImpl< Target >( dynamic_cast<Target*>(this), GetCurrentChoicePtr, SetCurrentChoicePtr, GetValidChoicesPtr );
        p->SetParamName( paramName );
        m_params.push_back( p );
    }

    std::vector< vtkGenericParam * > m_params;
};

#endif
