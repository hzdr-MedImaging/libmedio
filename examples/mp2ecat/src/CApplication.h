#ifndef CAPPLICATION_H
#define CAPPLICATION_H

#include <QString>

class CApplication
{
  public:
    //constructors
    CApplication();

    //methods
    bool parseCmdLine(int argc, char* argv[]);
    bool convertFile();

  private:
    // methods
    void showUsage(int argc, char* argv[]);
    void showAppInfo();
    void showVersion();
    bool convIntArrayToShort();
    bool checkOutputFile(QString sFileName);
    bool checkOutputDir(QString sDirectory);

    // members
    QString m_sInputFileName;
    QString m_sOutputFileName;
    QString m_sPatientName;
    bool    m_bOverWrite;
    bool    m_bPreserveDataType;
};

#endif // CAPPLICATION_H
