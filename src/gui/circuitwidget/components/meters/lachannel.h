/***************************************************************************
 *   Copyright (C) 2020 by santiago González                               *
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

#ifndef LACHANNEL_H
#define LACHANNEL_H

#include "datachannel.h"
#include "logicanalizer.h"


class MAINMODULE_EXPORT LaChannel : public DataChannel
{
        friend class LAnalizer;

    public:

        LaChannel( LAnalizer* la, QString id );
        ~LaChannel();

        virtual void resetState() override;
        virtual void updateStep() override;
        virtual void setVChanged() override;

    private:
        double m_lastValue;

        uint64_t m_risEdge;

        bool m_rising;
        bool m_falling;

        paCond m_chCond;
        bool   m_chCondFlag;

        LAnalizer* m_analizer;
};

#endif

