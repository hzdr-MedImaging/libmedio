#include <QtTest/QtTest>

class CPhilipsFile;

class TestCPhilipsFile : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void testReadMainHeader();
    void testGetFileType();
    void testGetSubHeaderType();

    void testReadMatrixDataQByteArray();
    void testReadMatrixDataChar();
    void testReadMatrixDataQByteArrayAndSubHeader();
    void testReadMatrixDataCharAndSubHeader();
    void testCreateEmptyMainHeader();
    void cleanupTestCase();

  private:
    CPhilipsFile* m_pInputFile;
    // CPhilipsFile* m_pOutputFile;
};
