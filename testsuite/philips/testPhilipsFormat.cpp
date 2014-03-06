#include <QCoreApplication>

#include "testCPhilipsFile.h"
#include "testCPhilipsSubHeaderImage.h"
#include "testCPhilipsSubHeaderSinogram.h"

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);

  TestCPhilipsFile testCPhilipsFile(argv[1]);
  QTest::qExec(&testCPhilipsFile);

  TestCPhilipsSubHeaderImage testCPhilipsSubHeaderImage(argv[1]);
  QTest::qExec(&testCPhilipsSubHeaderImage);

  TestCPhilipsSubHeaderSinogram testCPhilipsSubHeaderSinogram(argv[1]);
  QTest::qExec(&testCPhilipsSubHeaderSinogram);

  return 0;
}
