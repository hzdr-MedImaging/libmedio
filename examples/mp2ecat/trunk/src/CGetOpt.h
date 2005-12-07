/* vim:set ts=2 nowrap: ****************************************************

 LMMC - ListMode Movement Correction
 Copyright (C) 2003 by Jens Langner <Jens.Langner@light-speed.de>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

#ifndef CGETOPT_H
#define CGETOPT_H

//#include <q3dict.h>
#include <QList>
#include <QString>
#include <iostream>

#include <rtdebug.h>

// forward declarations
class CGetOptCategory;

class CGetOptOption
{
	public:
		enum Type { String=0, Int, Short, Long, Float, Flag };
	
		CGetOptOption(CGetOptOption::Type type, char shortOpt,
									char* longOpt=0, char* desc=0, char* defaultVal=0)
			: m_Type(type), m_ShortOption(shortOpt),
				m_pLongOption(longOpt), m_pDescription(desc),
				m_pDefaultValue(defaultVal), m_pFoundValue(0)
		{ }

		// accessor methods
		CGetOptOption::Type getType()	{ return m_Type;					}
		char	getShortOption()				{	return m_ShortOption;		}
		char*	getLongOption()					{ return m_pLongOption;		}
		char* getDescription()				{ return m_pDescription;	}
		char* getDefaultValue()				{ return m_pDefaultValue;	}
		char* getFoundValue()
		{
			char* result = 0;

			// return the default if a explicit value couldn't be found
			if(m_pFoundValue)				 result = m_pFoundValue;
			else if(m_pDefaultValue) result = m_pDefaultValue;
			
			return result;
		}

		// mutator methods
		void setType(CGetOptOption::Type type)	{ m_Type = type;						}
		void setShortOption(char so)						{ m_ShortOption = so;				}
		void setLongOption(char* longOpt)				{ m_pLongOption = longOpt;	}
		void setDescription(char* desc)					{ m_pDescription = desc;		}
		void setDefaultValue(char* defVal)			{ m_pDefaultValue = defVal;	}
		void setFoundValue(char* foundVal)			{ m_pFoundValue = foundVal;	}

		void show(void);

	private:
		CGetOptOption::Type	m_Type;
		char							m_ShortOption;
		char*							m_pLongOption;
		char*							m_pDescription;
		char*							m_pDefaultValue;
		char*							m_pFoundValue;
};

class CGetOptCategory : public QList<CGetOptOption*>
{
	public:
		CGetOptCategory(QString catName=0, QString catDesc=0)
			: m_Name(catName), m_Description(catDesc)
		{
			//setAutoDelete(true);
		}

		bool addOption(CGetOptOption::Type optType, char shortOpt, char* desc,
									 char* longOpt = 0, char* defaultVal = 0) 
		{
			CGetOptOption* newOption 
				= new CGetOptOption(optType, shortOpt, longOpt, desc, defaultVal);

			append(newOption);

			return true;
		}

		char* getValueOption(char shortOpt)
		{
			// we need to search through all our categories to find the option
			QListIterator<CGetOptOption*> it(*this);
			CGetOptOption* curOption;
			while(it.hasNext())
			{
				curOption = it.next();

				if(curOption->getShortOption() == shortOpt)
				{
					return curOption->getFoundValue();
				}
			}
			
			return 0;
		}

		void showOptions(void);	

		QString getName(void)					{ return m_Name;	}
		QString getDescription(void)	{ return m_Description;	}
			
	private:
		QString	m_Name;
		QString m_Description;
};

class CGetOpt : private QList<CGetOptCategory*>
{
	public:
		CGetOpt()
		{
			//setAutoDelete(true);
		}

		CGetOptCategory* addCategory(char* catName, char* catDescription=0)
		{
			CGetOptCategory* category = new CGetOptCategory(catName, catDescription);

			// add this category to our list
			append(category);
			
			return category;
		}
		
		bool addOption(char* category, CGetOptOption::Type optType, char shortOpt,
									 char* desc, char* longOpt = 0, char* defaultVal = 0) 
		{
			// lets iterate through our list and find the category with the supplied
			// name
			QListIterator<CGetOptCategory*> it(*this);
			CGetOptCategory* curCategory;
			while(it.hasNext())
			{
				curCategory = it.next();
				if(curCategory->getName() == category) break;
			}

			if(curCategory)
				return curCategory->addOption(optType, shortOpt, desc, longOpt, defaultVal);

			E("category not found");
			return false;
		}

		bool addOption(CGetOptCategory* category, CGetOptOption::Type optType, char shortOpt,
									 char* desc, char* longOpt = 0, char* defaultVal = 0) 
		{
			if(category)
			{
				return category->addOption(optType, shortOpt, longOpt, desc, defaultVal);
			}
			
			return false;
		}

		char* getValueOption(char shortOpt)
		{
			// we need to search through all our categories to find the option
			QListIterator<CGetOptCategory*> it(*this);
			CGetOptCategory* curCategory;
			while(it.hasNext())
			{
				curCategory = it.next();
				char* curFoundValue = curCategory->getValueOption(shortOpt);
				if(curFoundValue) return curFoundValue;
			}
			
			return 0;
		}

		bool parse(int argc, char* argv[]);
		void showAllOptions(void);
		void showCategoryOptions(QString category);

	protected:
	
	private:
};

#endif // CGETOPT_H
