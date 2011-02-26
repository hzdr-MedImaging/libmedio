#ifndef CAPPLICATION_H
#define CAPPLICATION_H

#include <QString>
#include <QStringList>
#include <QDir>

class CApplication
{
  public:
    //constructors
    CApplication();

    //methods
    bool parseCmdLine(int argc, char* argv[]);
    bool process();

  protected:
    bool convertFile(const QFileInfo& inputFile);
    bool walkDirectory(const QDir& dir);

  private:
    // methods
    void showUsage(int argc, char* argv[]);
    void showAppInfo();
    void showVersion();
    bool checkOutputFile(const QFileInfo& inputFile);

    // members
    QStringList m_sInputFileNames;
    QString     m_sOutputFileName;
    QString     m_sPreferedOutputFile;
    QString     m_sPatientName;
    QString     m_sStartDirectory;
    bool        m_bOverWrite;
    bool        m_bPreserveDataType;
    bool        m_bRecursive;
};

#endif // CAPPLICATION_H
