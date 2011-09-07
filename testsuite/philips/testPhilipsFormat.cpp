#include <QCoreApplication>

#include "testCPhilipsFile.h"
#include "testCPhilipsSubHeaderImage.h"
#include "testCPhilipsSubHeaderSinogram.h"

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);

  TestCPhilipsFile testCPhilipsFile;
  QTest::qExec(&testCPhilipsFile);

  TestCPhilipsSubHeaderImage testCPhilipsSubHeaderImage;
  QTest::qExec(&testCPhilipsSubHeaderImage);

  // TestCPhilipsSubHeaderSinogram testCPhilipsSubHeaderSinogram;
  // QTest::qExec(&testCPhilipsSubHeaderSinogram);

  return 0;
}
