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

#include "inodebugger.h"
#include "baseprocessor.h"
#include "circuit.h"
#include "utils.h"
#include "simuapi_apppath.h"

static const char* InoDebugger_properties[] = {
    QT_TRANSLATE_NOOP("App::Property","Board"),
    QT_TRANSLATE_NOOP("App::Property","Custom Board")
};

InoDebugger::InoDebugger( QObject* parent, OutPanelText* outPane, QString filePath ) 
           : BaseDebugger( parent, outPane, filePath )
{
    Q_UNUSED( InoDebugger_properties );
    
    setObjectName( "Arduino Compiler/Debugger" );
    
    m_compilerPath = "";
    m_compSetting = "arduino_Path";
    
    readSettings();
    
    m_boardList << "uno" << "megaADK" << "nano" << "diecimila" << "leonardo";
    m_board = Uno;
    
    m_typesList["char"]   = "int8";
    m_typesList["uchar"]  = "uint8";
    m_typesList["int"]    = "int16";
    m_typesList["uint"]   = "uint16";
    m_typesList["short"]  = "int16";
    m_typesList["ushort"] = "uint16";
    m_typesList["long"]   = "int32";
    m_typesList["ulong"]  = "uint32";
    m_typesList["float"]  = "float32";
}
InoDebugger::~InoDebugger() {}

void InoDebugger::upload()
{
    QString circDir = Circuit::self()->getFileName();
    QString firmPath =  m_firmware;
    
    if( circDir != "" )
    {
        QDir circuitDir = QFileInfo( circDir ).absoluteDir();
        m_firmware = circuitDir.absolutePath()+"/"+m_fileName+".hex";
        circuitDir.remove( m_fileName+".hex" );
        QFile::copy( firmPath, m_firmware );
    }
    BaseDebugger::upload();
    m_firmware = firmPath;
}

int InoDebugger::compile()
{
    QString builder = "arduino-builder";
    #ifndef Q_OS_UNIX
    builder += ".exe";
    #endif

    if( !QFile::exists( m_compilerPath+builder) )
    {
        m_outPane->appendText( "\nArduino" );
        toolChainNotFound();
        return -1;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QString filePath = m_file;
    QString buildPath = SIMUAPI_AppPath::self()->RWDataFolder().absoluteFilePath("codeeditor/buildIno");

    QDir dir(buildPath);
    bool b = dir.cd( "build" );
    if( b ) dir.removeRecursively(); // Remove old files
    dir.mkpath(buildPath+"/cache");  // Create cache folder ( if doesn't exist )
    dir.mkpath(buildPath+"/build");  // Create build folder

    if( !QFile::exists(buildPath+"/build") || !QFile::exists(buildPath+"/cache") )
        m_outPane->writeText( "\n    ERROR: Build folders NOT found at:\n    "+buildPath+"\n" );
    else m_outPane->writeText( "\nFound Build folders at:\n"+buildPath+"\n" );

    //QDir directory( m_fileDir );
    //m_fileList = directory.entryList( QDir::Files );
    m_fileList.clear();
    m_fileList.append( m_fileName+m_fileExt );

    QStringList inoLines = fileToStringList( filePath, "InoDebugger::compile" );
    QString line;

    m_varList.clear();
    for( QString inoLine : inoLines )                        // Get Variables
    {
        line = inoLine;
        line = line.replace( "\t", " " ).remove(";");
        QStringList wordList= line.split( " " );
        wordList.removeAll( "" );
        if( !wordList.isEmpty())
        {
            QString type = wordList.takeFirst();
            if( type == "unsigned" ) type = "u"+wordList.takeFirst();

            if( m_typesList.contains( type ) && !wordList.isEmpty() )
            {
                QString varName = wordList.at(0);
                if( !m_varList.contains( varName ) )
                    m_varList[ varName ] = m_typesList[ type ];
                //qDebug() << "InoDebugger::compile  variable "<<type<<varName<<m_typesList[ type ];
    }   }   }
    QString command  = m_compilerPath+"arduino";

    if( m_sketchBook.isEmpty() )
    {
        #ifndef Q_OS_UNIX
        command += "_debug";
        #endif
        command = addQuotes( command );
        command += " --get-pref sketchbook.path";

        QProcess getSkBook( this );  // Get sketchBook Path
        getSkBook.start( command );
        getSkBook.waitForFinished();
        m_sketchBook = getSkBook.readAllStandardOutput();
        m_sketchBook = m_sketchBook.remove("\r").remove("\n");
        getSkBook.close();
        if( m_sketchBook.isEmpty() )
            m_outPane->writeText( "\nNo User sketchBook Found\n\n" );
        else
            m_outPane->writeText( "\nFound User sketchBook at:\n"+m_sketchBook+"\n\n" );
    }
    filePath           = addQuotes( filePath );
    command            = addQuotes( m_compilerPath+"arduino-builder" );
    QString hardware   = addQuotes( m_compilerPath+"hardware" );
    QString toolsBuild = addQuotes( m_compilerPath+"tools-builder" );
    QString toolsAvr   = addQuotes( m_compilerPath+"hardware/tools/avr" );
    QString libraries  = addQuotes( m_compilerPath+"libraries" );
    QString userLibrar = addQuotes( m_sketchBook+"/libraries" );
    QString cBuildPath = addQuotes( buildPath+"/build" );
    QString cCachePath = addQuotes( buildPath+"/cache" );
    QString boardName;
    if( m_board < Custom ) boardName = "arduino:avr:"+m_boardList.at( m_board );
    else                   boardName = m_customBoard;

    command += " -compile";
    command += " -hardware "+hardware;
    command += " -tools "+toolsBuild;
    command += " -tools "+toolsAvr;
    command += " -built-in-libraries "+libraries;
    command += " -libraries "+userLibrar;
    command += " -fqbn="+boardName;
    command += " -build-path "+cBuildPath;
    command += " -build-cache "+cCachePath;
    command += " "+filePath;
    m_firmware = "";

    m_outPane->writeText( command+"\n\n" );

    m_compProcess.start( command );
    m_compProcess.waitForFinished(-1);

    QString p_stderr = m_compProcess.readAllStandardError();

    int error = -1;
    if( !p_stderr.isEmpty() )
    {
        m_outPane->writeText( p_stderr );
        QStringList lines = p_stderr.split("\n");
        for( QString line : lines )
        {
            if( line.contains( "error:" ) )
            {
                QStringList words = line.remove(":\\").split(":");
                error = words.at(1).toInt();
                break;
    }   }   }
    else{
        m_firmware = buildPath+"/build/"+m_fileName+".ino.hex";
        error = 0;
    }
    QApplication::restoreOverrideCursor();
    return error;
}

void InoDebugger::getVariables()
{
    QString buildPath = SIMUAPI_AppPath::self()->RWDataFolder().absoluteFilePath("codeeditor/buildIno");

    QString objdump = m_compilerPath+"hardware/tools/avr/bin/avr-objdump";
    QString elfPath = buildPath+"/build/"+m_fileName+".ino.elf";

    objdump = addQuotes( objdump );
    elfPath = addQuotes( elfPath );

    QProcess getBss( 0l );      // Get var addresses from .bss section
    QString command  = objdump+" -t -j.bss "+elfPath;
    getBss.start( command );
    getBss.waitForFinished(-1);

    QString  p_stdout = getBss.readAllStandardOutput();
    QStringList varNames = m_varList.keys();
    m_varNames.clear();

    for( QString line : p_stdout.split("\n") )
    {
        QStringList words = line.split(" ");
        if( words.size() < 4 ) continue;
        QString addr   = words.takeFirst();
        QString symbol = words.takeLast();

        if( varNames.contains( symbol ) )
        {
            bool ok = false;
            int address = addr.toInt( &ok, 16 );
            if( !ok ) continue;
            address -= 0x800000;          // 0x800000 offset
            BaseProcessor* proc = BaseProcessor::self();
            if( proc ) proc->addWatchVar( symbol, address, m_varList.value( symbol ) );
            m_varNames.append( symbol );
            //qDebug() << "InoDebugger::compile  variable "<<addr<<varName<<address<<i.value();
}   }   }

void InoDebugger::mapFlashToSource()
{
    getVariables();
    m_flashToSource.clear();
    m_sourceToFlash.clear();

    QString buildPath = SIMUAPI_AppPath::self()->RWDataFolder().absoluteFilePath("codeeditor/buildIno");
    QString elfPath = buildPath+"/build/"+m_fileName+".ino.elf";
    QString avrSize = m_compilerPath+"hardware/tools/avr/bin/avr-size";
    QString addr2li = m_compilerPath+"hardware/tools/avr/bin/avr-addr2line";

    avrSize = addQuotes( avrSize );
    addr2li = addQuotes( addr2li );
    elfPath = addQuotes( elfPath );

    QProcess getSize( this );  // Get Firmware size
    getSize.start( avrSize+" "+elfPath );
    getSize.waitForFinished();
    QString lines = getSize.readAllStandardOutput();
    getSize.close();
    bool ok = false;
    int flashSize;
    if( !lines.isEmpty() )
    {
        QString size = lines.split("\n").at(1).split("\t").takeFirst().remove(" ");
        flashSize = size.toInt( &ok );
    }
    if( !ok ) flashSize = 35000;

    QProcess flashToLine( this );
    flashToLine.start( addr2li+" -e "+elfPath );
    bool started = flashToLine.waitForStarted( 1000 );
    if( !started ) return;

    for( int flashAddr=0; flashAddr<flashSize; ++flashAddr ) // Map Flash Address to Source Line
    {
        QString addr = val2hex( flashAddr )+"\n";
        flashToLine.write( addr.toUtf8() );
    }
    flashToLine.closeWriteChannel();
    flashToLine.waitForFinished();

    for( int flashAddr=0; flashAddr<flashSize; ++flashAddr ) // Map Flash Address to Source Line
    {
        QString p_stdout = flashToLine.readLine();
        if( p_stdout.isEmpty() ) continue;
        if( p_stdout.startsWith("?") ) continue;

        int idx = p_stdout.lastIndexOf( ":" );
        if( idx == -1 ) continue;

        QString fileName = QFileInfo( p_stdout.left( idx ) ).fileName();
        if( m_fileList.contains( fileName ) )
        {
            bool ok = false;
            int inoLineNum = p_stdout.mid( idx+1 ).toInt( &ok );
            if( !ok ) continue;
            int addr = flashAddr/2;
            if( !m_sourceToFlash.contains( inoLineNum ) )
            {
                if( inoLineNum > m_lastLine ) m_lastLine = inoLineNum;
                m_flashToSource[ addr ] = inoLineNum;
                m_sourceToFlash[ inoLineNum ] = addr;
            }
    }   }
    flashToLine.close();
}

#include "moc_inodebugger.cpp"
