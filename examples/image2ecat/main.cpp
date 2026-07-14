/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2026 hzdr.de and contributors
 
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
    http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <iostream>
#include <fstream>

using namespace std;

// Qt includes
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QImage>
#include <QColor>
#include <QImageReader>
#include <QCoreApplication>

// medio includes
#include <CECATFile>
#include <CECAT7SubHeaderImage>
#include <CECAT7MainHeader>
#include <CECATMainHeader>

// coreg includes
#include <basics.hh>
#include <dataset.hh>

#include "COptions.h"
COptions options;

void buildSubHeader(CECAT7SubHeaderImage * sub, DataSet *data)
{
  sub->setX_Dimension(data->Nx); 
  sub->setY_Dimension(data->Ny);
  sub->setZ_Dimension(data->Nz); 

  sub->setX_Pixel_Size(data->xsize);
  sub->setY_Pixel_Size(data->ysize);
  sub->setZ_Pixel_Size(data->zsize);

  sub->setData_Type(CECATSubHeader::SunShort);
}

void save_matrix(CECATFile *ecat_file, CECAT7SubHeaderImage *sub, DataSet *data, int frame)
{
  short int* matrixdata = new short int [data->Nz * data->Ny * data->Nx];
  unsigned size = data->Nx * data->Ny * data->Nz * sizeof(short);
  float scale_factor = 1; //data->calc_max_activity()/32767;

  int count = 0;
  for (int z = 1; z <= data->Nz; z++)
  {
    for (int y = 1; y <= data->Ny; y++)
    {
      for (int x = 1; x <= data->Nx; x++)
      {
        matrixdata[count] = (short) nint(data->activity[z][y][x] / scale_factor);
        count++;
      }
    }
  }

  CECAT7SubHeaderImage* sub_out = static_cast<CECAT7SubHeaderImage*>(ecat_file->createEmptySubHeader());
  (*sub_out) = (*sub);

  sub_out->setX_Dimension(data->Nx); 
  sub_out->setY_Dimension(data->Ny);
  sub_out->setZ_Dimension(data->Nz); 

  sub_out->setX_Pixel_Size(data->xsize);
  sub_out->setY_Pixel_Size(data->ysize);
  sub_out->setZ_Pixel_Size(data->zsize);

  sub_out->setScale_Factor(scale_factor);

  short max = (short) nint(data->calc_max_activity() / scale_factor);
  short min = (short) nint(data->calc_min_activity() / scale_factor);
  sub_out->setImage_Max(max);
  sub_out->setImage_Min(min);
  sub_out->setScale_Factor(scale_factor);

  ecat_file->writeMatrix((char*)matrixdata, size, CECATSubHeader::SunShort, frame, 1, 1);
  ecat_file->writeSubHeader(*sub_out, frame, 1, 1);

  delete [] matrixdata;
  delete sub_out;
}


void setDataSet(DataSet *data, QImage &image, int which, int slice)
{
  QRgb rgb;
  for(int y = 1; y <= data->Ny; y++)
  {
    for(int x = 1; x <= data->Nx; x++)
    {
      if(x-1 >= image.width()) continue;
      if(y-1 >= image.height()) continue;
      
      rgb = image.pixel(x-1,y-1);
      switch(which){
      case 0:
        //data->activity[1][y][x] = (float)qAlpha(rgb);
        data->activity[slice][y][x] = 255-(float)qGray(rgb);
        break;
      case 1:
        data->activity[slice][y][x] = (float)qRed(rgb);
        break;
      case 2:
        data->activity[slice][y][x] = (float)qGreen(rgb);
        break;
      case 3:
        data->activity[slice][y][x] = (float)qBlue(rgb);
        break;
      }
    }
  }
}

void setDataSetBlocked(DataSet *data, QImage &image)
{
  QRgb rgb;
  int gray;
  int total;
  int count;
  int bX =  nint((float)image.width() / data->Nx);
  int bY =  nint((float)image.height() / data->Ny);
  int xIndex;
  int yIndex;

  cout << bX << " " << bY << " " << image.width() << " " << image.height() << endl;

  for(int y = 1; y <= data->Ny; y++)
    for(int x = 1; x <= data->Nx; x++){

      total = 0;
      count = 0;

      for(int i = 0; i < bX; i++)
        for(int j = 0; j < bY; j++){
	  
          xIndex = (x-1) * bX + i;
          yIndex = (y-1) * bY + j;

          if(xIndex >= image.width()) continue;
          if(yIndex >= image.height()) continue;

          rgb = image.pixel(xIndex, yIndex);
          gray = qGray(rgb);

          total++;

          if(gray > 0)
            count++;

        }
      //cout << total << " " << count << " " << (float)count / total << endl;
      //data->activity[1][bx(y, data->Ny)][bx(x, data->Nx)] = 255 - 255 * (float)count / total;
      data->activity[1][y][x] = bX * bY * (float)count / total;
    }
}


void scaleImage(QImage &image)
{
  int maxsize = options.rescale();
  QImage scaledImage = image;

  if(image.width() > maxsize || image.height() > maxsize){

    float xSize = (float)image.width() / image.dotsPerMeterX();
    float ySize = (float)image.height() / image.dotsPerMeterY();

    int width;
    int height;

    if(image.width() > image.height()){

      height = maxsize;
      width = nint(maxsize * (float)image.width() / image.height());
    }
    else{

      width = maxsize;
      height = nint(maxsize * (float)image.height() / image.width());
    }

    scaledImage = image.scaled(width, height, Qt::IgnoreAspectRatio, Qt::FastTransformation);

    scaledImage.setDotsPerMeterX(nint(scaledImage.width() / xSize));
    scaledImage.setDotsPerMeterY(nint(scaledImage.height() / ySize));
  }

  image = scaledImage;
}


void adjustScale(DataSet *data)
{
  float max = data->calc_max_activity();
  float min = data->calc_min_activity();
  float value;
  max -= min;

  for(int z = 1; z <= data->Nz; z++)
    for(int y = 1; y <= data->Ny; y++)
      for(int x = 1; x <= data->Nx; x++)
        data->activity[z][y][x] -= min;


  for(int z = 1; z <= data->Nz; z++)
    for(int y = 1; y <= data->Ny; y++)
      for(int x = 1; x <= data->Nx; x++){

        value = data->activity[z][y][x];
        data->activity[z][y][x] = (max - value);
      }

  data->ajust_scale(32000.0 / data->calc_max_activity());
}


void convert(const QString & imageFile, const QString & ecatFile)
{
  QImage image;
  bool ok = image.load(imageFile);

  if(!ok)
  {
    cerr << "error: cannot open image file " << qPrintable(imageFile) << endl;
    exit(2);
  }

  if(options.xSize() != 0)
  {
    options.setXDim(options.xSize() * image.width());
  }
  if(options.ySize() != 0)
  {
    options.setYDim(options.ySize() * image.height());
  }

  if(options.xDim() != 0)
  {
    image.setDotsPerMeterX(nint(100 * image.width() / options.xDim()));
    if(options.yDim() == 0)
    {
      image.setDotsPerMeterY(image.dotsPerMeterX());
    }
  }

  if(options.yDim() != 0)
  {
    image.setDotsPerMeterY(nint(100 * image.height() / options.yDim()));
    if(options.xDim() == 0)
    {
      image.setDotsPerMeterX(image.dotsPerMeterY());
    }
  }

  if(!image.dotsPerMeterX())
  { 
    if(options.force())
    {
      image.setDotsPerMeterX(100);
      cerr << "warning: cannot read dots per meter (x), using 100" << endl;
    }
    else
    {
      cerr << "error: cannot read dots per meter (x)" << endl;
      exit(3);
    }
  }

  if(!image.dotsPerMeterY())
  {
    if(options.force())
    {
      image.setDotsPerMeterY(100);
      cerr << "warning: cannot read dots per meter (y), using 100" << endl;
    }
    else{
      cerr << "error: cannot read dots per meter (x)" << endl;
      exit(3);
    }
  }

  if(options.rescale() > 100)
    scaleImage(image);

  DataSet *data = new DataSet;

  data->Nx = image.width();
  data->Ny = image.height();
  data->Nz = 1;

  data->xsize = 100.0 / (image.dotsPerMeterX());
  data->ysize = 100.0 / (image.dotsPerMeterY());
  
  if(options.zSize() != 0)
    data->zsize = options.zSize();
  else
    data->zsize = 1;

  data->scale = 1;
  
  data->allocate_activity_memory();
  data->set_to_zero_special();

  if(options.verbose())
  {
    cout << "dots per meter\t" << image.dotsPerMeterX() << " " << image.dotsPerMeterY() << endl;
    cout << "size of image\t" << data->Nx * data->xsize << " " << data->Ny * data->ysize << endl;
  }

  CECATFile *ecat_file = new CECATFile(ecatFile, CECATMainHeader::ECAT7_Volume16);
  ecat_file->open(QIODevice::WriteOnly);

  CECAT7MainHeader *mh = static_cast<CECAT7MainHeader*>(ecat_file->createEmptyMainHeader());

  mh->setPlane_Separation(1);
  mh->setCalibration_Factor(1);
  mh->setNum_Planes(1);
  mh->setStudy_Description("convert image");
  mh->setPatient_Name("converted image");

  if(options.rgb())
    mh->setNum_Frames(4);
  else
    mh->setNum_Frames(1);

  ecat_file->writeMainHeader(*mh);

  CECAT7SubHeaderImage* sub = static_cast<CECAT7SubHeaderImage*>(ecat_file->createEmptySubHeader());
  buildSubHeader(sub, data);

  setDataSet(data, image, 0, 1);
  save_matrix(ecat_file, sub, data, 1);

  if(options.rgb()){
    setDataSet(data, image, 1, 1);
    save_matrix(ecat_file, sub, data, 2);

    setDataSet(data, image, 2, 1);
    save_matrix(ecat_file, sub, data, 3);

    setDataSet(data, image, 3, 1);
    save_matrix(ecat_file, sub, data, 4);
  }

  ecat_file->close();

  data->delete_activity_memory();
  delete data;

  delete mh;
  delete sub;
  delete ecat_file;
}

void convert(const QStringList & imageList, const QString & ecatFile)
{
  bool ok = false;
  QList<QImage> iList;
  DataSet *data = new DataSet;

  for(int i = 0; i < imageList.count(); ++i)
  {
    QImage image;
    QString imageFile = imageList[i];
    cout << "processing file: " << qPrintable(imageFile) << endl;

    ok = image.load(imageFile);

    if(!ok)
    {
      cerr << "error: cannot open image file " << qPrintable(imageFile) << endl;
      exit(2);
    }

    if(i == 0)
    {
      data->Nx = image.width();
      data->Ny = image.height();
      data->Nz = imageList.count();

      if(options.xDim() != 0)
        data->xsize = options.xDim() / (image.width());
      else
        data->xsize = options.xSize();

      if(options.yDim() != 0)
        data->ysize = options.yDim() / (image.height());
      else
        data->ysize = options.ySize();

      if(options.zDim() != 0)
        data->zsize = options.zDim() / (imageList.count());
      else
        data->zsize = options.zSize();

      data->scale = 1;
  
      data->allocate_activity_memory();
      data->set_to_zero_special();
    }
    else
    {
      if(image.width() != iList[0].width() || image.height() != iList[0].height()){
        cerr << "error: images must have the same width and height" << endl;
        exit(2);
      }
    }

    if(options.rescale() > 100)
      scaleImage(image);

    iList.append(image);
  }

  CECATFile *ecat_file = new CECATFile(ecatFile, CECATMainHeader::ECAT7_Volume16);
  ecat_file->open(QIODevice::WriteOnly);

  CECAT7MainHeader *mh = static_cast<CECAT7MainHeader*>(ecat_file->createEmptyMainHeader());

  mh->setPlane_Separation(data->zsize);
  mh->setCalibration_Factor(1);
  mh->setNum_Planes(data->Nz);
  mh->setStudy_Description("convert image");
  mh->setPatient_Name("converted image");

  if(options.rgb())
    mh->setNum_Frames(4);
  else
    mh->setNum_Frames(1);

  ecat_file->writeMainHeader(*mh);

  CECAT7SubHeaderImage* sub = static_cast<CECAT7SubHeaderImage*>(ecat_file->createEmptySubHeader());
  buildSubHeader(sub, data);

  for(int i = 0; i < iList.count(); ++i)
    setDataSet(data, iList[i], 0, i+1);
  //float max = data->calc_max_activity();
  //data->ajust_scale(32000.0 / max);
  adjustScale(data);
  cout << "writing gray scale frame" << endl;
  save_matrix(ecat_file, sub, data, 1);

  if(options.rgb()){
    for(int i = 0; i < iList.count(); ++i)
      setDataSet(data, iList[i], 1, i+1);
    //max = data->calc_max_activity();
    //data->ajust_scale(32000.0 / max);
    cout << "writing red frame" << endl;
    save_matrix(ecat_file, sub, data, 2);

    for(int i = 0; i < iList.count(); ++i)
      setDataSet(data, iList[i], 2, i+1);
    //max = data->calc_max_activity();
    //data->ajust_scale(32000.0 / max);
    cout << "writing green frame" << endl;
    save_matrix(ecat_file, sub, data, 3);

    for(int i = 0; i < iList.count(); ++i)
      setDataSet(data, iList[i], 3, i+1);
    //max = data->calc_max_activity();
    //data->ajust_scale(32000.0 / max);
    cout << "writing blue frame" << endl;
    save_matrix(ecat_file, sub, data, 4);
  }

  ecat_file->close();

  data->delete_activity_memory();
  delete data;

  delete mh;
  delete sub;
  delete ecat_file;
}


void convertBlocked(const QString & imageFile, QString & ecatFile)
{
  QImage image;

  bool ok = image.load(imageFile);

  if(!ok)
  {
    cerr << "error: cannot open image file " << qPrintable(imageFile) << endl;
    exit(2);
  }

  if(options.xDim() != 0)
  {
    image.setDotsPerMeterX(nint(100 * image.width() / options.xDim()));
    if(options.yDim() == 0)
    {
      image.setDotsPerMeterY(image.dotsPerMeterX());
    }
  }

  if(options.yDim() != 0)
  {
    image.setDotsPerMeterY(nint(100 * image.height() / options.yDim()));
    if(options.xDim() == 0)
    {
      image.setDotsPerMeterX(image.dotsPerMeterY());
    }
  }

  if(!image.dotsPerMeterX())
  { 
    if(options.force())
    {
      image.setDotsPerMeterX(100);
      cerr << "warning: cannot read dots per meter (x), using 100" << endl;
    }
    else
    {
      cerr << "error: cannot read dots per meter (x)" << endl;
      exit(3);
    }
  }

  if(!image.dotsPerMeterY())
  {
    if(options.force())
    {
      image.setDotsPerMeterY(100);
      cerr << "warning: cannot read dots per meter (y), using 100" << endl;
    }
    else
    {
      cerr << "error: cannot read dots per meter (x)" << endl;
      exit(3);
    }
  }

  if(options.rescale() > 100)
    scaleImage(image);

  DataSet *data = new DataSet;

  int Ny = options.pixel();
  int Nx = Ny * image.width() / image.height();

  float blockX =  (float)Nx / image.width();
  float blockY =  (float)Ny / image.height();

  data->Nx = Nx;
  data->Ny = Ny;
  data->Nz = 1;

  data->xsize = (100.0 / (image.dotsPerMeterX())) / blockX;
  data->ysize = (100.0 / (image.dotsPerMeterY())) / blockY;
  
  data->zsize = 1;
  data->scale = 1;
  
  data->allocate_activity_memory();
  data->set_to_zero_special();

  if(options.verbose())
  {
    cout << "dots per meter\t" << image.dotsPerMeterX() << " " << image.dotsPerMeterY() << endl;
    cout << "size of image\t" << data->Nx * data->xsize << " " << data->Ny * data->ysize << endl;
  }

  CECATFile *ecat_file = new CECATFile(ecatFile, CECATMainHeader::ECAT7_Volume16);
  ecat_file->open(QIODevice::WriteOnly);

  CECAT7MainHeader *mh = static_cast<CECAT7MainHeader*>(ecat_file->createEmptyMainHeader());

  mh->setPlane_Separation(1);
  mh->setCalibration_Factor(1);
  mh->setNum_Planes(1);
  mh->setStudy_Description("convert image");
  mh->setPatient_Name("converted image");

  if(options.rgb())
    mh->setNum_Frames(4);
  else
    mh->setNum_Frames(1);

  ecat_file->writeMainHeader(*mh);

  CECAT7SubHeaderImage* sub = static_cast<CECAT7SubHeaderImage*>(ecat_file->createEmptySubHeader());
  buildSubHeader(sub, data);

  setDataSetBlocked(data, image);
  save_matrix(ecat_file, sub, data, 1);

  if(options.rgb()){
    setDataSet(data, image, 1, 1);
    save_matrix(ecat_file, sub, data, 2);

    setDataSet(data, image, 2, 1);
    save_matrix(ecat_file, sub, data, 3);

    setDataSet(data, image, 3, 1);
    save_matrix(ecat_file, sub, data, 4);
  }

  ecat_file->close();

  data->delete_activity_memory();
  delete data;

  delete mh;
  delete sub;
  delete ecat_file;
}


int main(int argc, char **argv)
{
  QCoreApplication::setOrganizationName("hzdr");
  QCoreApplication::setOrganizationDomain("hzdr.de");
  QCoreApplication::setApplicationName("image2ecat");
  QCoreApplication::setApplicationVersion("1.0.2");

  options.read_options(argc, argv);

  QString imageFile = options.inputFile();
  QString ecatFile  = options.outputFile();
  QStringList fileList = options.inputFileList();

  if(ecatFile.isEmpty())
  { 
    ecatFile = QFileInfo(imageFile).baseName() + ".v";
  }

  if(fileList.count() > 1)
  {
    convert(fileList, ecatFile);
  }
  else
  {
    if(!QFile(imageFile).exists())
    {
      cerr << "error: file " << qPrintable(imageFile) << " does not exist" << endl; 
      exit(1);
    }
  
    if(options.block())
    {
      convertBlocked(imageFile, ecatFile);
    }
    else
    {
      convert(imageFile, ecatFile);
    }
  }

  return 0;
}
