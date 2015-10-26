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
    bool convert2Ecat(const QFileInfo& inputFile);
    bool convert2Img(const QFileInfo& inputFile);
    bool walkDirectory(const QDir& dir);
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
    bool        m_bRecursive;
    bool        m_bEcat2Img;
};

#endif // CAPPLICATION_H
