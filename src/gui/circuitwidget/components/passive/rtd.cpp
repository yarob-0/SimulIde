/***************************************************************************
 *   Copyright (C) 2020 by Benoit ZERR                                       *
 *                                                      *
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
/*
 *   Modified 2020 by santiago González                                    *
 *   santigoro@gmail.com                                                   *
 *                                                                         */

#include "rtd.h"
#include "simulator.h"
#include "itemlibrary.h"

static const char* RTD_properties[] = {
    QT_TRANSLATE_NOOP("App::Property","R0")
};

Component* RTD::construct (QObject* parent, QString type, QString id)
{
  return new RTD (parent, type, id);
}

LibraryItem* RTD::libraryItem()
{
    return new LibraryItem(
        tr( "RTD" ),
        tr( "Resistive Sensors" ),
        "rtd.png",
        "RTD",
        RTD::construct );
}

RTD::RTD( QObject* parent, QString type, QString id )
   : ThermistorBase( parent, type, id )
{
    Q_UNUSED( RTD_properties );
}

RTD::~RTD() {}

void RTD::updateStep()
{
    if( !m_changed ) return;

    //m_t0_tau = (double) (Simulator::self()->step())/1e6;

    //uint64_t time = Simulator::self()->circTime();

    //double dt = (double)time/1e6 - m_t0_tau;
    //double dr = sensorFunction( m_temp )-m_resist;

    //double res = m_resist + dr*(1.0 -exp(-dt/m_tau));
    double res = sensorFunction( m_value*m_unitMult );
    eResistor::setResSafe( res );
   
    //m_lastTime = time;
    m_changed = false;
}

double RTD::sensorFunction( double temp )
{
  // linear approximation
  // double r_sense = r0*(1.0+coef_temp*sense); // linear approximation
  // polynomial approximation (more realistic)
  double r_sense = m_r0*(1.0+coef_temp_a*temp+coef_temp_b*temp*temp);
  if (temp < 0) r_sense += m_r0*coef_temp_c*(temp-100)*pow(temp,3.0);

  return r_sense;
}

void RTD::paint( QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    if( m_hidden ) return;

    Component::paint( p, option, widget );

    QFont font = p->font();
    font.setPixelSize(8);
    p->setFont( font );

    p->drawRect( -10.5, -4, 21, 8 );

    p->drawLine(-6, 6, 8,-8 );
    p->drawText(-8,-5, "+tº" );
}

#include "moc_rtd.cpp"
