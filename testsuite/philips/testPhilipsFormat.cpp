#include <QCoreApplication>

#include "testCPhilipsFile.h"
#include "testCPhilipsSubHeaderImage.h"

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);

  TestCPhilipsFile testCPhilipsFile;
  QTest::qExec(&testCPhilipsFile);

  TestCPhilipsSubHeaderImage testCPhilipsSubHeaderImage;
  QTest::qExec(&testCPhilipsSubHeaderImage);

  return 0;
}
