
#include "otbDEMHandler.h"
#include "otbGDALDriverManagerWrapper.h"
#include "boost/filesystem.hpp"
#include "gdal_utils.h"

//TODO C++ 17 : use std::optional instead
#include <boost/optional.hpp>

namespace otb {

namespace DEMDetails {

// Adapted from boost filesystem documentation : https://www.boost.org/doc/libs/1_53_0/libs/filesystem/doc/reference.html
std::vector<std::string> GetFilesInDirectory(const std::string & directoryPath)
{
  std::vector<std::string> fileList;
  
  if ( !boost::filesystem::exists( directoryPath) )
  {
    return fileList;
  }
  else if (!boost::filesystem::is_directory(directoryPath))
  {
    fileList.push_back(directoryPath);
    return fileList;
  }
  
  boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
  for ( boost::filesystem::directory_iterator itr( directoryPath ); itr != end_itr; ++itr )
  {
    if ( boost::filesystem::is_directory(itr->status()) )
    {
      auto subDirList = GetFilesInDirectory(itr->path().string());
      fileList.insert(fileList.end(), subDirList.begin(), subDirList.end());
    }
    else
    {
      fileList.push_back(itr->path().string());
    }
  }
  return fileList;
}

boost::optional<double> GetDEMValue(double lon, double lat, GDALDataset* ds)
{
  double geoTransform[6];
  
  ds->GetGeoTransform(geoTransform);

  auto x = (lon - geoTransform[0]) / geoTransform[1] - 0.5;
  auto y = (lat - geoTransform[3]) / geoTransform[5] - 0.5;

  if (x < 0 || y < 0 || x > ds->GetRasterXSize() || y > ds->GetRasterYSize())
  {
    return boost::none;
  }

  auto x_int = static_cast<int>(x);
  auto y_int = static_cast<int>(y);

  auto deltaX = x - x_int;
  auto deltaY = y - y_int;

  if (x < 0 || y < 0 || x+1 > ds->GetRasterXSize() || y+1 > ds->GetRasterYSize())
  {
    return boost::none;
  }
  
  // Bilinear interpolation.
  double elevData[4];
  
  auto err = ds->GetRasterBand(1)->RasterIO( GF_Read, x_int, y_int, 2, 2,
              elevData, 2, 2, GDT_Float64,
              0, 0, nullptr);

  if (err)
  {
    return boost::none;
  }

  // Test for no data. Don't return a value if one pixel 
  // of the interpolation is no data.
  for (int i =0; i<4; i++)
  {
    if (elevData[i] == ds->GetRasterBand(1)->GetNoDataValue())
    {
      return boost::none;
    }
  }

  auto xBil1 = elevData[0] * (1-deltaX) + elevData[1] * deltaX;
  auto xBil2 = elevData[2] * (1-deltaX) + elevData[3] * deltaX;

  auto yBil = xBil1 * (1.0 - deltaY) + xBil2 * deltaY;
  return yBil;
}

}  // namespace DEMDetails

/** Initialize the singleton */
DEMHandler::Pointer DEMHandler::m_Singleton = nullptr;

DEMHandler::Pointer DEMHandler::Instance()
{
  if (m_Singleton.GetPointer() == nullptr)
  {
    m_Singleton = itk::ObjectFactory<Self>::Create();

    if (m_Singleton.GetPointer() == nullptr)
    {
      m_Singleton = new DEMHandler;
    }
    m_Singleton->UnRegister();
  }

  return m_Singleton;
}


DEMHandler::DEMHandler() : m_Dataset(nullptr),
                           m_GeoidDS(nullptr),
                           m_DefaultHeightAboveEllipsoid(0.0)
{
  GDALAllRegister();
};

DEMHandler::~DEMHandler()
{
  GDALClose(m_GeoidDS);
  ClearDEMs();
}

void DEMHandler::OpenDEMFile(const std::string& path)
{
  m_DatasetList.push_back(otb::GDALDriverManagerWrapper::GetInstance().Open(path));
  m_Dataset = m_DatasetList.back()->GetDataSet();
  m_DEMDirectories.push_back(path);
}

void DEMHandler::OpenDEMDirectory(const std::string& DEMDirectory)
{
  auto demFiles = DEMDetails::GetFilesInDirectory(DEMDirectory);
  for (const auto & file : demFiles)
  {
    auto ds = otb::GDALDriverManagerWrapper::GetInstance().Open(file);
    if (ds)
    {  
      m_DatasetList.push_back(ds );
    }
  }
  
  int vrtSize = m_DatasetList.size();
 
  // Don't build a vrt if there is less than two dataset
  if (m_DatasetList.empty())
  {
    m_Dataset = nullptr;
  }
  else
  {
    if (vrtSize == 1)
    {
      m_Dataset = m_DatasetList[0]->GetDataSet();
      m_DEMDirectories.push_back(DEMDirectory);
    }
    else
    {
      auto vrtDatasetList = new GDALDatasetH[vrtSize];
      for (int i = 0; i < vrtSize; i++)
      {
        vrtDatasetList[i] = m_DatasetList[i]->GetDataSet();
      }

      int error = 0;

      m_Dataset = (GDALDataset *) GDALBuildVRT(nullptr, vrtSize, vrtDatasetList, 
                                                nullptr, nullptr, &error);
      m_DEMDirectories.push_back(DEMDirectory);
      delete[] vrtDatasetList;
    }
    
  }
}

bool DEMHandler::OpenGeoidFile(const std::string& geoidFile)
{  
  if (m_GeoidDS)
  {
    GDALClose(m_GeoidDS);
  }

  int pbError;

  m_GeoidDS = static_cast<GDALDataset*>(GDALOpenVerticalShiftGrid(geoidFile.c_str(), &pbError));
  m_GeoidFilename = geoidFile;

  return pbError;
}

double DEMHandler::GetHeightAboveEllipsoid(double lon, double lat)
{
  double result = 0.;
  boost::optional<double> DEMresult;
  boost::optional<double> geoidResult;

  if (m_Dataset)
  {
    DEMresult = DEMDetails::GetDEMValue(lon, lat, m_Dataset);
    if (DEMresult)
    {
      result += *DEMresult;
    }
  }


  if (m_GeoidDS)
  {
    geoidResult = DEMDetails::GetDEMValue(lon, lat, m_GeoidDS);
    if (geoidResult)
    {
      result += *geoidResult;
    }
  }

  if (DEMresult || geoidResult)
    return result;
  else
    return m_DefaultHeightAboveEllipsoid;
}

double DEMHandler::GetHeightAboveEllipsoid(const PointType& geoPoint)
{
  return GetHeightAboveEllipsoid(geoPoint[0], geoPoint[1]);
}

double DEMHandler::GetHeightAboveMSL(double lon, double lat)
{
  if (m_Dataset)
  { 
    auto result = DEMDetails::GetDEMValue(lon, lat, m_Dataset);
    
    if (result)
    {
      return *result;
    }
  }

  return 0;
}

double DEMHandler::GetHeightAboveMSL(const PointType& geoPoint)
{
  return GetHeightAboveMSL(geoPoint[0], geoPoint[1]);
}

unsigned int DEMHandler::GetDEMCount() const
{
  return m_DatasetList.size();
}

bool DEMHandler::IsValidDEMDirectory(const std::string& DEMDirectory)
{
  for (const auto & filename : DEMDetails::GetFilesInDirectory(DEMDirectory))
  {
    // test if a driver can identify this dataset
    auto identifyDriverH = static_cast<GDALDriver *>(GDALIdentifyDriver(filename.c_str(), nullptr));
    if (identifyDriverH)
    {
      return true;
    }
  }

  return false;
}

std::string DEMHandler::GetDEMDirectory(unsigned int idx) const
{
  if (idx >= m_DEMDirectories.size())
  {
    return "";
  }
  else
  {
    return m_DEMDirectories[idx];
  }
}

std::string DEMHandler::GetGeoidFile() const
{
  return m_GeoidFilename;
}

void DEMHandler::ClearDEMs()
{
  m_DEMDirectories.clear();

  // This will call GDALClose on all datasets
  m_DatasetList.clear();
}

}