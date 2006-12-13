/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004 by Jens Langner <Jens.Langner@light-speed.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

#ifndef MEDIOUNITS_H
#define MEDIOUNITS_H

#include <QString>

// forward declarations

class CWeight
{
	public:
		enum WeightUnits { Unknown_units = 0,
											 Gram_units,
											 Ounce_units,
											 Kilogram_units,
											 Pound_units };

		CWeight();
		CWeight(const CWeight& other);
		CWeight(const float value, const WeightUnits u = Unknown_units);
	
		void setWeight(const float value);
		float weight() const;

		void setUnit(const WeightUnits u);
		WeightUnits unit() const;

		QString unitAsString() const;
		QString toString() const;
		CWeight convertTo(const WeightUnits u) const;
		CWeight& operator=(const CWeight& other);

	protected:
		// Private member vars
		float m_value;
		WeightUnits m_unit;	
};

class CLength
{
	public:
		enum LengthUnits { Unknown_units = 0,
											 Millimeter_units,
											 Centimeter_units,
											 Meter_units,
											 Inch_units };

		CLength();
		CLength(const CLength& other);
		CLength(const float value, const LengthUnits u = Unknown_units);
		
		void setLength(const float value);
		float length() const;

		void setUnit(const LengthUnits u);
		LengthUnits unit() const;

		QString unitAsString() const;
		QString toString() const;
		CLength convertTo(const LengthUnits u) const;
		CLength& operator=(const CLength& other);

	protected:
		// Private member vars
		float m_value;
		LengthUnits m_unit;
};

class CActivityConcentrationUnit
{
	public:
		enum ActivityConcentrationUnits { Unknown_units = 0,
																			BequerelsPerCubiccentimeter_units,
																			BequerelsPerMilliliter_units,
																			NanoCuriesPerCubiccentimeter_units,
																		};

		CActivityConcentrationUnit(const ActivityConcentrationUnits u = Unknown_units);
		CActivityConcentrationUnit(const CActivityConcentrationUnit& other);
		CActivityConcentrationUnit(const QString& s);
		
		void setUnit(const ActivityConcentrationUnits unit);
		void setUnitFromString(const QString& s);
		ActivityConcentrationUnits unit() const;

		QString unitAsString() const;
		QString activityUnitAsString() const;
		QString toString() const;

    void setUnknownUnitString(const QString & S);
    QString unknownUnitString(){return m_unknownUnitString;}

		CActivityConcentrationUnit& operator=(const CActivityConcentrationUnit& other);

	protected:
		// Private member vars
		ActivityConcentrationUnits m_unit;
    QString m_unknownUnitString;
};

class CDosage
{
	public:
		enum DosageUnits { Unknown_units = 0,
											 Bequerels_units,
											 Megabequerels_units,
											 Nanocurie_units,
											 Millicurie_units };

		CDosage();
		CDosage(const CDosage& other);
		CDosage(float value, const DosageUnits u = Unknown_units);
		
		void setDosage(const float value);
		float dosage() const;

		void setUnit(const DosageUnits unit);
		void setUnitFromString(const QString& s);
		DosageUnits unit() const;

		QString unitAsString() const;
		QString toString() const;

		CDosage& operator=(const CDosage& other);
		CDosage convertTo(const DosageUnits u) const;
		CDosage convertTo(const CActivityConcentrationUnit& a) const;

	protected:
		// Private member vars
		float m_value;
		DosageUnits m_unit;
};

#endif // MEDIOUNITS_H
