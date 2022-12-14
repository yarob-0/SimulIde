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

#include "ramtable.h"
#include "baseprocessor.h"
#include "basedebugger.h"
#include "mainwindow.h"
#include "utils.h"

RamTable::RamTable( QWidget* parent, BaseProcessor* processor )
        : QWidget( parent )
        , m_status( 1, 8 )
        , m_pc( 1, 2 )
{
    setupUi(this);

    m_processor = processor;
    m_debugger  = NULL;
    m_numRegs = 60;
    m_loadingVars = false;

    float scale = MainWindow::self()->fontScale();
    int row_heigh = 23*scale;
    int numberColor = 0x202090;

    QTableWidgetItem* it;
    QFont font;
    font.setFamily("Ubuntu Mono");
    font.setBold(true);
    font.setPixelSize( 14*scale );
    m_status.setFont(font);
    m_pc.setFont(font);

    m_status.setVerticalHeaderLabels( QStringList()<<" STATUS " );
    m_status.horizontalHeader()->hide();
    m_status.setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_status.setRowHeight( 0, row_heigh );
    m_status.setFixedHeight( row_heigh );
    font.setPixelSize( 12*scale );
    for( int i=0; i<8; i++ )
    {
        m_status.setColumnWidth( i, 22*scale );
        it = new QTableWidgetItem(0);
        it->setFlags( Qt::ItemIsEnabled );
        it->setFont( font );
        m_status.setItem( 0, i, it );
    }
    m_status.setMinimumWidth( (62+8*22)*scale );
    m_status.setMaximumWidth( (62+8*22)*scale );

    m_pc.setVerticalHeaderLabels( QStringList()<<" PC "  );
    m_pc.horizontalHeader()->hide();
    m_pc.setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_pc.setRowHeight( 0, row_heigh );
    m_pc.setFixedHeight( row_heigh  );

    font.setPixelSize( 14*scale );
    it = new QTableWidgetItem(0);
    it->setFlags( Qt::ItemIsEnabled );
    it->setFont( font );
    it->setTextColor( QColor( numberColor ) );
    m_pc.setItem( 0, 0, it );
    m_pc.setColumnWidth(0, 45*scale);

    it = new QTableWidgetItem(0);
    it->setFlags( Qt::ItemIsEnabled );
    it->setFont( font );
    it->setTextColor( QColor( 0x3030B8 ) );
    m_pc.setItem( 0, 1, it );
    m_pc.setColumnWidth(1, 80*scale);
    m_pc.setMinimumWidth( 150*scale );
    m_pc.setMaximumWidth( 150*scale );

    table->verticalHeader()->setSectionsMovable( true );
    table->setColumnWidth( 0, 60*scale );
    table->setColumnWidth( 1, 55*scale );
    table->setColumnWidth( 2, 35*scale );
    table->setColumnWidth( 3, 80*scale );

    for( int row=0; row<m_numRegs; row++ )
    {
        it = new QTableWidgetItem(0);
        it->setFont( font );
        it->setText( "---" );
        table->setVerticalHeaderItem( row, it );
        for( int col=0; col<4; col++ )
        {
            QTableWidgetItem* it = new QTableWidgetItem(0);
            if( col>0 ){
                it->setFlags( Qt::ItemIsEnabled );
                it->setText("---");
            }
            it->setFont( font );
            table->setItem( row, col, it );
        }
        table->setRowHeight(row, row_heigh);
    }
    table->setHorizontalHeaderLabels( QStringList()<<tr("Reg.")<<tr("Type")<<tr("Dec")<<tr("Value")  );

    setContextMenuPolicy( Qt::CustomContextMenu );

    registers->setFont( font );
    registers->setFixedWidth( 80*scale );
    registers->setEditTriggers( QAbstractItemView::NoEditTriggers );
    m_registerModel = new QStandardItemModel(this);
    registers->setModel( m_registerModel );

    connect( registers, SIGNAL(doubleClicked(QModelIndex)),
             this,      SLOT(RegDoubleClick(QModelIndex)));

    connect( this, SIGNAL(customContextMenuRequested(const QPoint&)),
             this, SLOT  (slotContextMenu(const QPoint&)), Qt::UniqueConnection);

    connect( table, SIGNAL(itemChanged(QTableWidgetItem*)  ),
             this, SLOT(addToWatch(QTableWidgetItem*)), Qt::UniqueConnection );
}

void RamTable::RegDoubleClick(const QModelIndex& index)
{
    m_currentRow = table->currentRow();
    if( m_currentRow < 0 ) return;

    setItemValue( 0, m_registerModel->item(index.row())->text() );
}

void RamTable::setStatusBits( QStringList statusBits )
{
    for( int i=0; i<8; i++ )
    {
        //qDebug()<<"RamTable::setStatusBits"<<i<<statusBits.at( i );
        m_status.item( 0, i )->setText( statusBits.at( i ));
    }
}

void RamTable::slotContextMenu( const QPoint& point )
{
    QMenu menu;
    if( m_debugger )
    {
        QAction *loadVars = menu.addAction( QIcon(":/open.png"),tr("Load Variables") );
        connect( loadVars, SIGNAL(triggered()), this, SLOT(loadVariables()), Qt::UniqueConnection );
    }

    QAction *clearSelected = menu.addAction( QIcon(":/remove.png"),tr("Clear Selected") );
    connect( clearSelected, SIGNAL(triggered()), this, SLOT(clearSelected()), Qt::UniqueConnection );

    QAction *clearTable = menu.addAction( QIcon(":/remove.png"),tr("Clear Table") );
    connect( clearTable, SIGNAL(triggered()), this, SLOT(clearTable()), Qt::UniqueConnection );

    menu.addSeparator();

    QAction *loadVarSet = menu.addAction( QIcon(":/open.png"),tr("Load VarSet") );
    connect( loadVarSet, SIGNAL(triggered()), this, SLOT(loadVarSet()), Qt::UniqueConnection );

    QAction *saveVarSet = menu.addAction( QIcon(":/save.png"),tr("Save VarSet") );
    connect( saveVarSet, SIGNAL(triggered()), this, SLOT(saveVarSet()), Qt::UniqueConnection );

    menu.exec( mapToGlobal(point) );
}

void RamTable::clearSelected()
{
    for( QTableWidgetItem* item : table->selectedItems() ) item->setData( 0, "");
}

void RamTable::clearTable()
{
    for( QTableWidgetItem* item : table->findItems( "*", Qt::MatchWildcard)  )
    { if( item ) item->setData( 0, "");}
    table->setCurrentCell( 0, 0 );
}

void RamTable::loadVariables()
{
    if( !m_debugger ) return;

    m_loadingVars = true;
    QStringList variables = m_debugger->getVarList();
    //qDebug() << "RamTable::loadVariables" << variables;

    loadVarSet( variables );
}

void RamTable::loadVarSet()
{
    const QString dir = m_processor->getFileName();

    QString fileName = QFileDialog::getOpenFileName( this, tr("Load VarSet"), dir, tr("VarSets (*.vst);;All files (*.*)"));

    if( !fileName.isEmpty() )
    {
        QStringList varSet = fileToStringList( fileName, "RamTable::loadVarSet" );
        if( !varSet.isEmpty() ) loadVarSet( varSet );
    }
}

void RamTable::loadVarSet( QStringList varSet )
{
    if( !m_loadingVars )
    {
        m_loadingVars = true;
        table->setCurrentCell( 0, 0 ); // Loading varset
    }

    int row = table->currentRow()-1;
    for( QString var : varSet )
    {
        row++;
        if( row >= m_numRegs ) break;
        table->item( row, 0 )->setText( var );
    }
    table->setCurrentCell( 0, 0 );

    m_loadingVars = false;
}

QStringList RamTable::getVarSet()
{
    QStringList varset;
    for( int row=0; row<m_numRegs; row++ )
    {
        QString name = table->item( row, 0 )->text();
        varset.append( name );
    }
    return varset;
}

void RamTable::saveVarSet()
{
    const QString dir = m_processor->getFileName();

    QString fileName = QFileDialog::getSaveFileName( this, tr("Save VarSet"), dir,
                                                 tr("VarSets (*.vst);;All files (*.*)"));
    if( !fileName.isEmpty() )
    {
        if( !fileName.endsWith(".vst") ) fileName.append(".vst");

        QFile file( fileName );

        if( !file.open(QFile::WriteOnly | QFile::Text) )
        {
              QMessageBox::warning(0l, "RamTable::saveVarSet",
              tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
              return;
        }
        file.flush();

        QTextStream out(&file);
        out.setCodec( "UTF-8" );
        QApplication::setOverrideCursor(Qt::WaitCursor);

        for( int row=0; row<m_numRegs; row++ )
        {
            QString name = table->item( row, 0 )->text();
            out << name << "\n";
        }
        file.close();
        QApplication::restoreOverrideCursor();
    }
}

void RamTable::updateValues()
{
    if( m_processor )
    {
        int status = m_processor->status();
        for( int i=0; i<8; i++ )
        {
            int bit = status & 1;
            if( bit ) m_status.item( 0, 7-i )->setBackground( QColor( 255, 150, 00 ) );
            else      m_status.item( 0, 7-i )->setBackground( QColor( 120, 230, 255 ) );
            status >>= 1;
        }

        int pc = m_processor->pc();
        m_pc.item( 0, 0 )->setData( 0, pc );
        m_pc.item( 0, 1 )->setText("  0x"+decToBase(pc, 16, 4).remove(0,1) );

        for( int _row: watchList.keys() )
        {
            m_currentRow = _row;
            QString name = watchList[_row];

            bool ok;
            int addr = name.toInt(&ok, 10);
            if( !ok ) addr = name.toInt(&ok, 16);
            if( !ok ) m_processor->updateRamValue( name );  // Var or Reg name
            else                                            // Address
            {
                int value = m_processor->getRamValue( addr );

                if( value >= 0 )
                {
                    table->item( _row, 1 )->setText("uint8");
                    table->item( _row, 2 )->setData( 0, value );
                    table->item( _row, 3 )->setData( 0, decToBase(value, 2, 8) );
                }
            }
        }
    }
}

void RamTable::setItemValue( int col, QString value  )
{
    table->item( m_currentRow, col )->setData( 0, value );
}

void RamTable::setItemValue( int col, float value  )
{
    table->item( m_currentRow, col )->setData( 0, value );
}

void RamTable::setItemValue( int col, int32_t value  )
{
    table->item( m_currentRow, col )->setData( 0, value );
}

void RamTable::addToWatch( QTableWidgetItem* it )
{
    if( table->column(it) != 0 ) return;
    int _row = table->row(it);
    table->setCurrentCell( _row, 0 );

    QString name = it->text().remove(" ").remove("\t").remove("*");//.toLower();

    if( name.isEmpty() )
    {
        watchList.remove(_row);
        table->verticalHeaderItem( _row )->setText("---");

        table->item( _row, 3 )->setText("---");
        table->item( _row, 2 )->setText("---");
        table->item( _row, 1 )->setText("---");
    }
    else
    {
        int value = m_processor->getRegAddress( name );
        if( value < 0 )
        {
            bool ok;
            value = name.toInt(&ok, 10);
            if( !ok ) value = name.toInt(&ok, 16);
            if( !ok ) value = -1;
        }
        if( value >= 0 )
        {
            watchList[_row] = name;
            table->verticalHeaderItem( _row )->setData( 0, value );
        }
        if( !m_debugger ) return;
        QString varType = m_debugger->getVarType( name );

        if( !m_loadingVars && varType.contains( "array" ) )
        {
            int size = varType.replace( "array", "" ).toInt();

            QStringList variables = m_debugger->getVarList();

            int indx = variables.indexOf( name );
            int listEnd = variables.size()-1;
            for( int i=1; i<size ; i++ )
            {
                int index = indx+i;
                if( index > listEnd ) break;

                QString varName = variables.at( index );
                if( varName.contains( name ) ) table->item( _row+i, 0 )->setText( varName );
            }
        }
    }
}

void RamTable::setRegisters()
{
    QStringList regs = m_processor->getRegList();
    regs.sort();
    for( QString reg : regs ) m_registerModel->appendRow( new QStandardItem(reg) );
}

void RamTable::setDebugger( BaseDebugger* deb )
{
    m_debugger = deb;
}

void RamTable::remDebugger( BaseDebugger* deb )
{
    if( m_debugger == deb ) m_debugger = NULL;
}

#include "moc_ramtable.cpp"


