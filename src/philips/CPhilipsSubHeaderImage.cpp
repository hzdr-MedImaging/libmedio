/* -*- mode: c++; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 * vim:set ts=2 sw=2 expandtab: *********************************************
 *
 * libmedio - C++ I/O Library for loading/saving medical data formats
 * Copyright (C) 2006-2010 by Jens Langner <Jens.Langner@light-speed.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id$
 *
 **************************************************************************/

#include "CPhilipsSubHeaderImage.h"
#include "CPhilipsDirectoryItem.h"
#include "CPhilipsFile.h"
#include "CPhilipsMainHeader.h"

#include <QDataStream>
#include <QDateTime>
#include <QtCore/qmath.h>

#include <rtdebug.h>

CPhilipsSubHeaderImage::CPhilipsSubHeaderImage(CPhilipsFile* philipsFile,
                                               CPhilipsDirectoryItem* pDirItem)
  : CPhilipsSubHeader(philipsFile, pDirItem)
{
  ENTER();

  LEAVE();
}

CPhilipsSubHeaderImage::CPhilipsSubHeaderImage(const CPhilipsSubHeaderImage& src)
  : CPhilipsSubHeader(src)
{
  ENTER();

  LEAVE();
}

CPhilipsSubHeader::Type CPhilipsSubHeaderImage::subHeaderType(void) const
{
  return CPhilipsSubHeader::Image;
}

float CPhilipsSubHeaderImage::ecat_Scale_Factor(bool& ok, const CPhilipsMainHeader* mainHeader) const
{
  ENTER();

  float scaleFactor = 1.0f;
  ok = false;

  //
  // check if the SUV scaling factor has been set
  // in the subheader and if so we only take that scale
  // factor and no other to scale the raw data values.
  //
  // So the latest knowledge about how Philips scale their image data is
  //
  // 1. if there is a suvscl() factor use that one to scale to SUV units
  // 2. if suvscl() == 0 use imgscl() instead to scale to Bq/ml
  // 3. for sinograms use scnscl() instead of imgscl().
  //

  if(suvscl() > 0.0f)
  {
    bool reloadedMainHeader = false;

    // check if the mainHeader was supplied and if so we go
    // and use that instead of having to load it from the file
    if(mainHeader == NULL)
    {
      CMedIOData* mData = medIOData();

      if(mData != NULL && mData->isReadable())
      {
        CPhilipsMainHeader* mainHead = NULL;
        CPhilipsFile* file = static_cast<CPhilipsFile*>(mData);

        if(file->readMainHeader(mainHead))
        {
          mainHeader = mainHead;
          reloadedMainHeader = true;
        }
      }
    }

    // check if we have a valid main header now
    if(mainHeader != NULL)
    {
      // now calculate the scale factor
      float dosage = mainHeader->activity() * 1000000.0f; // Bq
      float deltaT = static_cast<float>(mainHeader->acq_date_time() -
                                        mainHeader->injection_date_time()); // s
      float halfLife = mainHeader->halfLife() * 60.0f; // s
      float patientWeight = static_cast<float>(mainHeader->weight()); // g

      if(patientWeight != 0 && halfLife != 0 &&
         dosage != 0)
      {
        // calculate the image scale factor by using dosage, deltaT, halfLife and
        // patient weight to get the scale factor which should be used scale each
        // image matrix element to get the real values.
        scaleFactor = suvscl() * dosage * qExp(-qLn(2) * (deltaT/halfLife)) / patientWeight;
        ok = true;
      }

      if(reloadedMainHeader)
        delete mainHeader;
    }
  }
  else if(imgscl() > 0.0f)
  {
    // else we use the imgscl() factor right away.
    scaleFactor = imgscl();
    ok = true;
  }

  RETURN(scaleFactor);
  return scaleFactor;
}

CMedIOHeader* CPhilipsSubHeaderImage::clone() const
{
  ENTER();

  CPhilipsSubHeaderImage* pNewHeader = new CPhilipsSubHeaderImage(*this);

  RETURN(pNewHeader);
  return pNewHeader;
}

float CPhilipsSubHeaderImage::suvscl() const
{
  return CPhilipsSubHeader::suvscl();
}

float CPhilipsSubHeaderImage::imgscl() const
{
  return CPhilipsSubHeader::imgscl();
}

short CPhilipsSubHeaderImage::imgmin() const
{
  return CPhilipsSubHeader::imgmin();
}

short CPhilipsSubHeaderImage::imgmax() const
{
  return CPhilipsSubHeader::imgmax();
}

float CPhilipsSubHeaderImage::imgsum() const
{
  return CPhilipsSubHeader::imgsum();
}

void CPhilipsSubHeaderImage::setSuvscl(const float suvscl)
{
  CPhilipsSubHeader::setSuvscl(suvscl);
}

void CPhilipsSubHeaderImage::setImgscl(const float imgscl)
{
  CPhilipsSubHeader::setImgscl(imgscl);
}

void CPhilipsSubHeaderImage::setImgmin(const short imgmin)
{
  CPhilipsSubHeader::setImgmin(imgmin);
}

void CPhilipsSubHeaderImage::setImgmax(const short imgmax)
{
  CPhilipsSubHeader::setImgmax(imgmax);
}

void CPhilipsSubHeaderImage::setImgsum(const float imgsum)
{
  CPhilipsSubHeader::setImgsum(imgsum);
}
