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

#include "flipflopbase.h"
#include "circuitwidget.h"
#include "simulator.h"
#include "circuit.h"
#include "pin.h"


FlipFlopBase::FlipFlopBase( QObject* parent, QString type, QString id )
         : LogicComponent( parent, type, id )
         , eLogicDevice( id.toStdString() )
{
    m_dataPins = 0;
}
FlipFlopBase::~FlipFlopBase(){}

void FlipFlopBase::stamp()
{
    m_Q0 = 0;
    eNode* enode = m_setPin->getEpin(0)->getEnode();         // Set pin
    if( enode ) enode->addToChangedFast( this );

    enode = m_resetPin->getEpin(0)->getEnode();              // Reset pin
    if( enode ) enode->addToChangedFast( this );

    if( m_etrigger != Trig_Clk )
    {
        for( int i=0; i<m_dataPins; i++ ) // J K or D
        {
            eNode* enode = m_input[i]->getEpin(0)->getEnode();
            if( enode ) enode->addToChangedFast( this );
        }
    }
    eLogicDevice::stamp();
}

void FlipFlopBase::setTrigger( Trigger trigger )
{
    if( Simulator::self()->isRunning() ) CircuitWidget::self()->powerCircOff();

    int trig = static_cast<int>( trigger );
    eLogicDevice::seteTrigger( trig );
    LogicComponent::setTrigger( trigger );

    Circuit::self()->update();
}

void FlipFlopBase::setSrInv( bool inv )
{
    m_srInv = inv;
    m_setPin->setInverted( inv ); // Set
    m_resetPin->setInverted( inv ); // Reset

    Circuit::self()->update();
}

#include "moc_flipflopbase.cpp"
