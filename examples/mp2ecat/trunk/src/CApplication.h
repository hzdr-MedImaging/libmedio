#ifndef CAPPLICATION_H
#define CAPPLICATION_H

#include <QString>

class CApplication
{
	public:
		//constructors
		CApplication();

		//destructor
		~CApplication();

		//methods
		bool parseCmdLine(int argc, char* argv[]);
		bool convertFile();

	private:
		QString m_sInputFileName;
		QString m_sOutputFileName;
		QString m_sPatientName;
};

#endif // CAPPLICATION_H
