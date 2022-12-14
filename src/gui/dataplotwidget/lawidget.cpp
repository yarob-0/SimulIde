/***************************************************************************
 *   Copyright (C) 2021 by santiago González                               *
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

#include "lawidget.h"
#include "logicanalizer.h"
#include "utils.h"


LaWidget::LaWidget( QWidget* parent , LAnalizer* la )
        : QDialog( parent )
{
    setupUi(this);

    m_analizer = la;
    m_blocked = false;
    m_channel = 4;

    QFont font = timeDivLabel->font();
    font.setFamily("Ubuntu");
    font.setBold( true );
    font.setPixelSize( 14 );
    timeDivLabel->setFont( font );
    timePosLabel->setFont( font );
    voltDivLabel->setFont( font );
    trigLabel->setFont( font );

    font.setPixelSize( 10 );
    timeDivBox->setFont( font );
    timePosBox->setFont( font );
    voltDivBox->setFont( font );
    triggerBox->setFont( font );
    condEdit->setFont( font );
}
LaWidget::~LaWidget()
{}

void LaWidget::on_timeDivDial_valueChanged( int DialPos )
{
    uint64_t timeDiv = m_analizer->timeDiv();
    uint64_t   delta = timeDiv/100;
    if( delta < 1 ) delta = 1;

    if( DialPos < m_timeDivDialPos ) timeDiv -= delta;
    else                             timeDiv += delta;

    m_analizer->setTimeDiv( timeDiv );

    m_timeDivDialPos = DialPos;
}

void LaWidget::on_timeDivBox_valueChanged( double timeDiv ) // User entered value
{
    if( m_blocked ) return;

    QString unit = timeDivBox->suffix().remove("s");
    unitToVal( timeDiv, unit );

    m_analizer->setTimeDiv( timeDiv/1000 );
}

void LaWidget::updateTimeDivBox( uint64_t timeDiv )
{
    m_blocked = true;
    double val = timeDiv*1e3; // *1e3 bcos here sim time is in ns
    QString unit = " p";
    int Vdecimals = 0;
    valToUnit( val, unit, Vdecimals );

    timeDivBox->setDecimals( Vdecimals );
    timeDivBox->setValue( val );
    timeDivBox->setSuffix( unit+"s" );
    m_blocked = false;
}

void LaWidget::on_timePosDial_valueChanged( int DialPos )
{
    int64_t timePos = m_analizer->timePos();
    int64_t   delta = m_analizer->timeDiv()/100;
    if( delta < 1 ) delta = 1;

    if( DialPos < m_timePosDialPos ) timePos += delta;
    else                             timePos -= delta;

    m_analizer->setTimePos( timePos );

    m_timePosDialPos = DialPos;
}

void LaWidget::on_timePosBox_valueChanged( double timePos )
{
    if( m_blocked ) return;

    QString unit = timePosBox->suffix().remove("s");
    unitToVal( timePos, unit );

    m_analizer->setTimePos( timePos/1000 );
}

void LaWidget::updateTimePosBox( int64_t timePos )
{
    m_blocked = true;

    double val = timePos*1e3;
    QString unit = " p";
    int Vdecimals = 0;
    valToUnit( val, unit, Vdecimals )

    timePosBox->setDecimals( Vdecimals );
    timePosBox->setValue( val );
    timePosBox->setSuffix( unit+"s" );
    m_blocked = false;
}

void LaWidget::on_voltDivDial_valueChanged( int DialPos )
{
    double voltDiv = m_analizer->voltDiv();
    double delta = voltDiv/100;
    if( DialPos < m_voltDivDialPos ) voltDiv += delta;
    else                             voltDiv -= delta;

    m_analizer->setVoltDiv( voltDiv );

    m_voltDivDialPos = DialPos;
}

void LaWidget::on_voltDivBox_valueChanged( double voltDiv )
{
    if( m_blocked ) return;

    QString unit = voltDivBox->suffix().remove("V");
    unitToVal( voltDiv, unit );

    m_analizer->setVoltDiv( voltDiv/1e12 );
}

void LaWidget::updateVoltDivBox( double voltDiv )
{
    m_blocked = true;

    double val  = voltDiv*1e12;
    QString unit = " p";
    int Vdecimals = 0;
    valToUnit( val, unit, Vdecimals );

    voltDivBox->setDecimals( Vdecimals );
    voltDivBox->setValue( val );
    voltDivBox->setSuffix( unit+"V" );
    m_blocked = false;
}

void LaWidget::on_triggerBox_currentIndexChanged( int index )
{
    m_analizer->setTrigger( index );
}

void LaWidget::setTrigger( int ch )
{
    triggerBox->setCurrentIndex( ch );
}

void LaWidget::closeEvent( QCloseEvent* event )
{
    if( !parent() ) return;
    m_analizer->expand( false );
    QWidget::closeEvent( event );
}

void LaWidget::resizeEvent( QResizeEvent* event )
{
    //plotDisplay->updateValues();
}
