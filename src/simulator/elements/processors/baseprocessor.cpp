/***************************************************************************
 *   Copyright (C) 2012 by santiago González                               *
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

#include "baseprocessor.h"
#include "mcucomponent.h"
#include "circuitwidget.h"
#include "mainwindow.h"
#include "simulator.h"
#include "utils.h"
#include "simuapi_apppath.h"

BaseProcessor* BaseProcessor::m_pSelf = 0l;

BaseProcessor::BaseProcessor( QObject* parent )
             : QObject( parent )
             , eElement( "baseprocessor" )
{
    m_pSelf = this;

    m_wordSize = 2; // Size of Program memory word in bytes
    m_romSize = 0;
    m_nextCycle = 0;
    m_loadStatus  = false;
    m_resetStatus = false;
    m_debugging   = false;
    m_symbolFile = "";
    m_device     = "";

    m_ramTable = new RamTable( NULL, this );
    m_ramTable->hide();
}
BaseProcessor::~BaseProcessor() 
{
    delete m_ramTable;
}

void BaseProcessor::setExtraStep() // Run Extra Simulation Step If MCU clock speed > Simulation speed
{
    if( m_mcuStepsPT > 1 ) m_extraCycle = 1; //(uint64_t)((double)cycle()*m_stepNS);
    //qDebug() << "BaseProcessor::setExtraStep" <<m_mcuStepsPT<< m_extraCycle;
}

void BaseProcessor::simuClockStep()
{
    if( !m_loadStatus || m_resetStatus || m_debugging ) return;

    while( m_nextCycle >= 1 )
    {
        stepCpu();
        m_nextCycle -= 1;
        m_cpuTime += m_stepNS;

        if( m_extraCycle > 0 )
        {
            //qDebug() << "-" << m_extraCycle;qDebug() << " ";
            Simulator::self()->runExtraStep( m_cpuTime );
            m_extraCycle = 0;
        }
    }
    m_nextCycle += m_mcuStepsPT;
}

void BaseProcessor::stepOne()
{
    stepCpu();

    while( cycle() >= m_nextCycle )
    {
        m_nextCycle += m_mcuStepsPT;
        runSimuStep();
    }
}

void BaseProcessor::initialized()
{
    //qDebug() << "\nBaseProcessor::initialized  Firmware: " << m_symbolFile;
    //qDebug() << "\nBaseProcessor::initialized Data File: " << m_dataFile;

    m_loadStatus = true;
    m_msimStep = 0;
    m_extraCycle = 0;
    Simulator::self()->addToSimuClockList( this );
}

void BaseProcessor::terminate()
{
    Simulator::self()->remFromSimuClockList( this );
    m_pSelf = NULL;
}

void BaseProcessor::runSimuStep()
{
    Simulator::self()->runCircuit();
    
    m_msimStep++;
    if( m_msimStep >= 50000*Simulator::self()->stepsPerus() ) // 20 fps
    {
        m_msimStep = 0;
        Simulator::self()->runGraphicStep2();
    }
}

void BaseProcessor::setFreq( double freq ) // Instruction exec. freq
{
    m_mcuStepsPT = freq/Simulator::self()->stepsPerus();
    m_stepNS = 1000/freq;
}

QString BaseProcessor::getFileName() { return m_symbolFile; }

void BaseProcessor::setDataFile( QString datafile ) 
{ 
    m_dataFile = datafile;
    setRegisters();
    m_ramTable->setRegisters();
}

int BaseProcessor::status()
{
    return getRamValue( m_statusReg );
}

void BaseProcessor::hardReset( bool rst )
{
    m_resetStatus = rst;
    
    if( rst ) McuComponent::self()->reset();
}

int BaseProcessor::getRegAddress( QString name ) 
{
    name = name.toUpper();
    if( m_regsTable.contains( name ) ) return m_regsTable.value( name ); 
    return -1;
}

void BaseProcessor::updateRamValue( QString name )
{
    if( !m_loadStatus ) return;

    name = name.toUpper();
    QString type = "";
    if( m_typeTable.contains( name )) type = m_typeTable[ name ];
    else return;
    
    QByteArray ba;
    ba.resize(4);
    int address = getRegAddress( name );
    if( address < 0 ) return;

    int bits = 8;
    
    if( type.contains( "32" ) )    // 4 bytes
    {
        bits = 32;
        ba[0] = getRamValue( address );
        ba[1] = getRamValue( address+1 );
        ba[2] = getRamValue( address+2 );
        ba[3] = getRamValue( address+3 );
    }
    else if( type.contains( "16" ) )  // 2 bytes
    {
        bits = 16;
        ba[0] = getRamValue( address );
        ba[1] = getRamValue( address+1 );
        ba[2] = 0;
        ba[3] = 0;
    }
    else                                  // 1 byte
    {
        ba[0] = getRamValue( address );
        ba[1] = 0;
        ba[2] = 0;
        ba[3] = 0;
    }
    if( type.contains( "f" ) )                          // float, double
    {
        float value = 0;
        memcpy(&value, ba, 4);
        m_ramTable->setItemValue( 1, value  );
    }
    else                                              // char, int, long
    {
        int32_t value = 0;
        
        if( type.contains( "u" ) ) 
        {
            uint32_t val = 0;
            memcpy(&val, ba, 4);
            value = val;
        }
        else 
        {
            if( bits == 32 )
            {
                int32_t val = 0;
                memcpy(&val, ba, 4);
                
                value = val;
            }
            else if( bits == 16 )
            {
                int16_t val = 0;
                memcpy(&val, ba, 2);
                
                value = val;
            }
            else
            {
                int8_t val = 0;
                memcpy(&val, ba, 1);
                
                value = val;
            }
        }
        m_ramTable->setItemValue( 2, value  );
        
        if     ( type.contains( "8" ) ) m_ramTable->setItemValue( 3, decToBase(value, 2, 8)  );
        else if( type.contains( "string" ) ) 
        {
            QString strVal = "";
            for( int i=address; i<=address+value; i++ )
            {
                QString str = "";
                const QChar cha = getRamValue( i );
                str.setRawData( &cha, 1 );
                
                strVal += str; //QByteArray::fromHex( getRamValue( i ) );
            }
            //qDebug() << "string" << name << value << strVal;
            m_ramTable->setItemValue( 3, strVal  );
        }
        
    }
    //qDebug()<<name<<type <<address<<value;
    //if( !type.contains( "8" ) ) 
    m_ramTable->setItemValue( 1, type  );
}


int BaseProcessor::getRamValue( QString name )
{
    if( m_regsTable.isEmpty() ) return -1;

    bool isNumber = false;
    int address = name.toInt( &isNumber );      // Try to convert to integer

    if( !isNumber ) 
    { 
        address = m_regsTable[name.toUpper()];  // Is a register name
    }

    return getRamValue( address );
}

void BaseProcessor::addWatchVar( QString name, int address, QString type )
{
    name = name.toUpper();
    if( !m_regsTable.contains(name) ) m_regList.append( name );
    m_regsTable[ name ] = address;
    m_typeTable[ name ] = type;
}

void BaseProcessor::setRegisters() // get register addresses from data file
{
    QStringList lineList = fileToStringList( m_dataFile, "BaseProcessor::setRegisters" );

    if( !m_regsTable.isEmpty() ) 
    {
        m_regList.clear();
        m_regsTable.clear();
        m_typeTable.clear();
    }

    for( QString line : lineList )
    {
        if( line.contains("EQU ") )   // This line contains a definition
        {
            line = line.replace("\t"," ");

            QString name    = "";
            QString addrtxt = "";
            int address   = 0;
            bool isNumber = false;

            line.remove(" ");
            QStringList wordList = line.split("EQU"); // Split in words
            if( wordList.size() < 2 ) continue;

            name    = wordList.takeFirst();
            while( addrtxt.isEmpty() ) addrtxt = wordList.takeFirst();

            address = addrtxt.toInt( &isNumber, 0 );
            
            if( isNumber )        // If found a valid address add to map
            {
                address = validate( address );
                addWatchVar( name, address, "u8" );        // type uint8 
            }
            //qDebug() << name << address<<"\n";
        }
    }
}

QVector<int>* BaseProcessor::eeprom()
{
    for( uint32_t i=0; i<m_romSize; ++i ) m_eeprom[i] = getRomValue( i );
    return &m_eeprom;
}

void BaseProcessor::setEeprom( QVector<int>* eep )
{
    int size = m_romSize;
    if( eep->size() < size ) size = eep->size();

    for( int i=0; i<size; ++i ) setRomValue( i, eep->at(i) );
}

void BaseProcessor::uartOut( int uart, uint32_t value ) // Send value to OutPanelText
{
    emit uartDataOut( uart, value );

    //qDebug()<<"BaseProcessor::uartOut" << uart << value;
}

void BaseProcessor::uartIn( int uart, uint32_t value ) // Receive one byte on Uart
{
    //qDebug()<<"BaseProcessor::uartIn" << uart << value;
    emit uartDataIn( uart, value );
}

#include "moc_baseprocessor.cpp"
