#ifndef CAPPLICATION_H
#define CAPPLICATION_H

#include <QString>
#include <QStringList>
#include <QFileInfo>

class CApplication
{
  public:
    CApplication();

    //methods
    bool parseCmdLine(int argc, char* argv[]);
    bool process();

  private:
    bool convertFile(const QFileInfo& inputFile);
    bool checkOutputFile(const QFileInfo& inputFile);
    void showUsage(int argc, char* argv[]);
    void showAppInfo();
    void showVersion();

  
    QStringList m_sInputFileNames;
    QString     m_sOutputFileName;
    QString     m_sPreferedOutputFile;
    QString     m_sPatientName;
    QString     m_sStartDirectory;
    bool        m_bOverWrite;
    bool        m_bPreserveDataType;
};

#endif // CAPPLICATION_H
