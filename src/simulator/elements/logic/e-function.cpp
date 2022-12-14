/***************************************************************************
 *   Copyright (C) 2010 by santiago González                               *
 *   santigoro@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 *                                                                         *
 ***************************************************************************/

#include "e-function.h"
#include "simulator.h"

eFunction::eFunction( std::string id )
         : eLogicDevice( id )
         , m_engine()
         , m_functions()
{
}
eFunction::~eFunction()
{
}

void eFunction::stamp()
{
    eLogicDevice::stamp();
    
    for( int i=0; i<m_numInputs; i++ )
    {
        eNode* enode = m_input[i]->getEpin()->getEnode();
        if( enode ) enode->addToChangedFast(this);
    }
}

void eFunction::setVChanged()
{
    for( int i=0; i<m_numInputs; i++ )
    {
        m_engine.globalObject().setProperty( "i"+QString::number(i), QScriptValue( eLogicDevice::getInputState( i )) );
        m_engine.globalObject().setProperty( "vi"+QString::number(i), QScriptValue( m_input[i]->getEpin()->getVolt()) );
    }

    for( int i=0; i<m_numOutputs; i++ )
    {
        m_engine.globalObject().setProperty( "o"+QString::number(i), QScriptValue( eLogicDevice::getOutputState( i )) );
        m_engine.globalObject().setProperty( "vo"+QString::number(i), QScriptValue( m_output[i]->getEpin()->getVolt()) );
    }
        
    for( int i=0; i<m_numOutputs; i++ )
    {
        QString text = m_funcList.at(i).toLower();
            
        if( text.startsWith( "vo" ) )
        {
            float out = m_engine.evaluate( text ).toNumber();
            m_output[i]->setVoltHigh( out );
            eLogicDevice::setOut( i, true );
        }
        else
        {
            bool out = m_engine.evaluate( text ).toBool();
            eLogicDevice::setOut( i, out );
        }
    }
}

QString eFunction::functions()
{
    return m_functions;
}

void eFunction::setFunctions( QString f )
{
    if( f.isEmpty() ) return;
    m_functions = f;
    m_funcList = f.split(",");
}
