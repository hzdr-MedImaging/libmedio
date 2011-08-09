#include <QCoreApplication>

#include "testCPhilipsFile.h"

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);

  TestCPhilipsFile testCPhilipsFile;
  QTest::qExec(&testCPhilipsFile);

  return 0;
}
