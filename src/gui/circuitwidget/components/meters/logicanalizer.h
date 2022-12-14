/***************************************************************************
 *   Copyright (C) 2018 by santiago González                               *
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

#ifndef LANALIZER_H
#define LANALIZER_H

#include "plotbase.h"

enum paCond {
    None = 0,
    Rising,
    Falling,
    High,
    Low
};

class LibraryItem;
class LaChannel;
class LaWidget;
class DataLaWidget;

class MAINMODULE_EXPORT LAnalizer : public PlotBase
{
    Q_OBJECT

    //Q_PROPERTY( bool    Data_Log READ paOnCond WRITE setPaOnCond DESIGNABLE true USER true )
    //Q_PROPERTY( double  Log_us   READ dataSize WRITE setDataSize DESIGNABLE true USER true )
    Q_PROPERTY( double  vTick    READ voltDiv  WRITE setVoltDiv )
    Q_PROPERTY( int     Trigger  READ trigger  WRITE setTrigger )
    Q_PROPERTY( qint64  TimePos  READ timePos  WRITE setTimePos )

    public:

        LAnalizer( QObject* parent, QString type, QString id );
        ~LAnalizer();

        Q_ENUM( paCond )

        static Component* construct( QObject* parent, QString type, QString id );
        static LibraryItem* libraryItem();

        double dataSize() { return m_dataSize/1e6; }
        void setDataSize( double ds ) { m_dataSize = ds*1e6; }

        bool paOnCond() { return m_paOnCond; }
        void setPaOnCond( bool pa ) { m_paOnCond = pa; }

        virtual void updateStep() override;

        virtual void setTimeDiv( uint64_t td ) override;

        int64_t timePos(){ return m_timePos; }
        void setTimePos( int64_t tp );

        double voltDiv(){ return m_voltDiv; }
        void setVoltDiv( double vd );

        int trigger() { return m_trigger; }
        void setTrigger( int ch );

        virtual QStringList tunnels() override;
        virtual void setTunnels( QStringList tunnels ) override;

        virtual void expand( bool e ) override;

        virtual void channelChanged( int ch, QString name ) override;

        void pauseOnCond();

    private:
        bool m_paOnCond;
        paCond m_refCond;

        double m_voltDiv;

        int m_trigger;
        int m_updtCount;

        int64_t m_timePos;

        LaChannel* m_channel[8];
        LaWidget*  m_laWidget;
        DataLaWidget* m_dataWidget;
};

#endif

