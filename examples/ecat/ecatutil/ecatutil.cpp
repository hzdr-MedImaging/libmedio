#include <CMedIO>
#include <CECATFile>
#include <CECAT7MainHeader>
#include <CECAT7SubHeaderScan3D>
#include <CECAT7SubHeaderImage>

#include <QFileInfo>
#include <QMultiHash>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QMap>

#include <iostream>
#include <iomanip>

#define INFEED_3D 116.40
#define INFEED_2D 135.80

using namespace std;

// global data
QString g_sECATFileName = 0;
QStringList g_sSrcFileNames;
QStringList g_inputFiles;
QString g_sMainHeaderEntry = 0;
QString g_sSubHeaderEntry = 0;
QString g_sMatrix = 0;
QString g_sValue = 0;
QString g_sPosition = 0;
QString g_sDestMatrix = 0;

float g_fInfeed = INFEED_3D;
QMultiHash<QString, QString> g_args;
enum g_eCommand {Get, Set, Copy, Join, Delete, New};
g_eCommand g_commando;
enum g_eCopyType {UnknownCopyType, CopyMainHeader, CopyItem};
g_eCopyType g_CopyOption = UnknownCopyType;
enum g_eDeleteType {UnknownDeleteType, DelItem};
g_eDeleteType g_DeleteOption = UnknownDeleteType;
enum g_eJoinFileIndex {UnknownIndex, Frames, Gates, Beds};
g_eJoinFileIndex g_joinIndex = UnknownIndex;
enum g_eEntyInfo {UnknownEntries, ShowMainHeaderEntries, ShowSubHeaderEntries};
g_eEntyInfo g_eShowEntryInfo = UnknownEntries;

enum g_eECAT7MainHeader
{
	UNKNOWN_MAINHEADER_ENTRY,
	MAGIC_NUMBER,
	ORIGINAL_FILE_NAME,
	SW_VERSION,
	SYSTEM_TYPE,
	FILE_TYPE,
	SERIAL_NUMBER,
	SCAN_START_TIME,
	ISOTOPE_NAME,
	ISOTOPE_HALFLIFE,
	RADIOPHARMACEUTICAL,
	GANTRY_TILT,
	ROTATION_TILT,
	BED_ELEVATION,
	INTRINSIC_TILT,
	WOBBLE_SPEED,
	TRANSM_SOURCE_TYPE,
	DISTANCE_SCANNED,
	TRANSAXIAL_FOV,
	ANGULAR_COMPRESSION,
	COIN_SAMP_MODE,
	AXIAL_SAMP_MODE,
	ECAT_CALIBRATION_FACTOR,
	CALIBRATION_UNITS,
	CALIBRATION_UNITS_LABEL,
	COMPRESSION_CODE,
	STUDY_TYPE,
	PATIENT_ID,
	PATIENT_NAME,
	PATIENT_SEX,
	PATIENT_DEXTERITY,
	PATIENT_AGE,
	PATIENT_HEIGHT,
	PATIENT_WEIGHT,
	PATIENT_BIRTH_DATE,
	PHYSICIAN_NAME,
	OPERATOR_NAME,
	STUDY_DESCRIPTION,
	ACQUISITION_TYPE,
	PATIENT_ORIENTATION,
	FACILITY_NAME,
	NUM_PLANES,
	NUM_FRAMES,
	NUM_GATES,
	NUM_BED_POS,
	INIT_BED_POSITION,
	BED_POSITION,
	PLANE_SEPARATION,
	LWR_SCTR_THRES,
	LWR_TRUE_THRES,
	UPR_TRUE_THRES,
	USER_PROCESS_CODE,
	ACQUISITION_MODE,
	BIN_SIZE,
	BRANCHING_FRACTION,
	DOSE_START_TIME,
	DOSAGE,
	WELL_COUNTER_CORR_FACTOR,
	DATA_UNITS,
	SEPTA_STATE,
	FILL
};
enum g_eECAT7ImageHeader
{
	UNKNOWN_IMAGEHEADER_ENTRY,
	DATA_TYPE_IMAGE,
	NUM_DIMENSIONS_IMAGE,
	X_DIMENSION,
	Y_DIMENSION,
	Z_DIMENSION,
	X_OFFSET,
	Y_OFFSET,
	Z_OFFSET,
	RECON_ZOOM,
	SCALE_FACTOR_IMAGE,
	IMAGE_MIN,
	IMAGE_MAX,
	X_PIXEL_SIZE,
	Y_PIXEL_SIZE,
	Z_PIXEL_SIZE,
	FRAME_DURATION_IMAGE,
	FRAME_START_TIME_IMAGE,
	FILTER_CODE,
	X_RESOLUTION_IMAGE,
	V_RESOLUTION_IMAGE,
	Z_RESOLUTION_IMAGE,
	NUM_R_ELEMENTS_IMAGE,
	NUM_ANGLES_IMAGE,
	Z_ROTATION_ANGLE,
	DECAY_CORR_FCTR,
	PROCESSING_CODE,
	GATE_DURATION_IMAGE,
	R_WAVE_OFFSET_IMAGE,
	NUM_ACCEPTED_BEATS_IMAGE,
	FILTER_CUTOFF_FREQUENCY,
	FILTER_RESOLUTION,
	FILTER_RAMP_SLOPE,
	FILTER_ORDER,
	FILTER_SCATTER_FRACTION,
	FILTER_SCATTER_SLOPE,
	ANNOTATION,
	MT_1_1,
	MT_1_2,
	MT_1_3,
	MT_2_1,
	MT_2_2,
	MT_2_3,
	MT_3_1,
	MT_3_2,
	MT_3_3,
	RFILTER_CUTOFF,
	RFILTER_RESOLUTION,
	RFILTER_CODE,
	RFILTER_ORDER,
	ZFILTER_CUTOFF,
	ZFILTER_RESOLUTION,
	ZFILTER_CODE,
	ZFILTER_ORDER,
	MT_1_4,
	MT_2_4,
	MT_3_4,
	SCATTER_TYPE,
	RECON_TYPE,
	RECON_VIEWS,
	FILL_CTI_IMAGE,
	FILL_USER_IMAGE
};
enum g_eECAT7Scan3DHeader
{
	UNKNOWN_SCAN3DHEADER_ENTRY,
	DATA_TYPE_SCAN3D,
	NUM_DIMENSIONS_SCAN3D,
	NUM_R_ELEMENTS_SCAN3D,
	NUM_ANGLES_SCAN3D,
	CORRECTIONS_APPLIED,
	NUM_Z_ELEMENTS,
	RING_DIFFERENCE,
	STORAGE_ORDER,
	AXIAL_COMPRESSION,
	X_RESOLUTION_SCAN3D,
	V_RESOLUTION_SCAN3D,
	Z_RESOLUTION_SCAN3D,
	W_RESOLUTION,
	FILL_GATING,
	GATE_DURATION_SCAN3D,
	R_WAVE_OFFSET_SCAN3D,
	NUM_ACCEPTED_BEATS_SCAN3D,
	SCALE_FACTOR_SCAN3D,
	SCAN_MIN,
	SCAN_MAX,
	PROMPTS,
	DELAYED,
	MULTIPLES,
	NET_TRUES,
	TOT_AVG_COR,
	TOT_AVG_UNCOR,
	TOTAL_COIN_RATE,
	FRAME_START_TIME_SCAN3D,
	FRAME_DURATION_SCAN3D,
	DEADTIME_CORRECTION_FACTOR,
	FILL_CTI_SCAN3D,
	FILL_USER_SCAN3D,
	UNCOR_SINGLES
};

QMap<QString,  g_eECAT7MainHeader> ecat7MainHeaderMap;
QMap<QString,  g_eECAT7ImageHeader> ecat7ImageHeaderMap;
QMap<QString,  g_eECAT7Scan3DHeader> ecat7Scan3DHeaderMap;

bool parseCommandLine(int& argc, char** argv);
bool processCommando_Get();
bool processCommando_Set();
bool processCommando_Copy();
bool processCommando_Join();
bool processCommando_Delete();
bool processCommando_New();
void showVersionInformation();
void showHelp(int& argc, char** argv);
void showMainHeaderEntries();
void showSubHeaderEntries();
bool initHeaderMaps();
bool retrieveMatrixIDValues(QString sMatrixID, int &iFrame, int &iPlane, int &iGate, int &iData, int &iBed);
bool retrieveFilterCode(short iFilterCode, CECAT7SubHeaderImage::Filter_Code &filterCode);

//  Function:    main
//!
//! This is a small and simple example utility to modify an ECAT file and process
//! some operations on it (e.g. remove or add a frame/matrix)
//!
//! @param       argc number of commandline options
//! @param       *argv[] pointer to a array of the commandline options
//! @return      integer of the exit code.
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	int iReturnCode = 0; // return no error on default
	bool bResult = false;
	// You want this, unless you mix couts output with C output.
  // Read  http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8 for an explanation.
  ios::sync_with_stdio(false);

	// we first parse our command line to check if there are no syntax errors
	if(initHeaderMaps() && parseCommandLine(argc, argv))
	{
		switch(g_commando)
		{
			case Get:	bResult = processCommando_Get(); break;
			case Set:	bResult = processCommando_Set(); break;
			case Copy:	bResult = processCommando_Copy(); break;
			case Join:	bResult = processCommando_Join(); break;
			case Delete:	bResult = processCommando_Delete(); break;
			case New:	bResult = processCommando_New(); break;
			default: cout << "ERROR: unknown commando." << endl;
		}
	}
	
	if(!bResult)
		iReturnCode = 1;
	return iReturnCode;
}

bool processCommando_Get()
{
	bool bResult = true;
	int iFrame = 1;
	int iPlane = 1;
	int iGate = 1;
	int iData = 0;
	int iBed = 0;
	// before we are processing the user input we check if all parameters from commandline
	// are set correctly
	// first check if ecatfile is existing
	QFileInfo fileInfo(g_sECATFileName);
	if(!(fileInfo.exists() && fileInfo.isFile() && fileInfo.isReadable()))
	{
		cout << "ERROR: couldn't read ecatfile'" << g_sECATFileName.toAscii().constData() << "'." << endl;
		bResult = false;
	}

	int iNumHeadersInCmdLine = 0;
	if(!g_sMainHeaderEntry.isEmpty())
		iNumHeadersInCmdLine++;
	if(!g_sSubHeaderEntry.isEmpty())
		iNumHeadersInCmdLine++;

	if(iNumHeadersInCmdLine == 0)
	{
		cout << "ERROR: no headerentry provided."<< endl;
		bResult = false;
	}

	if(iNumHeadersInCmdLine > 1)
	{
		cout << "ERROR: main- and subheaderentry provided, only one supported."<< endl;
		bResult = false;
	}

	// now check if an subheaderentry is provided we need an matrixID too
	if(!g_sSubHeaderEntry.isEmpty())
	{
		if(g_sMatrix.isEmpty())
		{
			cout << "ERROR: no matrixID provided."<< endl;
			bResult = false;
		}
		else
		{
			bResult = retrieveMatrixIDValues(g_sMatrix, iFrame, iPlane, iGate, iData, iBed);
		}
	}

	if(bResult)
	{
		// open ecatfile
		CECATFile ecatFile(g_sECATFileName);
		if(ecatFile.open(QIODevice::ReadOnly) && ecatFile.format() != CECATFile::Undefined)
		{
			if(!g_sMainHeaderEntry.isEmpty())
			{
				CECATMainHeader* pMainHeader;
				if(ecatFile.readMainHeader(pMainHeader))
				{
					CECAT7MainHeader* pECAT7MainHeader = static_cast<CECAT7MainHeader*>(pMainHeader);
					if(pECAT7MainHeader)
					{
						switch(ecat7MainHeaderMap.value(g_sMainHeaderEntry))
						{
							case MAGIC_NUMBER: cout << pECAT7MainHeader->magic_Number() << endl; break;
							case FILE_TYPE: cout << pECAT7MainHeader->file_Type() << endl; break;
							case ORIGINAL_FILE_NAME: cout << pECAT7MainHeader->original_File_Name() << endl; break;
							case ANGULAR_COMPRESSION: cout << pECAT7MainHeader->angular_Compression() << endl; break;
							case CALIBRATION_UNITS_LABEL: cout << pECAT7MainHeader->calibration_Units_Label() << endl; break;
							case DATA_UNITS: cout << pECAT7MainHeader->data_Units() << endl; break;
							case ACQUISITION_TYPE: cout << pECAT7MainHeader->acquisition_Type() << endl; break;
							case PATIENT_ORIENTATION: cout << pECAT7MainHeader->patient_Orientation() << endl; break;
							case ISOTOPE_NAME: cout << pECAT7MainHeader->isotope_Name() << endl; break;
							case ISOTOPE_HALFLIFE: cout << pECAT7MainHeader->isotope_Halflife() << endl; break;
							case RADIOPHARMACEUTICAL: cout << pECAT7MainHeader->radiopharmaceutical() << endl; break;
							case ECAT_CALIBRATION_FACTOR: cout << pECAT7MainHeader->calibration_Factor() << endl; break;
							case CALIBRATION_UNITS: cout << pECAT7MainHeader->calibration_Units() << endl; break;
							default:
							{
								cout << "ERROR: main header entry not supported." << endl;
								bResult = false;
							}
						}
					}
					else
					{
						cout << "ERROR: no ecat7 main header '" << g_sECATFileName.toAscii().constData() << "'" << endl;
						bResult = false;
					}
				}
				else
				{
					cout << "ERROR: couldn't read main header '" << g_sECATFileName.toAscii().constData() << "'" << endl;
					bResult = false;
				}
			}
			else
			{
				CECATSubHeader* pSubHeader = NULL;
				if(ecatFile.readSubHeader(pSubHeader, iFrame, iPlane, iGate, iBed, iData))
				{
					switch(pSubHeader->subHeaderType())
					{
						case CECATSubHeader::ECAT7_Image:
						{
							CECAT7SubHeaderImage* pECAT7ImageSubHeader = static_cast<CECAT7SubHeaderImage*>(pSubHeader);
							if(pECAT7ImageSubHeader)
							{
								switch(ecat7ImageHeaderMap.value(g_sSubHeaderEntry))
								{
									case RECON_ZOOM: cout << pECAT7ImageSubHeader->recon_Zoom() << endl; break;
									case X_PIXEL_SIZE: cout << pECAT7ImageSubHeader->x_Pixel_Size() << endl; break;
									case Y_PIXEL_SIZE: cout << pECAT7ImageSubHeader->y_Pixel_Size() << endl; break;
									case Z_PIXEL_SIZE: cout << pECAT7ImageSubHeader->z_Pixel_Size() << endl; break;
									default:
									{
										cout << "ERROR: sub header entry not supported." << endl;
										bResult = false;
									}
								}
							}
							else
							{
								cout << "ERROR: no ecat7 image header '" << g_sECATFileName.toAscii().constData() << "'" << endl;
								bResult = false;
							}
						}
						break;
						default:
						{
							cout << "ERROR: unsupported subheader" << endl;
							bResult = false;
						}
					}
				}
				else
				{
					cout << "ERROR: couldn't read subheader '" << g_sECATFileName.toAscii().constData() << "'" << endl;
					bResult = false;
				}
			}
			ecatFile.close();
		}
		else
		{
			cout << "ERROR: provided ecatfile is not an ECAT file." << endl;
			bResult = false;
		}
	}
	return bResult;
}

bool processCommando_Set()
{
	bool bResult = true;
	int iFrame = 1;
	int iPlane = 1;
	int iGate = 1;
	int iData = 0;
	int iBed = 0;
	// before we are processing the user input we check if all parameters from commandline
	// are set correctly
	// first check if ecatfile is existing
	QFileInfo fileInfo(g_sECATFileName);
	if(!(fileInfo.exists() && fileInfo.isFile() && fileInfo.isReadable() && fileInfo.isWritable()))
	{
		cout << "ERROR: couldn't read/write ecatfile'" << g_sECATFileName.toAscii().constData() << "'." << endl;
		bResult = false;
	}

	int iNumHeadersInCmdLine = 0;
	if(!g_sMainHeaderEntry.isEmpty())
		iNumHeadersInCmdLine++;
	if(!g_sSubHeaderEntry.isEmpty())
		iNumHeadersInCmdLine++;

	if(iNumHeadersInCmdLine == 0)
	{
		cout << "ERROR: no headerentry provided."<< endl;
		bResult = false;
	}

	if(iNumHeadersInCmdLine > 1)
	{
		cout << "ERROR: main- and subheaderentry provided, only one supported."<< endl;
		bResult = false;
	}

	// now check if an subheaderentry is provided we need an matrixID too
	if(!g_sSubHeaderEntry.isEmpty())
	{
		if(g_sMatrix.isEmpty())
		{
			cout << "ERROR: no matrixID provided."<< endl;
			bResult = false;
		}
		else
		{
			bResult = retrieveMatrixIDValues(g_sMatrix, iFrame, iPlane, iGate, iData, iBed);
		}
	}

	// at last we need to check if an value is provided
	if(g_sValue.isEmpty())
	{
		cout << "ERROR: no value available to set in header."<< endl;
		bResult = false;
	}
	
	if(bResult)
	{
		// open ecatfile
		CECATFile ecatFile(g_sECATFileName);
		if(ecatFile.open(QIODevice::ReadWrite) && ecatFile.format() != CECATFile::Undefined)
		{
			if(!g_sMainHeaderEntry.isEmpty())
			{
				CECATMainHeader* pMainHeader;
				if(ecatFile.readMainHeader(pMainHeader))
				{
					CECAT7MainHeader* pECAT7MainHeader = static_cast<CECAT7MainHeader*>(pMainHeader);
					if(pECAT7MainHeader)
					{
						switch(ecat7MainHeaderMap.value(g_sMainHeaderEntry))
						{
							case FILE_TYPE: 
							{
								bool bSuccess = false;
								short iFileType = g_sValue.toInt(&bSuccess);
								if(bSuccess)
								{
									switch(iFileType)
									{
										case 1: pECAT7MainHeader->setFile_Type(CECAT7MainHeader::Sinogram); break;
										case 2: pECAT7MainHeader->setFile_Type(CECAT7MainHeader::Image16); break;
										case 3: pECAT7MainHeader->setFile_Type(CECAT7MainHeader::AttenuationCorr); break;
										case 4: pECAT7MainHeader->setFile_Type(CECAT7MainHeader::Normalization); break;
										case 5: pECAT7MainHeader->setFile_Type(CECAT7MainHeader::PolarMap); break;
										case 6: pECAT7MainHeader->setFile_Type(CECAT7MainHeader::Volume8); break;
										case 7: pECAT7MainHeader->setFile_Type(CECAT7MainHeader::Volume16); break;
										case 8: pECAT7MainHeader->setFile_Type(CECAT7MainHeader::Projection8); break;
										case 9: pECAT7MainHeader->setFile_Type(CECAT7MainHeader::Projection16); break;
										case 10: pECAT7MainHeader->setFile_Type(CECAT7MainHeader::Image8); break;
										case 11: pECAT7MainHeader->setFile_Type(CECAT7MainHeader::Sinogram3D_16); break;
										case 12: pECAT7MainHeader->setFile_Type(CECAT7MainHeader::Sinogram3D_8); break;
										case 13: pECAT7MainHeader->setFile_Type(CECAT7MainHeader::Normalization_3D); break;
										case 14: pECAT7MainHeader->setFile_Type(CECAT7MainHeader::Sinogram3D_Float); break;
										default:
										{
											cout << "ERROR: file type not supported." << endl;
											bResult = false;
										}
									}
								}
								else
								{
									cout << "ERROR: file type is not a short value." << endl;
									bResult = false;
								}
							}
							break;
							case ISOTOPE_NAME:
							{
								pECAT7MainHeader->setIsotope_Name(g_sValue.toAscii().data());
							}
							break;
							case ISOTOPE_HALFLIFE:
							{
								bool success = false;
								float fIsotopeHalflife = g_sValue.toFloat(&success);
								if(success)
									pECAT7MainHeader->setIsotope_Halflife(fIsotopeHalflife);
								else
								{
									cout << "ERROR: can not convert isotope halflife to float value." << endl;
									bResult = false;
								}
							}
							break;
							case RADIOPHARMACEUTICAL:
							{
								pECAT7MainHeader->setRadiopharmaceutical(g_sValue.toAscii().data());
							}
							break;
							case ECAT_CALIBRATION_FACTOR:
							{
								bool success = false;
								float fCalibrationFactor = g_sValue.toFloat(&success);
								if(success)
									pECAT7MainHeader->setCalibration_Factor(fCalibrationFactor);
								else
								{
									cout << "ERROR: can not convert calibration factor to float value." << endl;
									bResult = false;
								}
							}
							break;
							case ORIGINAL_FILE_NAME:
							{
								pECAT7MainHeader->setOriginal_File_Name(g_sValue.toAscii().data());
							}
							break;
							case ANGULAR_COMPRESSION:
							{
								bool bSuccess = false;
								short iAngularCompression = g_sValue.toInt(&bSuccess);
								if(bSuccess)
								{
									switch(iAngularCompression)
									{
										case 0: pECAT7MainHeader->setAngular_Compression(CECAT7MainHeader::No_Mash); break;
										case 1: pECAT7MainHeader->setAngular_Compression(CECAT7MainHeader::Mash2); break;
										case 2: pECAT7MainHeader->setAngular_Compression(CECAT7MainHeader::Mash4); break;
										default:
										{
											cout << "ERROR: angular compression not supported." << endl;
											bResult = false;
										}
									}
								}
								else
								{
									cout << "ERROR: angular compression is not a short value." << endl;
									bResult = false;
								}
							}
							break;
							case ACQUISITION_TYPE:
							{
								bool bSuccess = false;
								short iAcquisitionType = g_sValue.toInt(&bSuccess);
								if(bSuccess)
								{
									switch(iAcquisitionType)
									{
										case 1: pECAT7MainHeader->setAcquisition_Type(CECAT7MainHeader::Blank); break;
										case 2: pECAT7MainHeader->setAcquisition_Type(CECAT7MainHeader::Transmission); break;
										case 3: pECAT7MainHeader->setAcquisition_Type(CECAT7MainHeader::StaticEmission); break;
										case 4: pECAT7MainHeader->setAcquisition_Type(CECAT7MainHeader::DynamicEmission); break;
										case 5: pECAT7MainHeader->setAcquisition_Type(CECAT7MainHeader::GatedEmission); break;
										case 6: pECAT7MainHeader->setAcquisition_Type(CECAT7MainHeader::TransmissionRectilinear); break;
										case 7: pECAT7MainHeader->setAcquisition_Type(CECAT7MainHeader::EmissionRectilinear); break;
										default:
										{
											cout << "ERROR: acquisition type not supported." << endl;
											bResult = false;
										}
									}
								}
								else
								{
									cout << "ERROR: acquisition type is not a short value." << endl;
									bResult = false;
								}
							}
							break;
							case PATIENT_ORIENTATION:
							{
								bool bSuccess = false;
								short iPatientOrientation = g_sValue.toInt(&bSuccess);
								if(bSuccess)
								{
									switch(iPatientOrientation)
									{
										case 0: pECAT7MainHeader->setPatient_Orientation(CECAT7MainHeader::FF_Prone); break;
										case 1: pECAT7MainHeader->setPatient_Orientation(CECAT7MainHeader::HF_Prone); break;
										case 2: pECAT7MainHeader->setPatient_Orientation(CECAT7MainHeader::FF_Supine); break;
										case 3: pECAT7MainHeader->setPatient_Orientation(CECAT7MainHeader::HF_Supine); break;
										case 4: pECAT7MainHeader->setPatient_Orientation(CECAT7MainHeader::FF_Right); break;
										case 5: pECAT7MainHeader->setPatient_Orientation(CECAT7MainHeader::HF_Right); break;
										case 6: pECAT7MainHeader->setPatient_Orientation(CECAT7MainHeader::FF_Left); break;
										case 7: pECAT7MainHeader->setPatient_Orientation(CECAT7MainHeader::HF_Left); break;
										default:
										{
											cout << "ERROR: patient orientation not supported." << endl;
											bResult = false;
										}
									}
								}
								else
								{
									cout << "ERROR: patient orientation is not a short value." << endl;
									bResult = false;
								}
							}
							break;
							case CALIBRATION_UNITS:
							{
								bool bSuccess = false;
								short iCalibrationUnits = g_sValue.toInt(&bSuccess);
								if(bSuccess)
								{
									switch(iCalibrationUnits)
									{
										case 0: pECAT7MainHeader->setCalibration_Units(CECAT7MainHeader::Uncalibrated); break;
										case 1: pECAT7MainHeader->setCalibration_Units(CECAT7MainHeader::Calibrated); break;
										case 2: pECAT7MainHeader->setCalibration_Units(CECAT7MainHeader::CalibrationUnits_Processed); break;
										default:
										{
											cout << "ERROR: calibration units not supported." << endl;
											bResult = false;
										}
									}
								}
								else
								{
									cout << "ERROR: calibration units is not a short value." << endl;
									bResult = false;
								}
							}
							break;
							case CALIBRATION_UNITS_LABEL:
							{
								bool bSuccess = false;
								short iCalibrationUnitsLabel = g_sValue.toInt(&bSuccess);
								if(bSuccess)
								{
									switch(iCalibrationUnitsLabel)
									{
										case 0: pECAT7MainHeader->setCalibration_Units_Label(CECAT7MainHeader::Blood_Flow); break;
										case 1: pECAT7MainHeader->setCalibration_Units_Label(CECAT7MainHeader::LMRGLU); break;
										case 2: pECAT7MainHeader->setCalibration_Units_Label(CECAT7MainHeader::Label_Processed); break;
										default:
										{
											cout << "ERROR: calibration units label not supported." << endl;
											bResult = false;
										}
									}
								}
								else
								{
									cout << "ERROR: calibration units label is not a short value." << endl;
									bResult = false;
								}
							}
							break;
							case DATA_UNITS:
							{
								pECAT7MainHeader->setData_Units(g_sValue.toAscii().data());
							}
							break;
							case DOSE_START_TIME:
							{
								QDateTime doseStartTime = QDateTime::fromString(g_sValue, QString("dd.MM.yyyy hh:mm:ss"));
								if(doseStartTime.isValid())
									pECAT7MainHeader->setDose_Start_Time_Qt(doseStartTime);
								else
								{
									cout << "ERROR: can not convert dose start time string to datetime object." << endl;
									bResult = false;
								}
							}
							break;
							case SCAN_START_TIME:
							{
								QDateTime scanStartTime = QDateTime::fromString(g_sValue, QString("dd.MM.yyyy hh:mm:ss"));
								if(scanStartTime.isValid())
									pECAT7MainHeader->setScan_Start_Time_Qt(scanStartTime);
								else
								{
									scanStartTime = QDateTime::fromString(g_sValue, QString("dd.MM.yyyy hh:mm:ss.zzz"));
									if(scanStartTime.isValid())
									{
										pECAT7MainHeader->setScan_Start_Time_Qt(scanStartTime);
									}
									else
									{
										cout << "ERROR: can not convert scan start time string to datetime object." << endl;
										bResult = false;
									}
								}
							}
							break;
							case BED_ELEVATION:
							{
								bool success = false;
								float fElevation = g_sValue.toFloat(&success);
								if(success)
									pECAT7MainHeader->setBed_Elevation(fElevation);
								else
								{
									cout << "ERROR: can not convert bed elevation to float value." << endl;
									bResult = false;
								}
							}
							break;
							case BED_POSITION:
							{
								// this option needs an index position so we check
								// if an position information is provided and
								// if position is within limits of header entry
								bool bSuccess = false;
								short iPosition = g_sPosition.toInt(&bSuccess);
								if(bSuccess && iPosition >= 0 && iPosition < 15)
								{
									bool success = false;
									float fBedPosition = g_sValue.toFloat(&success);
									if(success)
										pECAT7MainHeader->setBed_Offset(iPosition ,fBedPosition);
									else
									{
										cout << "ERROR: can not covert bed position to float value." << endl;
										bResult = false;
									}
								}
								else
								{
									cout << "ERROR: can not set bed position value - no index position provided." << endl;
									bResult = false;
								}
							}
							break;
							default:
							{
								cout << "ERROR: main header entry not supported." << endl;
								bResult = false;
							}
						}
						if(bResult)
						{
							if(!ecatFile.writeMainHeader(*pECAT7MainHeader))
								cout << "ERROR: couldn't write main header." << endl;
						}
					}
					else
					{
						cout << "ERROR: no ecat7 main header '" << g_sECATFileName.toAscii().constData() << "'" << endl;
						bResult = false;
					}
				}
				else
				{
					cout << "ERROR: couldn't read main header '" << g_sECATFileName.toAscii().constData() << "'" << endl;
					bResult = false;
				}
			}
			else
			{
				CECATSubHeader* pSubHeader = NULL;
				if(ecatFile.readSubHeader(pSubHeader, iFrame, iPlane, iGate, iBed, iData))
				{
					switch(pSubHeader->subHeaderType())
					{
						case CECATSubHeader::ECAT7_Image:
						{
							CECAT7SubHeaderImage* pECAT7ImageSubHeader = static_cast<CECAT7SubHeaderImage*>(pSubHeader);
							if(pECAT7ImageSubHeader)
							{
								switch(ecat7ImageHeaderMap.value(g_sSubHeaderEntry))
								{
									case X_PIXEL_SIZE:
									{
										bool success = false;
										float fPixelSize = g_sValue.toFloat(&success);
										if(success)
											pECAT7ImageSubHeader->setX_Pixel_Size(fPixelSize);
										else
										{
											cout << "ERROR: can not convert x-pixel size to float value." << endl;
											bResult = false;
										}
									}
									break;
									case Y_PIXEL_SIZE:
									{
										bool success = false;
										float fPixelSize = g_sValue.toFloat(&success);
										if(success)
											pECAT7ImageSubHeader->setY_Pixel_Size(fPixelSize);
										else
										{
											cout << "ERROR: can not convert y-pixel size to float value." << endl;
											bResult = false;
										}
									}
									break;
									case Z_PIXEL_SIZE:
									{
										bool success = false;
										float fPixelSize = g_sValue.toFloat(&success);
										if(success)
											pECAT7ImageSubHeader->setZ_Pixel_Size(fPixelSize);
										else
										{
											cout << "ERROR: can not convert z-pixel size to float value." << endl;
											bResult = false;
										}
									}
									break;
									case FILTER_CODE:
									{
										bool bSuccess = false;
										short iFilterCode = g_sValue.toInt(&bSuccess);
										if(bSuccess)
										{
											CECAT7SubHeaderImage::Filter_Code fCode = CECAT7SubHeaderImage::NoFilter;
											if(retrieveFilterCode(iFilterCode, fCode))
												pECAT7ImageSubHeader->setFilter_Code(fCode);
										}
										else
										{
											cout << "ERROR: wrong filter code format." << endl;
											bResult = false;
										}
									}
									break;
									case RFILTER_CODE:
									{
										bool bSuccess = false;
										short iFilterCode = g_sValue.toInt(&bSuccess);
										if(bSuccess)
										{
											CECAT7SubHeaderImage::Filter_Code fCode = CECAT7SubHeaderImage::NoFilter;
											if(retrieveFilterCode(iFilterCode, fCode))
												pECAT7ImageSubHeader->setRFilter_Code(fCode);
										}
										else
										{
											cout << "ERROR: wrong rfilter code format." << endl;
											bResult = false;
										}
									}
									break;
									case RFILTER_RESOLUTION:
									{
										bool success = false;
										float fRFilterResolution = g_sValue.toFloat(&success);
										if(success)
											pECAT7ImageSubHeader->setRFilter_Resolution(fRFilterResolution);
										else
										{
											cout << "ERROR: can not convert rfilter resolution to float value." << endl;
											bResult = false;
										}
									}
									break;
									case ZFILTER_CODE:
									{
										bool bSuccess = false;
										short iFilterCode = g_sValue.toInt(&bSuccess);
										if(bSuccess)
										{
											CECAT7SubHeaderImage::Filter_Code fCode = CECAT7SubHeaderImage::NoFilter;
											if(retrieveFilterCode(iFilterCode, fCode))
												pECAT7ImageSubHeader->setZFilter_Code(fCode);
										}
										else
										{
											cout << "ERROR: wrong zfilter code format." << endl;
											bResult = false;
										}
									}
									break;
									case ZFILTER_RESOLUTION:
									{
										bool success = false;
										float fZFilterResolution = g_sValue.toFloat(&success);
										if(success)
											pECAT7ImageSubHeader->setZFilter_Resolution(fZFilterResolution);
										else
										{
											cout << "ERROR: can not convert zfilter resolution to float value." << endl;
											bResult = false;
										}
									}
									break;
									case RECON_ZOOM:
									{
										bool success = false;
										float fReconZoom = g_sValue.toFloat(&success);
										if(success)
											pECAT7ImageSubHeader->setRecon_Zoom(fReconZoom);
										else
										{
											cout << "ERROR: can not convert recon zoom to float value." << endl;
											bResult = false;
										}
									}
									break;
									case NUM_R_ELEMENTS_IMAGE:
									{
										bool success = false;
										int iNumRElements = g_sValue.toInt(&success);
										if(success)
											pECAT7ImageSubHeader->setNum_R_Elements(iNumRElements);
										else
										{
											cout << "ERROR: can not convert num relements to int value." << endl;
											bResult = false;
										}
									}
									break;
									case NUM_ANGLES_IMAGE:
									{
										bool success = false;
										int iNumAngles = g_sValue.toInt(&success);
										if(success)
											pECAT7ImageSubHeader->setNum_Angles(iNumAngles);
										else
										{
											cout << "ERROR: can not convert num angles to int value." << endl;
											bResult = false;
										}
									}
									break;
									case PROCESSING_CODE:
									{
										bool success = false;
										int iProcessingCode = g_sValue.toInt(&success);
										if(success)
											pECAT7ImageSubHeader->setProcessing_Code(iProcessingCode);
										else
										{
											cout << "ERROR: can not convert processing code to int value." << endl;
											bResult = false;
										}
									}
									break;
									default:
									{
										cout << "ERROR: sub header entry not supported." << endl;
										bResult = false;
									}
								}
								if(bResult)
								{
									if(!ecatFile.writeSubHeader(*pECAT7ImageSubHeader, iFrame, iPlane, iGate, iBed, iData))
										cout << "ERROR: could not write image subheader." << endl;
								}
							}
							else
							{
								cout << "ERROR: no ecat7 image header '" << g_sECATFileName.toAscii().constData() << "'" << endl;
								bResult = false;
							}
						}
						break;
						case CECATSubHeader::ECAT7_Scan3D:
						{
							CECAT7SubHeaderScan3D* pECAT7Scan3DSubHeader = static_cast<CECAT7SubHeaderScan3D*>(pSubHeader);
							if(pECAT7Scan3DSubHeader)
							{
								switch(ecat7Scan3DHeaderMap.value(g_sSubHeaderEntry))
								{
									case FRAME_START_TIME_SCAN3D:
									{
										bool success = false;
										int iFrameStartTime = g_sValue.toInt(&success);
										if(success)
											pECAT7Scan3DSubHeader->setFrame_Start_Time(iFrameStartTime);
										else
										{
											cout << "ERROR: can not convert frame start time to int value." << endl;
											bResult = false;
										}
									}
									break;
									default:
									{
										cout << "ERROR: sub header entry not supported." << endl;
										bResult = false;
									}
								}
								if(bResult)
								{
									if(!ecatFile.writeSubHeader(*pECAT7Scan3DSubHeader, iFrame, iPlane, iGate, iBed, iData))
										cout << "ERROR: could not write scan3d subheader." << endl;
								}
							}
						}
						break;
						default:
						{
							cout << "ERROR: unsupported subheader" << endl;
							bResult = false;
						}
					}
				}
				else
				{
					cout << "ERROR: couldn't read subheader '" << g_sECATFileName.toAscii().constData() << "'" << endl;
					bResult = false;
				}
			}
			ecatFile.close();
		}
		else
		{
			cout << "ERROR: provided ecatfile is not an ECAT file." << endl;
			bResult = false;
		}
	}
	return bResult;
}

bool processCommando_Copy()
{
	bool bResult = true;

	// we check if we can read the provided ecat file
	QFileInfo fileInfo(g_sECATFileName);
	if(!(fileInfo.exists() && fileInfo.isFile() && fileInfo.isReadable() && fileInfo.isWritable()))
	{
		cout << "ERROR: couldn't read/write ecatfile'" << g_sECATFileName.toAscii().constData() << "'." << endl;
		bResult = false;
	}

	// first we check that we have one and only one source file to copy
	// information from
	if(g_sSrcFileNames.count() != 1)
	{
		cout << "ERROR: we can copy information only from one ecat file to another." << endl;
		bResult = false;
	}

	// we check if we can read the provided source file
	QFileInfo srcFileInfo(g_sSrcFileNames.first());
	if(!(srcFileInfo.exists() && srcFileInfo.isFile() && srcFileInfo.isReadable()))
	{
		cout << "ERROR: couldn't read source file'" << g_sSrcFileNames.first().toAscii().constData() << "'." << endl;
		bResult = false;
	}

	// we check the user provided a source and a destination matrixID if he likes to copy
	// a directory item
	if(g_CopyOption == CopyItem)
	{
		if(g_sMatrix.isEmpty())
		{
			cout << "ERROR: when copying a directory item we need a matrixID." << endl;
			bResult = false;
		}
		if(g_sDestMatrix.isEmpty())
		{
			cout << "ERROR: when copying a directory item we need a destination matrixID." << endl;
			bResult = false;
		}
	}

	if(bResult)
	{
		// split here in copy mainheader and directory item
		switch(g_CopyOption)
		{
			case CopyMainHeader:
			{
				// open ecatfile
				CECATFile ecatFile(g_sECATFileName);
				if(ecatFile.open(QIODevice::ReadWrite) && ecatFile.format() != CECATFile::Undefined)
				{
					// open src ecat file
					CECATFile srcEcatFile(g_sSrcFileNames.first());
					if(srcEcatFile.open(QIODevice::ReadOnly) && ecatFile.format() != CECATFile::Undefined)
					{
						CECATMainHeader* srcEcatMainHeader = NULL;
						CECATMainHeader* ecatMainHeader = NULL;
						if(srcEcatFile.readMainHeader(srcEcatMainHeader))
						{
							if(ecatFile.readMainHeader(ecatMainHeader))
							{
								*ecatMainHeader = *srcEcatMainHeader;
								if(!ecatFile.writeMainHeader(*ecatMainHeader))
								{
									cout << "ERROR: could not write destination main header." << endl;
									bResult = false;
								}
								cout << ecatMainHeader->num_Gates() << endl;
								cout << srcEcatMainHeader->num_Gates() << endl;
							}
							else
							{
								cout << "ERROR: could not read destination main header." << endl;
								bResult = false;
							}
						}
						else
						{
							cout << "ERROR: could not read src main header." << endl;
							bResult = false;
						}
						srcEcatFile.close();
					}
					else
					{
						cout << "ERROR: provided src file is not an ECAT file." << endl;
						bResult = false;
					}
					ecatFile.close();
				}
				else
				{
					cout << "ERROR: provided ecatfile is not an ECAT file." << endl;
					bResult = false;
				}
			}
			break;
			case CopyItem:
			{
				// check if source / destination matrixID has correct format
				int iSrcFrame = 0;
				int iSrcPlane = 0;
				int iSrcGate = 0;
				int iSrcData = 0;
				int iSrcBed = 0;
				if(!retrieveMatrixIDValues(g_sMatrix, iSrcFrame, iSrcPlane, iSrcGate, iSrcData, iSrcBed))
				{
					cout << "ERROR: source matrixID has wrong format." << endl;
					bResult = false;
				}

				int iDestFrame = 0;
				int iDestPlane = 0;
				int iDestGate = 0;
				int iDestData = 0;
				int iDestBed = 0;
				if(!retrieveMatrixIDValues(g_sDestMatrix, iDestFrame, iDestPlane, iDestGate, iDestData, iDestBed))
				{
					cout << "ERROR: destination matrixID has wrong format." << endl;
					bResult = false;
				}

				if(bResult)
				{
					// open ecatfile
					CECATFile ecatFile(g_sECATFileName);
					if(ecatFile.open(QIODevice::ReadWrite) && ecatFile.format() != CECATFile::Undefined)
					{
						// open src ecat file
						CECATFile srcEcatFile(g_sSrcFileNames.first());
						if(srcEcatFile.open(QIODevice::ReadOnly) && ecatFile.format() != CECATFile::Undefined)
						{
							// check if subheader and matrix is in available in source file
							CECATSubHeader* pSrcSubHeader = NULL;
							QByteArray* pSrcMatrixData = NULL;
							if(srcEcatFile.readSubHeader(pSrcSubHeader, iSrcFrame, iSrcPlane, iSrcGate, iSrcBed, iSrcData))
							{
								if(srcEcatFile.readMatrix(pSrcMatrixData, iSrcFrame, iSrcPlane, iSrcGate, iSrcBed, iSrcData))
								{
									// we check if the destination matrix is already existing in destination ecat file
									CECATSubHeader* pDestSubHeader = NULL;
									if(!ecatFile.readSubHeader(pDestSubHeader, iDestFrame, iDestPlane, iDestGate, iDestBed, iDestData))
									{
										pDestSubHeader = NULL;
										pDestSubHeader = ecatFile.createEmptySubHeader();
										*pDestSubHeader = *pSrcSubHeader;
										if(ecatFile.writeSubHeader(*pDestSubHeader, iDestFrame, iDestPlane, iDestGate, iDestBed, iDestData))
										{
											if(!ecatFile.writeMatrix(*pSrcMatrixData, iDestFrame, iDestPlane, iDestGate, iDestBed, iDestData))
											{
												cout << "ERROR: can not write destination matrix." << endl;
												bResult = false;
											}
										}
										else
										{
											cout << "ERROR: can not write destination subheader." << endl;
											bResult = false;
										}
									}
									else
									{
										cout << "ERROR: directory item is already existing in destination ecat file." << endl;
										bResult = false;
									}
								}
								else
								{
									cout << "ERROR: could not read matrix data of eact source file." << endl;
									bResult = false;
								}
							}
							else
							{
								cout << "ERROR: could not read subheader of eact source file." << endl;
								bResult = false;
							}
							srcEcatFile.close();
						}
						else
						{
							cout << "ERROR: provided src file is not an ECAT file." << endl;
							bResult = false;
						}
						ecatFile.close();
					}
					else
					{
						cout << "ERROR: provided ecatfile is not an ECAT file." << endl;
						bResult = false;
					}
				}
			}
			break;
			default:
			{
				cout << "ERROR: unknown copy option." << endl;
				bResult = false;
			}
		}
	}
	return bResult;
}

bool processCommando_Join()
{
	bool bResult = true;
	
	// check if all source files are available
	QStringListIterator it(g_sSrcFileNames);
	while(it.hasNext())
	{
		QString srcFileName = it.next();
		QFileInfo fileInfo(srcFileName);
		if(!(fileInfo.exists() && fileInfo.isFile() && fileInfo.isReadable()))
		{
			cout << "ERROR: couldn't read ecatfile'" << srcFileName.toAscii().constData() << "'." << endl;
			bResult = false;
		}
	}

	// check if ecatfile is not existing
	QFileInfo fileInfo(g_sECATFileName);
	if(fileInfo.exists())
	{
		cout << "ERROR: ecatfile already existing'" << g_sECATFileName.toAscii().constData() << "'." << endl;
		bResult = false;
	}

	// check if join index is not unknown
	if(g_joinIndex == UnknownIndex)
	{
		cout << "ERROR: no join index provided." << endl;
		bResult = false;
	}

	if(bResult)
	{
		CECATMainHeader::Type srcFileType = CECATMainHeader::Unknown;
		CECATMainHeader* pSrcMainHeader = NULL;
		// first we determine the type of our source ecat files / therefore we
		// open the first ecat source file
		CECATFile srcEcatFile(g_sSrcFileNames.first());
		if(srcEcatFile.open(QIODevice::ReadOnly) && srcEcatFile.format() != CECATFile::Undefined)
		{
			srcFileType = srcEcatFile.fileType();
			if(srcFileType == CECATMainHeader::Unknown)
			{
				cout << "ERROR: can not determine file type of source files." << endl;
				bResult = false;
			}

			// we copy the mainheader too
			if(!srcEcatFile.readMainHeader(pSrcMainHeader))
			{
				cout << "ERROR: can not read main header of first source file." << endl;
				bResult = false;
			}
			srcEcatFile.close();
		}
		else
		{
			cout << "ERROR: provided source file is not an ecat file." << endl;
			bResult = false;
		}

		if(bResult)
		{
			// know we know the file type of our source files
			// so we can create a new ecat file of this specific type
			CECATFile ecatFile(g_sECATFileName, srcFileType);
			if(ecatFile.open(QIODevice::WriteOnly))
			{
				// now we create an empty main header and fill it with the info
				// of the first source file
				CECATMainHeader* pDestMainHeader = NULL;
				pDestMainHeader = ecatFile.createEmptyMainHeader();
				*pDestMainHeader = *pSrcMainHeader;

				// now we write our main header
				if(ecatFile.writeMainHeader(*pDestMainHeader))
				{
					int iSrcFrame = 1;
					int iSrcPlane = 1;
					int iSrcGate = 1;
					int iSrcBed = 0;
					int iSrcData = 0;
					
					int iDestFrame = 1;
					int iDestPlane = 1;
					int iDestGate = 1;
					int iDestBed = 0;
					int iDestData = 0;

					// now copy first directory entry of source files to
					// our destination ecat file
					it.toFront();
					while(it.hasNext())
					{
						QString sSrcFileName = it.next();
						CECATFile srcEcatFile(sSrcFileName);
						if(srcEcatFile.open(QIODevice::ReadOnly) && ecatFile.format() != CECATFile::Undefined)
						{
							// now check if file format is the same as our destination file format
							if(srcFileType == srcEcatFile.fileType())
							{
								// now copy copy directory entry
								CECATSubHeader* pDestSubHeader = ecatFile.createEmptySubHeader();
								CECATSubHeader* pSrcSubHeader = NULL;
								if(srcEcatFile.readSubHeader(pSrcSubHeader, iSrcFrame, iSrcPlane, iSrcGate, iSrcBed, iSrcData))
								{
									*pDestSubHeader = *pSrcSubHeader;

									QByteArray* pSrcMatrixData = NULL;
									// write subheader and matrix
									if(ecatFile.writeSubHeader(*pDestSubHeader, iDestFrame, iDestPlane, iDestGate, iDestBed, iDestData))
									{
										if(srcEcatFile.readMatrix(pSrcMatrixData, iSrcFrame, iSrcPlane, iSrcGate, iSrcBed, iSrcData))
										{
											if(!ecatFile.writeMatrix(*pSrcMatrixData, iDestFrame, iDestPlane, iDestGate, iDestBed, iDestData))
											{
												cout << "ERROR: can not write destination matrix." << endl;
												bResult = false;
											}
										}
										else
										{
											cout << "ERROR: could not read matrix data of eact source file." << endl;
											bResult = false;
										}
									}
									else
									{
										cout << "ERROR: can not write destination subheader." << endl;
										bResult = false;
									}
								}
								else
								{
									cout << "ERROR: could not read subheader of eact source file." << endl;
									bResult = false;
								}
							}
							else
							{
								cout << "ERROR: source \"" << sSrcFileName.toAscii().data() << "\" does not match destination file format." << endl;
								bResult = false;
							}
							srcEcatFile.close();
						}
						else
						{
							cout << "ERROR: provided source file is not an ecat file." << endl;
							bResult = false;
						}
						switch(g_joinIndex)
						{
							case Frames: iDestFrame++; break;
							case Gates: iDestGate++; break;
							case Beds: iDestBed++; break;
							default:
							{
								cout << "ERROR: join index not supported." << endl;
								bResult = false;
							}
						}
					}
				}
				else
				{
					cout << "ERROR: could not write destination main header." << endl;
					bResult = false;
				}
				ecatFile.close();
			}
			else
			{
				cout << "ERROR: can not open destination ecat file." << endl;
				bResult = false;
			}
		}
	}

	return bResult;
}

bool processCommando_Delete()
{
	bool bResult = true;

	cout << "ERROR: deletion of directory items is not supported by libmedio." << endl;
	bResult = false;

	return bResult;

	// check if we can read/write ecatfile
	QFileInfo fileInfo(g_sECATFileName);
	if(fileInfo.exists())
	{
		cout << "ERROR: can not read/write ecat file'" << g_sECATFileName.toAscii().constData() << "'." << endl;
		bResult = false;
	}

	if(bResult)
	{
		switch(g_DeleteOption)
		{
			case DelItem:
			{
				int iFrame = 1;
				int iPlane = 1;
				int iGate = 1;
				int iData = 0;
				int iBed = 0;

				if(g_sMatrix.isEmpty())
				{
					cout << "ERROR: no matrixID provided."<< endl;
					bResult = false;
				}
				else
				{
					bResult = retrieveMatrixIDValues(g_sMatrix, iFrame, iPlane, iGate, iData, iBed);
				}
				
				if(bResult)
				{
					CECATFile ecatFile(g_sECATFileName);
					if(ecatFile.open(QIODevice::ReadWrite) && ecatFile.format() != CECATFile::Undefined)
					{
						#warning "deleting of directory items is not supported by libmedio"
					}
				}
			}
			break;
			default:
			{
				cout << "ERROR: unsupported delete type." << endl;
				bResult = false;
			}
		}
	}
	return bResult;
}

bool processCommando_New()
{
	bool bResult = true;
	// check if ecatfile is not existing
	QFileInfo fileInfo(g_sECATFileName);
	if(fileInfo.exists())
	{
		cout << "ERROR: ecatfile already existing'" << g_sECATFileName.toAscii().constData() << "'." << endl;
		bResult = false;
	}

	// first we check that we have one and only one source file
	if(g_sSrcFileNames.count() != 1)
	{
		cout << "ERROR: we can copy information only from one ecat file to another." << endl;
		bResult = false;
	}

	// check if we can read the source file
	QFileInfo srcFileInfo(g_sSrcFileNames.first());
	if(!(srcFileInfo.exists() && srcFileInfo.isFile() && srcFileInfo.isReadable()))
	{
		cout << "ERROR: couldn't read ecatfile'" << g_sSrcFileNames.first().toAscii().constData() << "'." << endl;
		bResult = false;
	}

	// retrieve matrixID
	int iFrame = 1;
	int iPlane = 1;
	int iGate = 1;
	int iData = 0;
	int iBed = 0;

	if(g_sMatrix.isEmpty())
	{
		cout << "ERROR: no matrixID provided."<< endl;
		bResult = false;
	}
	else
	{
		bResult = retrieveMatrixIDValues(g_sMatrix, iFrame, iPlane, iGate, iData, iBed);
	}

	if(bResult)
	{
		CECATMainHeader::Type srcFileType = CECATMainHeader::Unknown;
		// first we determine the type of our source ecat files / therefore we
		// open the first ecat source file
		CECATFile srcEcatFile(g_sSrcFileNames.first());
		if(srcEcatFile.open(QIODevice::ReadOnly) && srcEcatFile.format() != CECATFile::Undefined)
		{
			srcFileType = srcEcatFile.fileType();
			if(srcFileType == CECATMainHeader::Unknown)
			{
				cout << "ERROR: can not determine file type of source files." << endl;
				bResult = false;
			}
			srcEcatFile.close();
		}
		else
		{
			cout << "ERROR: provided source file is not an ecat file." << endl;
			bResult = false;
		}

		if(bResult)
		{
			// open ecatfile
			CECATFile ecatFile(g_sECATFileName, srcFileType);
			if(ecatFile.open(QIODevice::WriteOnly))
			{
				// open src ecat file
				CECATFile srcEcatFile(g_sSrcFileNames.first());
				if(srcEcatFile.open(QIODevice::ReadOnly) && ecatFile.format() != CECATFile::Undefined)
				{
					// check if subheader and matrix is in available in source file
					CECATMainHeader* pSrcMainHeader = NULL;
					CECATSubHeader* pSrcSubHeader = NULL;
					QByteArray* pSrcMatrixData = NULL;
					if(srcEcatFile.readMainHeader(pSrcMainHeader))
					{
						if(srcEcatFile.readSubHeader(pSrcSubHeader, iFrame, iPlane, iGate, iBed, iData))
						{
							if(srcEcatFile.readMatrix(pSrcMatrixData, iFrame, iPlane, iGate, iBed, iData))
							{
								CECATMainHeader* pDestMainHeader = NULL;
								pDestMainHeader = ecatFile.createEmptyMainHeader();
								*pDestMainHeader = *pSrcMainHeader;
								if(ecatFile.writeMainHeader(*pDestMainHeader))
								{
									CECATSubHeader* pDestSubHeader = NULL;
									pDestSubHeader = ecatFile.createEmptySubHeader();
									*pDestSubHeader = *pSrcSubHeader;
									if(ecatFile.writeSubHeader(*pDestSubHeader, 1, 1, 1, 0, 0))
									{
										if(!ecatFile.writeMatrix(*pSrcMatrixData, 1, 1, 1, 0, 0))
										{
											cout << "ERROR: can not write destination matrix." << endl;
											bResult = false;
										}
									}
									else
									{
										cout << "ERROR: can not write destination subheader." << endl;
										bResult = false;
									}
								}
								else
								{
									cout << "ERROR: can not write destination mainheader." << endl;
									bResult = false;
								}
							}
							else
							{
								cout << "ERROR: could not read matrix data of eact source file." << endl;
								bResult = false;
							}
						}
						else
						{
							cout << "ERROR: could not read subheader of eact source file." << endl;
							bResult = false;
						}
					}
					else
					{
						cout << "ERROR: could not read mainheader of eact source file." << endl;
						bResult = false;
					}
					srcEcatFile.close();
				}
				else
				{
					cout << "ERROR: provided src file is not an ECAT file." << endl;
					bResult = false;
				}
				ecatFile.close();
			}
			else
			{
				cout << "ERROR: provided ecatfile is not an ECAT file." << endl;
				bResult = false;
			}
		}
	}

	return bResult;
}

bool parseCommandLine(int& argc, char** argv)
{
	bool bResult = true;

	// if the user has specified some commandline options lets process and parse them.
	for(int i=1; i < argc; i++)
	{
		QString option(argv[i]);

		if(option[0] == '-')
		{
			if(i+1 < argc && (argv[i+1][0] != '-' || argv[i+1][0] == '-' && QChar(argv[i+1][1]).isDigit()))
			{
				g_args.insert(option, argv[i+1]);
				i++;
			}
			else
				g_args.insert(option, "");
		}
		else
		{
			g_inputFiles << argv[i];
		}
	}

	// if the user likes have help or versioning info we skip parameter processing
	
	if(g_args.contains("-h") || g_args.contains("-v"))
	{
		bResult = false;
	}
	else
	{
		// first we check if there is one and only one input file
		if(g_inputFiles.isEmpty())
		{
			cout << "ERROR: no input file specified." << endl;
			bResult = false;
		}
		else if(g_inputFiles.count() > 1)
		{
			cout << "ERROR: only one input file is allowed." << endl;
			bResult = false;
		}

		// now we check if there is one and only one command specified
		int iNumCommands = 0;
		if(g_args.contains("-g"))
		{
			g_commando = Get;
			iNumCommands++;
		}
		if(g_args.contains("-s"))
		{
			g_commando = Set;
			iNumCommands++;
		}
		if(g_args.contains("-c"))
		{
			g_commando = Copy;
			iNumCommands++;
		}
		if(g_args.contains("-j"))
		{
			g_commando = Join;
			iNumCommands ++;
		}
		if(g_args.contains("-d"))
		{
			g_commando = Delete;
			iNumCommands ++;
		}
		if(g_args.contains("-n"))
		{
			g_commando = New;
			iNumCommands ++;
		}

		if(iNumCommands == 0)
		{
			cout << "ERROR: no command found." << endl;
			bResult = false;
		}
		else if(iNumCommands > 1)
		{
			cout << "ERROR: can only process one command." << endl;
			bResult = false;
		}

		if(!g_inputFiles.isEmpty())
			g_sECATFileName = g_inputFiles.first();
		
		if(g_args.contains("-sf"))
		{
			if(!g_args.value("-sf").isEmpty())
				g_sSrcFileNames = g_args.value("-sf").split(",");
			else
			{
				cout << "ERROR: option \"-sf\" needs a value." << endl;
				bResult = false;
			}
		}

		if(g_args.contains("-mhe"))
		{
			if(!g_args.value("-mhe").isEmpty())
				g_sMainHeaderEntry = g_args.value("-mhe");
			else
			{
				cout << "ERROR: option \"-mhe\" needs a value." << endl;
				bResult = false;
			}
		}

		if(g_args.contains("-she"))
		{
			if(!g_args.value("-she").isEmpty())
				g_sSubHeaderEntry = g_args.value("-she");
			else
			{
				cout << "ERROR: option \"-she\" needs a value." << endl;
				bResult = false;
			}
		}

		if(g_args.contains("-m"))
		{
			if(!g_args.value("-m").isEmpty())
				g_sMatrix = g_args.value("-m");
			else
			{
				cout << "ERROR: option \"-m\" needs a value." << endl;
				bResult = false;
			}
		}

		if(g_args.contains("-va"))
		{
			if(!g_args.value("-va").isEmpty())
				g_sValue = g_args.value("-va");
			else
			{
				cout << "ERROR: option \"-va\" needs a value." << endl;
				bResult = false;
			}
		}

		if(g_args.contains("-po"))
		{
			if(!g_args.value("-po").isEmpty())
				g_sPosition = g_args.value("-po");
			else
			{
				cout << "ERROR: option \"-po\" needs a value." << endl;
				bResult = false;
			}
		}

		if(g_args.contains("-mh") && g_args.contains("-i"))
		{
			cout << "ERROR: can only copy mainheader or item." << endl;
			bResult = false;
		}

		if(g_args.contains("-mh"))
		{
			g_CopyOption = CopyMainHeader;
		}

		if(g_args.contains("-i"))
		{
			g_CopyOption = CopyItem;
			g_DeleteOption = DelItem;
		}

		if(g_args.contains("-dm"))
		{
			if(!g_args.value("-dm").isEmpty())
				g_sDestMatrix = g_args.value("-dm");
			else
			{
				cout << "ERROR: option \"-dm\" needs a value." << endl;
				bResult = false;
			}
		}

		if(g_args.contains("-as"))
		{
			if(g_args.value("-as").toLower() == "frames")
				g_joinIndex = Frames;
			else if(g_args.value("-as").toLower() == "gates")
				g_joinIndex = Gates;
			else if(g_args.value("-as").toLower() == "beds")
				g_joinIndex = Beds;
		}

		if(g_args.contains("-showMainHeaderEntries") && g_args.contains("-showSubHeaderEntries"))
		{
			cout << "ERROR: can only show supported main- or subheaderentries." << endl;
			bResult = false;
		}
		else
		{
			if(g_args.contains("-showMainHeaderEntries"))
			{
				g_eShowEntryInfo = ShowMainHeaderEntries;
				bResult = false;
			}

			if(g_args.contains("-showSubHeaderEntries"))
			{
				g_eShowEntryInfo = ShowSubHeaderEntries;
				bResult = false;
			}
		}
	}

	if(!bResult)
	{
		if(g_args.contains("-v"))
			showVersionInformation();
		else
		{
			switch(g_eShowEntryInfo)
			{
				case ShowMainHeaderEntries: showMainHeaderEntries(); break;
				case ShowSubHeaderEntries: showSubHeaderEntries(); break;
				case UnknownEntries: showHelp(argc, argv); break;
				default: showHelp(argc, argv);
			}
		}
	}
	return bResult;
}

void showVersionInformation()
{
	cout << "Detailed compilation information:" << endl << endl

			 // Compiler information
			 << "  "
			 #if defined(__GNUC__)
			 << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ <<  "." << __GNUC_PATCHLEVEL__ << " "
			 #else
			 #warning unknown compiler suite
			 << "unknown compiler "
			 #endif
			 #if #cpu(sparc)
			 << "[sparc]"
			 #elif #cpu(sparc64)
			 << "[sparc64]"
			 #elif #cpu(powerpc)
			 << "[ppc]"
			 #elif #cpu(i386)
			 << "[x86]"
			 #elif #cpu(amd64)
			 << "[amd64]"
			 #else
			 #warning Unknown CPU model
			 << "[Unknown]"
			 #endif
			 << endl << endl

			 // Qt version information
			 << "  Qt " << qVersion() << endl
									<< "  Copyright (c) 2006-2007 Trolltech Inc." << endl << endl

			 << "  libmedio " << CMedIO::version().toAscii().constData() <<  " ("
												<< CMedIO::buildDate().toAscii().constData() << ")" << endl
												<< "  " << CMedIO::copyright().toAscii().constData() << endl;
}

void showHelp(int& argc, char** argv)
{
	cout << endl;
	cout << "libmedio ECAT6/7 file utility v2.5" << endl;
	cout << "----------------------------------" << endl;
	cout << "Usage: " << argv[0] << " <options> ecatfile" << endl;
	cout << "Options:" << endl;
	cout << "  -g            : get any header entry of an ecatfile." << endl;
	cout << "  -s            : set any header entry in an ecatfile." << endl;
	cout << "  -c            : copy information from an ecat source file to an ecatfile." << endl;
	cout << "  -j            : join multiple ecatfiles to one ecatfile." << endl;
	cout << "  -d            : delete information from an ecat file." << endl;
	cout << "  -n            : create new items in ecatfile copied from ecat source file." << endl;
	cout << "  -mhe <entry>  : specifies an entry in the mainheader" << endl;
	cout << "                : use \"-showMainHeaderEntries\" to view supported entries." << endl;
	cout << "  -she          : specifies an entry in any subheader" << endl;
	cout << "                : use \"-showSubHeaderEntries\" to view supported entries." << endl;
	cout << "  -m <matrixID> : specific matrix in an ecatfile with ID <frame,plane,gate,data,bed>." << endl;
	cout << "  -va <value>   : sets specific value." << endl;
	cout << "  -po <position>: index position in header entry [0,1,...,N]." << endl;
	cout << "  -mh           : flag which enables copy of mainheader." << endl;
	cout << "  -i            : flag which enables copy/delete of directory item." << endl;
	cout << "  -sf <file1,file2,...,fileN>" << endl;
	cout << "                : specifies source file(s)." << endl;
	cout << "  -dm <matrixID>: specific matrix in an ecatfile with ID <frame,plane,gate,data,bed>." << endl;
	cout << "  -as <index>   : specifies join index in new ecatfile {frames, gates, beds}." << endl;
	cout << "  -v            : show version information." << endl;
	cout << "  -h            : this help page." << endl << endl;

	cout << "Examples:" << endl << endl;
	cout << "   ecatutil -g[--get] -mhe[--mainheaderentry] \"entry\" -po[--position] \"pos\" <ecatfile>" << endl;
	cout << "   ecatutil -g[--get] -she[--subheaderentry] \"entry\" -m[--matrix] \"f,p,g,d,b\" -po[--position] \"pos\" <ecatfile>" << endl;
	cout << endl;
	cout << "   ecatutil -s[--set] -mhe[--mainheaderentry] \"entry\" -va[--value] \"value\" -po[--position] \"pos\" <ecatfile>" << endl;
	cout << "   ecatutil -s[--set] -she[--subheaderentry] \"entry\" -m[--matrix] \"f,p,g,d,b\" -va[--value] \"value\" -po[--position] \"pos\" <ecatfile>" << endl;
	cout << endl;
	cout << "   ecatutil -c[--copy] -mh[--mainheader] -sf[--srcfile] \"ecatfile\" <ecatfile>" << endl;
	cout << "   ecatutil -c[--copy] -i[--item] -m[--matrix] \"f,p,g,d,b\" -sf[--srcfile] \"ecatfile\" -dm[--destinationmatrix] \"f,p,g,d,b\" <ecatfile>" << endl;
	cout << endl;
	cout << "   ecatutil -j[--join] -sf[--srcfile] \"file1,file2,...,fileN\" -as {\"frames\"|\"gates\"|\"beds\"} <ecatfile>" << endl;
	cout << endl;
	cout << "   ecatutil -d[--delete] -i[--item] -m[--matrix] \"f,p,g,d,b\" <ecatfile>" << endl;
	cout << endl;
	cout << "   ecatutil -n[--new] -sf[--srcfile] -m[--matrix] \"f,p,g,d,b\" <ecatfile>" << endl;
	cout << endl;
	cout << "   ecatutil -v[--version]" << endl;
	cout << "   ecatutil -h[--help]" << endl;
	cout << "   ecatutil -showMainHeaderEntries" << endl;
	cout << "   ecatutil -showSubHeaderEntries" << endl;
}

void showMainHeaderEntries()
{
	QList<QString> keys = ecat7MainHeaderMap.uniqueKeys();
	QListIterator<QString> it(keys);
	cout << endl;
	cout << "Supported MainHeaderEntries:" << endl;
	cout << "----------------------------------" << endl;
	while(it.hasNext())
		cout << it.next().toAscii().data() << endl;
	cout << endl;
}

void showSubHeaderEntries()
{
	QList<QString> imageKeys = ecat7ImageHeaderMap.uniqueKeys();
	QListIterator<QString> imageIt(imageKeys);
	cout << endl;
	cout << "Supported ImageHeaderEntries:" << endl;
	cout << "----------------------------------" << endl;
	while(imageIt.hasNext())
		cout << imageIt.next().toAscii().data() << endl;
	cout << endl;

	QList<QString> scan3DKeys = ecat7Scan3DHeaderMap.uniqueKeys();
	QListIterator<QString> scan3DIt(scan3DKeys);
	cout << endl;
	cout << "Supported Scan3DHeaderEntries:" << endl;
	cout << "----------------------------------" << endl;
	while(scan3DIt.hasNext())
		cout << scan3DIt.next().toAscii().data() << endl;
	cout << endl;
}

bool initHeaderMaps()
{
	bool bResult = true;
	ecat7MainHeaderMap.insert("filetype", FILE_TYPE);
	ecat7MainHeaderMap.insert("originalfilename", ORIGINAL_FILE_NAME);
	ecat7MainHeaderMap.insert("angularcompression", ANGULAR_COMPRESSION);
	ecat7MainHeaderMap.insert("calibrationunitslabel", CALIBRATION_UNITS_LABEL);
	ecat7MainHeaderMap.insert("dataunits", DATA_UNITS);
	ecat7MainHeaderMap.insert("dosestarttime", DOSE_START_TIME);
	ecat7MainHeaderMap.insert("scanstarttime", SCAN_START_TIME);
	ecat7MainHeaderMap.insert("bedelevation", BED_ELEVATION);
	ecat7MainHeaderMap.insert("bedposition", BED_POSITION);
	ecat7MainHeaderMap.insert("acquisitiontype", ACQUISITION_TYPE);
	ecat7MainHeaderMap.insert("patientorientation", PATIENT_ORIENTATION);
	ecat7MainHeaderMap.insert("isotopename", ISOTOPE_NAME);
	ecat7MainHeaderMap.insert("isotopehalflife", ISOTOPE_HALFLIFE);
	ecat7MainHeaderMap.insert("radiopharmaceutical", RADIOPHARMACEUTICAL);
	ecat7MainHeaderMap.insert("calibrationfactor", ECAT_CALIBRATION_FACTOR);
	ecat7MainHeaderMap.insert("calibrationunits", CALIBRATION_UNITS);

	ecat7ImageHeaderMap.insert("filtercode", FILTER_CODE);
	ecat7ImageHeaderMap.insert("rfiltercode", RFILTER_CODE);
	ecat7ImageHeaderMap.insert("rfilterresolution", RFILTER_RESOLUTION);
	ecat7ImageHeaderMap.insert("zfiltercode", ZFILTER_CODE);
	ecat7ImageHeaderMap.insert("zfiltercoderesolution", ZFILTER_RESOLUTION);
	ecat7ImageHeaderMap.insert("reconzoom", RECON_ZOOM);
	ecat7ImageHeaderMap.insert("numrelements", NUM_R_ELEMENTS_IMAGE);
	ecat7ImageHeaderMap.insert("numangles", NUM_ANGLES_IMAGE);
	ecat7ImageHeaderMap.insert("processingcode", PROCESSING_CODE);
	ecat7ImageHeaderMap.insert("xpixelsize",X_PIXEL_SIZE);
	ecat7ImageHeaderMap.insert("ypixelsize",Y_PIXEL_SIZE);
	ecat7ImageHeaderMap.insert("zpixelsize",Z_PIXEL_SIZE);

	ecat7Scan3DHeaderMap.insert("framestarttime", FRAME_START_TIME_SCAN3D);
	return bResult;
}

bool retrieveMatrixIDValues(QString sMatrixID, int &iFrame, int &iPlane, int &iGate, int &iData, int &iBed)
{
	bool bResult = true;
	// convert our matrixID to frame, plane, gate, data, bed
	QStringList list = sMatrixID.split(',');
	if(list.count() != 5)
	{
		cout << "ERROR: provided matrixID has wrong format."<< endl;
		bResult = false;
	}
	else
	{
		QStringListIterator it(list);
		int i = 0;
		for(; it.hasNext() && i <= 4; i++)
		{
			bool bSuccess = false;
			short num = it.next().toInt(&bSuccess);
			if(bSuccess == false)
			{
				i=-1;
				break;
			}

			if(i==0)
				iFrame = num;
			else if(i==1)
				iPlane = num;
			else if(i==2)
				iGate = num;
			else if(i==3)
				iData = num;
			else if(i==4)
				iBed = num;
			
		}

		if(i == -1)
		{
			cout << "ERROR: on converting supplied new matrix ID: '" << sMatrixID.toAscii().constData() << "'" << endl;
			bResult = false;
		}
	}
	return bResult;
}

bool retrieveFilterCode(short iFilterCode, CECAT7SubHeaderImage::Filter_Code &filterCode)
{
	bool bResult = true;
	CECAT7SubHeaderImage::Filter_Code fCode = CECAT7SubHeaderImage::NoFilter;
	switch(iFilterCode)
	{
		case 0: fCode = CECAT7SubHeaderImage::NoFilter; break;
		case 1: fCode = CECAT7SubHeaderImage::Ramp; break;
		case 2: fCode = CECAT7SubHeaderImage::Butterfield; break;
		case 3: fCode = CECAT7SubHeaderImage::Hanning; break;
		case 4: fCode = CECAT7SubHeaderImage::Hamming; break;
		case 5: fCode = CECAT7SubHeaderImage::Parzen; break;
		case 6: fCode = CECAT7SubHeaderImage::Shepp; break;
		case 7: fCode = CECAT7SubHeaderImage::ButterWoth2; break;
		case 8: fCode = CECAT7SubHeaderImage::Gaussian; break;
		case 9: fCode = CECAT7SubHeaderImage::Median; break;
		case 10: fCode = CECAT7SubHeaderImage::Boxcar; break;
		default:
		{
			cout << "ERROR: filter code not supported." << endl;
			bResult = false;
		}
	}
	if(bResult)
		filterCode = fCode;
	return bResult;
}
