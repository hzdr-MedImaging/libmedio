#include <QtTest/QtTest>

class CPhilipsFile;

class TestCPhilipsSubHeaderImage : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void testLoad();
    void cleanupTestCase();

  private:
    CPhilipsFile* m_pInputFile;
    CPhilipsFile* m_pOutputFile;
};
