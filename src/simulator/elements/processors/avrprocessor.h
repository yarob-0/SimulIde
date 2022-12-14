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

#ifndef AVRPROCESSOR_H
#define AVRPROCESSOR_H

#include <QtGui>
#include <QHash>

#include "baseprocessor.h"

// simavr includes
#include "sim_avr.h"
struct avr_t;

class AvrProcessor : public BaseProcessor
{
    Q_OBJECT

    public:
        AvrProcessor( QObject* parent=0 );
        ~AvrProcessor();

        bool initGdb();
        void setInitGdb( bool init );

        virtual bool setDevice( QString device ) override;
        virtual bool loadFirmware( QString file ) override;
        virtual void terminate() override;

        void reset();
        void stepCpu(){ if( m_avrProcessor->state < cpu_Done )
                            m_avrProcessor->run( m_avrProcessor );}

        virtual int pc() override;
        virtual uint64_t cycle() override { return m_avrProcessor->cycle; }

        virtual uint8_t getRamValue( int address ) override;
        virtual void   setRamValue( int address, uint8_t value ) override;
        virtual uint16_t getFlashValue( int address ) override;
        virtual void     setFlashValue( int address, uint16_t value ) override;
        virtual uint8_t getRomValue( int address ) override;
        virtual void    setRomValue( int address, uint8_t value ) override;
        
        avr_t* getCpu() { return m_avrProcessor; }
        void setCpu( avr_t* avrProc ) { m_avrProcessor = avrProc; }

        virtual void setRegisters();

        void uartIn( int uart, uint32_t value );
        
        static void uart_pty_out_hook( struct avr_irq_t* irq, uint32_t value, void* param )
        {
            Q_UNUSED(irq);
            // get the pointer out of param and asign it to AvrProcessor*
            //AvrProcessor* ptrAvrProcessor = reinterpret_cast<AvrProcessor*> (param);
            
            //ptrAvrProcessor->uartOut( value );
            intptr_t uart = reinterpret_cast<intptr_t> (param);
            BaseProcessor::self()->uartOut( uart, value );
        }

    private:
        virtual int  validate( int address );

        bool m_initGdb;

        //From simavr
        uint8_t* m_avrEEPROM;
        avr_t*     m_avrProcessor;
        QVector<avr_irq_t*> m_uartInIrq;
};


#endif
