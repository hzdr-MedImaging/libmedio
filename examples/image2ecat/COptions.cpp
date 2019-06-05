// system includes
#include <iostream>
#include <getopt.h>
using namespace std;

// self includes
#include "COptions.h"

// medio incluides
#include <CMedIO.h>
#include <coreg.hh>

// qt includes
#include <QCoreApplication>

COptions::COptions()
{
  m_Force = false;
  m_Rgb = false;
  m_Verbose = false;

  m_xSize = 0;
  m_ySize = 0;
  m_zSize = 0;

  m_xDim = 0;
  m_yDim = 0;
  m_zDim = 0;

  m_Rescale = 0;
}

void COptions::usage()
{
  cout << endl;
  cout << "usage: image2ecat [options] file" << endl;
  cout << endl;
  cout << "\t -o: output file name (default input-basename.v)" << endl;
  cout << "\t -x: x extension of image (default read from image file)" << endl;
  cout << "\t -y: y extension of image (default read from image file)" << endl;
  cout << "\t -z: z extension of image (default none)" << endl;
  cout << "\t -X: z pixel size of image (default none)" << endl;
  cout << "\t -Y: y pixel size of image (default none)" << endl;
  cout << "\t -Z: z pixel size of image (default none)" << endl;
  cout << "\t -f: force conversion" << endl;
  cout << "\t -R: store rgb values (frame order: gray, red, green, blue)" << endl;
  cout << endl;
}

void COptions::read_options(int argc, char **argv)
{
  int opt = 0;

  while((opt = getopt(argc, argv, "o: f x:y:z: X:Y:Z: r: R h v V p: b") )!= -1)
  {
    switch(opt)
    { 
    case 'V':
      cout << qPrintable(QCoreApplication::applicationName()) << " v" << qPrintable(QCoreApplication::applicationVersion()) << endl;
      cout << "Compiled on " << __DATE__ << " " << __TIME__ << "." << endl;
      cout << "Using: ";
      cout << "Qt v" << qVersion() << " as shared library" << endl;
      cout << "       MedIO v" << qPrintable(CMedIO::version()) << " from " << qPrintable(CMedIO::buildDate()) << endl;
      cout << "       Coreg v" << qPrintable(CoRegister::version()) << " from " << qPrintable(CoRegister::buildDate()) << endl;
      exit(0);
      break;

    case 'p':
      m_Pixel = atoi(optarg);
      break;

    case 'b':
      m_Block = true;
      break;

    case 'f':
      m_Force = true;
      break;

    case 'v':
      m_Verbose = true;
      break;

    case 'o':
      m_OutputFile = optarg;
      break;

    case 'z':
      m_zDim = atof(optarg);
      break;

    case 'y':
      m_yDim = atof(optarg);
      break;

    case 'x':
      m_xDim = atof(optarg);
      break;

    case 'Z':
      m_zSize = atof(optarg);
      break;

    case 'Y':
      m_ySize = atof(optarg);
      break;

    case 'X':
      m_xSize = atof(optarg);
      break;

    case 'r':
      m_Rescale = atoi(optarg);
      break;

    case 'R':
      m_Rgb = true;
      break;

    case 'h':
      usage();
      exit(0);
      break;
    }
  }

  for(int i = optind; i < argc; ++i)
  {
    QString file(argv[i]);
    m_InputFileList.append(file);
  }
  
  if(m_InputFileList.count() > 0)
  {
    m_InputFile = m_InputFileList[0];
  }
  else
  {
    usage();
    cerr << "error: input file name missing!" << endl;
    exit(1);}

  if(m_InputFileList.count() > 1)
  {
    if(m_xSize <= 0 && m_xDim <= 0)
    {
      cerr << "error: positive image x extention or x pixel size necessary " << endl;
      exit(2);
    }

    if(m_ySize <= 0 && m_yDim <= 0)
    {
      cerr << "error: positive image y extention or y pixel size necessary " << endl;
      exit(2);
    }

    if(m_zSize <= 0 && m_zDim <= 0)
    {
      cerr << "error: positive image z extention or z pixel size necessary " << endl;
      exit(2);
    }
  }
}


