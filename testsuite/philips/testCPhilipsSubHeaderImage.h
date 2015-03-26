#include <QtTest/QtTest>

class CPhilipsFile;

class TestCPhilipsSubHeaderImage : public QObject
{
  Q_OBJECT

  public:
    TestCPhilipsSubHeaderImage(const QString& file)
      : m_sFileName(file) { }

  private slots:
    void initTestCase();
    void testLoad();
    void cleanupTestCase();

  private:
    QString m_sFileName;
    CPhilipsFile* m_pInputFile;
    // CPhilipsFile* m_pOutputFile;
};
