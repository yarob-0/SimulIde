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

#include <sstream>
#include <QDebug>

#include "e-logic_device.h"
#include "simulator.h"
#include "circuit.h"

eLogicDevice::eLogicDevice( std::string id )
            : eElement( id )
{
    m_numInputs  = 0;
    m_numOutputs = 0;

    m_inputHighV = 2.5;
    m_inputLowV  = 2.5;
    m_outHighV   = 5;
    m_outLowV    = 0;

    m_inputImp = high_imp;
    m_outImp   = 40;

    m_invInputs = false;
    m_inverted  = false;
    m_clock     = false;
    m_outEnable = true;

    m_clockSource = 0l;
    m_outEnSource = 0l;

    m_etrigger = Trig_Clk;
}
eLogicDevice::~eLogicDevice()
{
    for( eSource* esource: m_output ) delete esource;
    for( eSource* esource: m_input )  delete esource;

    if( m_clockSource ) delete m_clockSource;
    if( m_outEnSource ) delete m_outEnSource;
}

void eLogicDevice::stamp()
{
    // Register for callBack when eNode volt change on clock or OE pins
    if( m_clockSource )
    {
        eNode* enode = m_clockSource->getEpin()->getEnode();
        if( enode ) enode->addToChangedFast(this);
    }
    if( m_outEnSource )
    {
        eNode* enode = m_outEnSource->getEpin()->getEnode();
        if( enode ) enode->addToChangedFast(this);
    }
}

void eLogicDevice::resetState()
{
    if( m_clockSource ) m_clock = false;
    
    for( int i=0; i<m_numOutputs; i++ )
        eLogicDevice::setOut( i, false );
}

bool eLogicDevice::outputEnabled()
{
    if( !m_outEnSource ) return true;

    double volt = m_outEnSource->getVolt();

    if     ( volt > m_inputHighV ) m_outEnable = false;   // Active Low
    else if( volt < m_inputLowV )  m_outEnable = true;

    return m_outEnable;
}

void eLogicDevice::updateOutEnabled()
{
    if( m_outEnSource )
    {
        bool outEn = true;
        bool outEnPrev = m_outEnable;
        outEn = outputEnabled();              // Refresh m_outEnable

        if( outEnPrev != outEn ) setOutputEnabled( outEn );
    }
}

void eLogicDevice::setOutputEnabled( bool enabled )
{
    double imp = 1e28;
    if( enabled ) imp = m_outImp;

    for( int i=0; i<m_numOutputs; i++ ) m_output[i]->setImp( imp );

}

void eLogicDevice::createClockPin()
{
    std::stringstream sspin;
    sspin << m_elmId << "-ePin-clock";
    ePin* epin = new ePin( sspin.str(), 0 );

    createClockeSource( epin );
}

void eLogicDevice::createClockPin( ePin* epin )
{
    std::stringstream sspin;
    sspin << m_elmId << "-ePin-clock";
    epin->setId( sspin.str() );

    createClockeSource( epin );
}

void eLogicDevice::createClockeSource( ePin* epin )
{
    std::stringstream ssesource;
    ssesource << m_elmId << "-eSource-clock";
    m_clockSource = new eSource( ssesource.str(), epin );
    m_clockSource->setImp( m_inputImp );
}

void eLogicDevice::createOutEnablePin()
{
    std::stringstream sspin;
    sspin << m_elmId << "-ePin-outEnable";
    ePin* epin = new ePin( sspin.str(), 0 );

    createOutEnableeSource( epin );
}

void eLogicDevice::createOutEnablePin( ePin* epin )
{
    std::stringstream sspin;
    sspin << m_elmId << "-ePin-outEnable";
    epin->setId( sspin.str() );

    createOutEnableeSource( epin );
}

void eLogicDevice::createOutEnableeSource( ePin* epin )
{
    std::stringstream ssesource;
    ssesource << m_elmId << "-eSource-outEnable";
    m_outEnSource = new eSource( ssesource.str(), epin );
    m_outEnSource->setImp( m_inputImp );
    epin->setInverted( true );
}

void eLogicDevice::createPins( int inputs, int outputs )
{
    setNumInps( inputs );
    setNumOuts( outputs );
}

void eLogicDevice::setInput( int n, eSource* input )
{
    if( m_numInputs < (n+1) )
    {
        m_numInputs = n+1;
        m_input.resize( m_numInputs );
        m_inputState.resize( m_numInputs );
    }
    m_input[n] = input;
}

void eLogicDevice::createInput( ePin* epin )
{
    //qDebug() << "eLogicDevice::createInput"<<QString::fromStdString(m_elmId)<<m_numInputs;
    int totalInps  = m_numInputs + 1;
    m_input.resize( totalInps );
    
    std::stringstream sspin;
    sspin << m_elmId << "-ePin-input" << m_numInputs;
    epin->setId( sspin.str() );

    std::stringstream ssesource;
    ssesource << m_elmId << "-eSource-input" << m_numInputs;
    m_input[m_numInputs] = new eSource( ssesource.str(), epin );
    m_input[m_numInputs]->setImp( m_inputImp );

    m_inputState.resize( totalInps );
    m_inputState[m_numInputs] = false;
    
    m_numInputs = totalInps;
}

void eLogicDevice::createInputs( int inputs )
{
    int totalInps  = m_numInputs + inputs;
    m_input.resize( totalInps );

    for( int i=m_numInputs; i<totalInps; i++ )
    {
        std::stringstream sspin;
        sspin << m_elmId << "-ePin-input" << i;
        ePin* epin = new ePin( sspin.str(), i );

        std::stringstream ssesource;
        ssesource << m_elmId << "-eSource-input" << i;
        m_input[i] = new eSource( ssesource.str(), epin );
        m_input[i]->setImp( m_inputImp );
    }
    m_numInputs = totalInps;
}

void eLogicDevice::createOutput( ePin* epin )
{
    int totalOuts = m_numOutputs + 1;
    m_output.resize( totalOuts );

    std::stringstream sspin;
    sspin << m_elmId << "-ePin-output" << m_numOutputs;
    epin->setId( sspin.str() );

    std::stringstream ssesource;
    ssesource << m_elmId << "-eSource-output" << m_numOutputs;
    m_output[m_numOutputs] = new eSource( ssesource.str(), epin );
    m_output[m_numOutputs]->setVoltHigh( m_outHighV );
    m_output[m_numOutputs]->setImp( m_outImp );

    m_numOutputs = totalOuts;
}

void eLogicDevice::createOutputs( int outputs )
{
    int totalOuts = m_numOutputs + outputs;
    m_output.resize( totalOuts );

    for( int i=m_numOutputs; i<totalOuts; i++ )
    {
        std::stringstream sspin;
        sspin << m_elmId << "-ePin-output" << i;
        ePin* epin = new ePin( sspin.str(), i );

        std::stringstream ssesource;
        ssesource << m_elmId << "-eSource-output" << i;
        m_output[i] = new eSource( ssesource.str(), epin );
        m_output[i]->setVoltHigh( m_outHighV );
        m_output[i]->setImp( m_outImp );
    }
    m_numOutputs = totalOuts;
}

void eLogicDevice::deleteInputs( int inputs )
{
    if( m_numInputs-inputs < 0 ) inputs = m_numInputs;

    for( int i=m_numInputs-1; i>m_numInputs-inputs-1; i-- )
    {
        delete m_input[i];
        m_input.pop_back();
    }
    m_numInputs -= inputs;
}

void eLogicDevice::deleteOutputs( int outputs )
{
    if( m_numOutputs-outputs < 0 ) outputs = m_numOutputs;

    for( int i=m_numOutputs-1; i>m_numOutputs-outputs-1; i-- )
    {
        delete m_output[i];
        m_output.pop_back();
    }
    m_numOutputs -= outputs;
}

void eLogicDevice::setNumInps( int inputs )
{
    if     ( inputs > m_numInputs ) createInputs( inputs - m_numInputs );
    else if( inputs < m_numInputs ) deleteInputs( m_numInputs - inputs );
    else return;
    m_inputState.resize( inputs );
}

void eLogicDevice::setNumOuts( int outputs )
{
    if     ( outputs > m_numOutputs ) createOutputs( outputs - m_numOutputs );
    else if( outputs < m_numOutputs ) deleteOutputs( m_numOutputs - outputs );
}

void eLogicDevice::setOut( int num, bool out )
{
    m_output[num]->setOut( out );
    m_output[num]->stampOutput();
}

void eLogicDevice::setOutHighV( double volt )
{
    bool pauseSim = Simulator::self()->isRunning();
    if( pauseSim ) Simulator::self()->pauseSim();

    m_outHighV = volt;

    for( int i=0; i<m_numOutputs; i++ )
        m_output[i]->setVoltHigh( volt );

    if( pauseSim ) Simulator::self()->runContinuous();
}

void eLogicDevice::setOutLowV( double volt )
{
    bool pauseSim = Simulator::self()->isRunning();
    if( pauseSim ) Simulator::self()->pauseSim();

    m_outLowV = volt;

    for( int i=0; i<m_numOutputs; i++ )
        m_output[i]->setVoltLow( volt );

    if( pauseSim ) Simulator::self()->runContinuous();
}

void eLogicDevice::setInputImp( double imp )
{
    bool pauseSim = Simulator::self()->isRunning();
    if( pauseSim ) Simulator::self()->pauseSim();

    m_inputImp = imp;

    for( int i=0; i<m_numInputs; i++ )
    {
        m_input[i]->setImp( imp );
    }
    if( pauseSim ) Simulator::self()->runContinuous();
}

void eLogicDevice::setOutImp( double imp )
{
    bool pauseSim = Simulator::self()->isRunning();
    if( pauseSim ) Simulator::self()->pauseSim();

    if( m_outImp == imp ) return;

    m_outImp = imp;

    for( int i=0; i<m_numOutputs; i++ )
    {
        m_output[i]->setImp( imp );
    }
    if( pauseSim ) Simulator::self()->runContinuous();
}

void eLogicDevice::setInverted( bool inverted )
{
    bool pauseSim = Simulator::self()->isRunning();
    if( pauseSim ) Simulator::self()->pauseSim();

    m_inverted = inverted;
    
    for( int i=0; i<m_numOutputs; i++ )
    {
        m_output[i]->setInverted( inverted );
    }
    Circuit::self()->update();

    if( pauseSim ) Simulator::self()->runContinuous();
}

void eLogicDevice::setInvertInps( bool invert )
{
    bool pauseSim = Simulator::self()->isRunning();
    if( pauseSim ) Simulator::self()->pauseSim();

    m_invInputs = invert;
    for( int i=0; i<m_numInputs; i++ )
    {
        m_input[i]->setInverted( invert );
    }
    Circuit::self()->update();

    if( pauseSim ) Simulator::self()->runContinuous();
}

void eLogicDevice::seteTrigger( int trigger )
{
    //qDebug() << "eLogicDevice::seteTrigger"<<trigger;
    m_etrigger = trigger;
    m_clock = false;
}

void eLogicDevice::setClockInv( bool inv )     
{
    if( !m_clockSource ) return;

    bool pauseSim = Simulator::self()->isRunning();
    if( pauseSim ) Simulator::self()->pauseSim();

    m_clockSource->setInverted(inv);
    Circuit::self()->update();

    if( pauseSim ) Simulator::self()->runContinuous();
}

int eLogicDevice::getClockState()
{
    if( !m_clockSource ) return Allow;

    int cState = 0;

    bool clock = m_clock;
    double volt = m_clockSource->getVolt(); // Clock pin volt.

    if     ( volt > m_inputHighV ) clock = true;
    else if( volt < m_inputLowV )  clock = false;

    if( m_clockSource->isInverted() ) clock = !clock;

    if( m_etrigger == Trig_InEn )
    {
        if     (!clock ) cState = CLow;
        else if( clock ) cState = Allow;
    }
    else if( m_etrigger == Trig_Clk )
    {
        if     (!m_clock &&  clock ) cState = Rising;
        else if( m_clock &&  clock ) cState = CHigh;
        else if( m_clock && !clock ) cState = Falling;
    }
    else cState = Allow;
    m_clock = clock;

    return cState;
}

bool eLogicDevice::getInputState( int input )
{
    double volt = m_input[input]->getVolt();
    bool  state = m_inputState[input];

    if     ( volt > m_inputHighV ) state = true;
    else if( volt < m_inputLowV )  state = false;

    if( m_input[input]->isInverted() ) state = !state;
    m_inputState[input] = state;
    
    return state;
}

bool eLogicDevice::getOutputState( int output )
{
    return m_output[output]->out();
}

ePin* eLogicDevice::getEpin( QString pinName )
{
    if( pinName.contains("clock")
     || pinName.contains("inputEnable"))
    {
        return m_clockSource->getEpin();
    }
    else if( pinName.contains("outEnable") )
    {
        return m_outEnSource->getEpin();
    }
    else if( pinName.contains("input") )
    {
        int pin = pinName.remove("input").toInt();

        return m_input[pin]->getEpin();
    }
    else if( pinName.contains("output") )
    {
        int pin = pinName.remove("output").toInt();

        return m_output[pin]->getEpin();
    }
    return eElement::getEpin( pinName );
}

