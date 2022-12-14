/***************************************************************************
 *   Copyright (C) 2019 by santiago González                               *
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

#include "ky040.h"
#include "pin.h"
#include "simulator.h"
#include "circuit.h"

#define WIDTH 40
#define HEIGHT 56
#define GAP 0
#define DIAL_SIZE 36

#define VIN 5

const bool clockwiseSequence[2][4] = { {true, false, false, true}, {false, false, true, true} };
const bool counterClockwiseSequence[2][4] = { {true, true, false, false}, {false, true, true, false} };

Component* KY040::construct( QObject* parent, QString type, QString id )
{ return new KY040( parent, type, id ); }

LibraryItem* KY040::libraryItem()
{
    return new LibraryItem(
            tr( "KY-040" ),
            tr( "Sensors" ),
            "ky-040.png",
            "KY040",
            KY040::construct);
}

KY040::KY040( QObject* parent, QString type, QString id )
    : Component( parent, type, id )
    , eElement( id.toStdString() )
{
    m_changed = false;
    m_closed = false;

    m_dialW.setupWidget();
    m_dialW.setFixedSize( DIAL_SIZE, DIAL_SIZE );

    m_sequenceIndex = -1;

    m_dial = m_dialW.dial;
    m_dial->setWrapping(true);
    setDetents(20);

    m_proxy = Circuit::self()->addWidget( &m_dialW );
    m_proxy->setParentItem( this );
    m_proxy->setPos( QPoint(-WIDTH/2+(WIDTH-DIAL_SIZE)/2, -HEIGHT/2+(WIDTH-DIAL_SIZE)/2+GAP) );

    m_button = new QToolButton();
    m_button->setMaximumSize( 10,10 );
    m_button->setGeometry(-10,-10,10,10);

    m_proxy_button = Circuit::self()->addWidget( m_button );
    m_proxy_button->setParentItem( this );
    m_proxy_button->setPos( QPoint(8, HEIGHT/2-13) );

    m_area = QRect( -WIDTH/2, -HEIGHT/2 + GAP, WIDTH, HEIGHT );
    setLabelPos(-WIDTH/2, -HEIGHT/2 - GAP, 0);

    m_pin.resize(3);

    QString pinid = id;
    pinid.append(QString("-dt"));
    QPoint pinpos = QPoint(-4,36);
    m_dtpin = new Pin( 270, pinpos, pinid, 0, this);
    m_dtpin->setLabelText( " DT" );
    m_pin[0] = m_dtpin;

    pinid.append(QString("-eSource"));
    m_dt = new eSource( pinid.toStdString(), m_dtpin );
    m_dt->setVoltHigh( VIN );
    m_dt->setImp( 40 );

    pinid = id;
    pinid.append(QString("-clk"));
    pinpos = QPoint(4,36);
    m_clkpin = new Pin( 270, pinpos, pinid, 0, this);
    m_clkpin->setLabelText( " CLK" );
    m_pin[1] = m_clkpin;

    pinid.append(QString("-eSource"));
    m_clk = new eSource( pinid.toStdString(), m_clkpin );
    m_clk->setVoltHigh( VIN );
    m_clk->setImp( 40 );

    pinid = id;
    pinid.append(QString("-sw"));
    pinpos = QPoint(-12,36);
    m_swpin = new Pin( 270, pinpos, pinid, 0, this);
    m_swpin->setLabelText( " SW" );
    m_pin[2] = m_swpin;

    pinid.append(QString("-eSource"));
    m_sw = new eSource( pinid.toStdString(), m_swpin );
    m_sw->setVoltHigh( VIN );
    m_sw->setOut( !m_closed );
    m_sw->setImp( 40 );

    Simulator::self()->addToSimuClockList( this );

    connect( m_dial, SIGNAL( valueChanged(int)),
             this,   SLOT  ( posChanged(int)) );

    connect( m_button, SIGNAL( pressed() ),
             this,     SLOT  ( onbuttonpressed() ));

    connect( m_button, SIGNAL( released() ),
             this,     SLOT  ( onbuttonreleased() ));

    resetState();
}
KY040::~KY040(){ Simulator::self()->remFromSimuClockList( this ); }

void KY040::stamp()
{
    eNode* enode = m_clkpin->getEnode(); // Register for Trigger Pin changes
    if( enode ) enode->addToChangedFast(this);
    
    enode = m_clkpin->getEnode(); // Register for Trigger Pin changes
    if( enode ) enode->addToChangedFast(this);
}

void KY040::onbuttonpressed()
{
    m_closed = true;
    m_changed = true;
    
    update();
}

void KY040::onbuttonreleased()
{
    m_closed = false;
    m_changed = true;
    
    update();
}

int KY040::detents () {
    return m_detents;
}

void KY040::setDetents( int val ) 
{
    if( val < 10 ) val = 10;
    
    m_detents = val;
    m_dial->setMinimum(1);
    m_dial->setMaximum(val);
    m_dial->setValue(1);
    m_dial->setSingleStep(1);
}

void KY040::resetState()
{

}

void KY040::posChanged( int value )
{
    if (m_previousDialValue == value) return;

    m_changed = true;
    m_sequenceIndex = 0;
    
    m_changeClockwise = (m_previousDialValue < value
                        || (m_previousDialValue > m_detents-3
                            && value < m_detents-3));
    
    m_previousDialValue = value;
}

void KY040::simuClockStep()
{
    if( m_sequenceIndex >= 0 )
    {
        m_outputCount--;
        
        if( m_outputCount <= 0 )
        {
            bool dtOuput;
            bool clkOuput;
            
            if (m_changeClockwise) {
                dtOuput = clockwiseSequence[0][m_sequenceIndex];
                clkOuput = clockwiseSequence[1][m_sequenceIndex];
            }
            else {
                dtOuput = counterClockwiseSequence[0][m_sequenceIndex];
                clkOuput = counterClockwiseSequence[1][m_sequenceIndex];
            }
            
            m_dt->setOut( dtOuput );
            m_dt->stampOutput();
            
            m_clk->setOut( clkOuput );
            m_clk->stampOutput();
            
            double stepsPerus = Simulator::self()->stepsPerus();
            m_outputCount = 100 * stepsPerus;
            
            m_sequenceIndex++;
            if( m_sequenceIndex >= 4 ) m_sequenceIndex = -1;
        }
    }
    
    if( m_changed )
    {
        m_sw->setOut( !m_closed );
        m_sw->stampOutput();
        
        m_changed = false;
    }
}

void KY040::remove()
{
    delete m_sw;
    delete m_clk;
    delete m_dt;
    Component::remove();
}

void KY040::paint( QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
    Component::paint( p, option, widget );
    
    p->setBrush(QColor( 50, 50, 70 ));
    p->drawRoundedRect( m_area, 2, 2 );
}

#include "moc_ky040.cpp"
