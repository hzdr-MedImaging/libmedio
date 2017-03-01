#include <CECAT7Assemble.h>

#include <CMedIO.h>

#include <QString>
#include <QMultiHash>
#include <QStringList>
#include <QFileInfo>

#include <rtdebug.h>

#include <iostream>

using namespace std;

QMultiHash<QString, QString> g_args;
QStringList g_inputFiles;
QString g_sInputFile;
QString g_sOutputFile;
bool g_bOverwrite = false;

bool parseCommandLine(int& argc, char** argv);
void showVersionInformation();
void showHelp(int& argc, char** argv);
bool assemble();


//  Function:    main
//!
//! 
//! 
//!
//! @param       argc number of commandline options
//! @param       *argv[] pointer to a array of the commandline options
//! @return      integer of the exit code.
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
  int iReturnCode = 0; // return no error on default
  bool bResult = false;
  // You want this, unless you mix couts output with C output.
  // Read  http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8 for an explanation.
  //ios::sync_with_stdio(false);

  #if defined(DEBUG)
  CRTDebug::init();
  #endif

  // we first parse our command line to check if there are no syntax errors
  if(parseCommandLine(argc, argv))
  {
    if(assemble())
      bResult = true;
  }

  #if defined(DEBUG)
  CRTDebug::destroy();
  #endif

  if(!bResult)
    iReturnCode = 1;

  return iReturnCode;
}

bool parseCommandLine(int& argc, char** argv)
{
  bool bResult = true;

  // if the user has specified some commandline options lets process and parse them.
  for(int i=1; i < argc; i++)
  {
    QString option(argv[i]);

    if(option[0] == '-')
    {
      if(i+1 < argc && ((argv[i+1][0] != '-') || (argv[i+1][0] == '-' && QChar(argv[i+1][1]).isDigit())))
      {
        g_args.insert(option, argv[i+1]);
        i++;
      }
      else
        g_args.insert(option, "");
    }
    else
    {
      g_inputFiles << argv[i];
    }
  }

  // if the user likes have help or versioning info we skip parameter processing
  if(g_args.contains("-h") || g_args.contains("-v"))
  {
    bResult = false;
  }
  else
  {
    // first we check if there is one and only one input file
    if(g_inputFiles.isEmpty())
    {
      cout << "ERROR: no input file specified." << endl;
      bResult = false;
    }
    else if(g_inputFiles.count() > 1)
    {
      cout << "ERROR: only one input file is allowed." << endl;
      bResult = false;
    }

    if(!g_inputFiles.isEmpty())
    {
      g_sInputFile = g_inputFiles.first();

      // check if inputfile is existing and readable
      QFileInfo fileInfo(g_sInputFile);
      if(!(fileInfo.exists() && fileInfo.isFile() && fileInfo.isReadable()))
      {
        cout << "ERROR: couldn't read inputfile '" << g_sInputFile.toLatin1().constData() << "'." << endl;
        bResult = false;
      }
    }

    if(g_args.contains("-f"))
    {
      g_bOverwrite = true;
    }

    if(g_args.contains("-o"))
    {
      if(!g_args.value("-o").isEmpty())
      {
        g_sOutputFile = g_args.value("-o");

        // check if outputfile is existing
        QFileInfo fileInfo(g_sOutputFile);
        if(fileInfo.exists())
        {
          if(g_bOverwrite)
          {
            // check if outputfile is a file and it is writeable
            if(!(fileInfo.isFile() && fileInfo.isWritable()))
            {
              cout << "ERROR: can't overwrite outputfile '" << g_sOutputFile.toLatin1().constData() << "'." << endl;
              bResult = false;
            }
          }
          else
          {
            cout << "ERROR: outputfile already exists '" << g_sOutputFile.toLatin1().constData() << "'." << endl;
            bResult = false;
          }
        }
      }
      else
      {
        cout << "ERROR: option \"-o\" needs a value." << endl;
        bResult = false;
      }
    }
    else
    {
      cout << "ERROR: no outputfile defined." << endl;
      bResult = false;
    }
  }

  if(!bResult)
  {
    if(g_args.contains("-v"))
      showVersionInformation();
    else
      showHelp(argc, argv);
  }
  return bResult;
}

void showVersionInformation()
{
  cout << "Detailed compilation information:" << endl << endl

       // Compiler information
       << "  "
       #if defined(__GNUC__)
       << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ <<  "." << __GNUC_PATCHLEVEL__ << " "
       #else
       #warning unknown compiler suite
       << "unknown compiler "
       #endif
       /*
       #if #cpu(sparc)
       << "[sparc]"
       #elif #cpu(sparc64)
       << "[sparc64]"
       #elif #cpu(powerpc)
       << "[ppc]"
       #elif #cpu(i386)
       << "[x86]"
       #elif #cpu(x86_64)
       << "[x86_64]"
       #else
       #warning Unknown CPU model
       << "[Unknown]"
       #endif
       */
       << endl << endl

       // Qt version information
       << "  Qt " << qVersion() << endl
                  << "  Copyright (c) 2006-2009 Trolltech Inc." << endl << endl

       << "  libmedio " << CMedIO::version().toLatin1().constData() <<  " ("
                        << CMedIO::buildDate().toLatin1().constData() << ")" << endl
                        << "  " << CMedIO::copyright().toLatin1().constData() << endl;
}

void showHelp(int& argc, char** argv)
{
  cout << endl;
  cout << "libmedio ECAT6/7 assembleBeds utility v1.2" << endl;
  cout << "----------------------------------" << endl;
  if(argc > 0)
  {
    cout << "Usage: " << argv[0] << " <options> ecatfile" << endl;
    cout << "Options:" << endl;
    cout << "  -o            : set outputfile." << endl;
    cout << "  -f            : force overwrite of outputfile." << endl;
    cout << "  -h            : this help page." << endl << endl;
  }
  else
  {
    cout << " input parameters screwed up." << endl << endl;
  }
}

bool assemble()
{
  bool bResult = true;

  CECAT7Assemble assemble;
  assemble.setInputFile(g_sInputFile);
  assemble.setOutputFile(g_sOutputFile);

  bResult = assemble.assemble();

  return bResult;
}
