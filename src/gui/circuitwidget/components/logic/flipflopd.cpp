/***************************************************************************
 *   Copyright (C) 2016 by santiago González                               *
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

#include "flipflopd.h"
#include "itemlibrary.h"
#include "simulator.h"

Component* FlipFlopD::construct( QObject* parent, QString type, QString id )
{
    return new FlipFlopD( parent, type, id );
}

LibraryItem* FlipFlopD::libraryItem()
{
    return new LibraryItem(
        tr( "FlipFlop D" ),
        tr( "Logic/Memory" ),
        "2to2.png",
        "FlipFlopD",
        FlipFlopD::construct );
}

FlipFlopD::FlipFlopD( QObject* parent, QString type, QString id )
         : FlipFlopBase( parent, type, id )
{
    m_width  = 3;
    m_height = 3;
    
    QStringList pinList;

    pinList // Inputs:
            << "IL01D"
            << "IU01S"
            << "ID02R"
            << "IL02>"
            
            // Outputs:
            << "OR01Q"
            << "OR02!Q"
            ;
    init( pinList );
    
    eLogicDevice::createInput( m_inPin[0] );                  // Input D
    eLogicDevice::createInput( m_inPin[1] );                  // Input S
    eLogicDevice::createInput( m_inPin[2] );                  // Input R

    m_setPin = m_input[1];
    m_resetPin = m_input[2];
    m_dataPins = 1;
    
    m_trigPin = m_inPin[3];
    eLogicDevice::createClockPin( m_trigPin );             // Input Clock
    
    eLogicDevice::createOutput( m_outPin[0] );               // Output Q
    eLogicDevice::createOutput( m_outPin[1] );               // Output Q'

    setSrInv( true );                           // Inver Set & Reset pins
    setClockInv( false );                        //Don't Invert Clock pin
    setTrigger( Clock );
}
FlipFlopD::~FlipFlopD(){}

void FlipFlopD::setVChanged()
{
    bool clkAllow = (getClockState() == Allow); // Get Clk to don't miss any clock changes

    bool set   = getInputState( 1 );
    bool reset = getInputState( 2 );

    if( set || reset)
    {
        eLogicDevice::setOut( 0, set );
        eLogicDevice::setOut( 1, reset );
    }
    else if( clkAllow )
    {
        m_Q0 = getInputState( 0 ); // D state
        eLogicDevice::setOut( 0, m_Q0 );
        eLogicDevice::setOut( 1, !m_Q0 );
    }
}

#include "moc_flipflopd.cpp"
