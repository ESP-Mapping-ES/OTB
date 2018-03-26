
#ifndef PersistentLabelImageVectorizationFilter_txx
#define PersistentLabelImageVectorizationFilter_txx

#include "itkDefaultConvertPixelTraits.h"
#include "itkProgressReporter.h"

#include "otbOGRFieldWrapper.h"

#include "PersistentLabelImageVectorizationFilter.h"

// --------- otb::PersistentLabelImageVectorizationFilter ---------------------

namespace otb
{
template<class TInputImage>
PersistentLabelImageVectorizationFilter<TInputImage>
::PersistentLabelImageVectorizationFilter()
{
  this->SetNumberOfRequiredOutputs(2);
  //this->SetNthOutput(0,TInputImage::New());
  m_TileNumber = 1;
  m_Tol = 0;
}

template<class TInputImage>
void
PersistentLabelImageVectorizationFilter<TInputImage>
::Reset(void)
{
}

template<class TInputImage>
void
PersistentLabelImageVectorizationFilter<TInputImage>
::GenerateOutputInformation()
{
  Superclass::GenerateOutputInformation();
}

template<class TInputImage>
void
PersistentLabelImageVectorizationFilter<TInputImage>
::GenerateInputRequestedRegion()
{
  Superclass::GenerateInputRequestedRegion();
}

  
template<class TInputImage>
typename PersistentLabelImageVectorizationFilter<TInputImage>::OGRDataSourcePointerType
PersistentLabelImageVectorizationFilter<TInputImage>
::ProcessTile()
{  
  ++m_TileNumber;
  // Apply an ExtractImageFilter to avoid problems with filters asking for the LargestPossibleRegion
  typedef itk::ExtractImageFilter<InputImageType, InputImageType> ExtractImageFilterType;
  typename ExtractImageFilterType::Pointer extract = ExtractImageFilterType::New();
  extract->SetInput( this->GetInput() );
  
  SizeType paddedRegionSize = this->GetInput()->GetRequestedRegion().GetSize();
  // Expend input region to avoid touching but not overlapping polygons during fusion
  paddedRegionSize[0] += 1;
  paddedRegionSize[1] += 1;
  
  RegionType paddedRegion(  this->GetInput()->GetRequestedRegion().GetIndex() , paddedRegionSize );
  paddedRegion.Crop(this->GetInput()->GetLargestPossibleRegion());
  
  extract->SetExtractionRegion( paddedRegion );
  extract->Update();



  // WARNING: itk::ExtractImageFilter does not copy the MetadataDictionary
  extract->GetOutput()->SetMetaDataDictionary(this->GetInput()->GetMetaDataDictionary());

  typename LabelImageToOGRDataSourceFilterType::Pointer labelImageToOGRDataFilter =
                                              LabelImageToOGRDataSourceFilterType::New();
  labelImageToOGRDataFilter->SetInput(extract->GetOutput());
  //labelImageToOGRDataFilter->SetInput(this->GetInput());
  
  labelImageToOGRDataFilter->UpdateLargestPossibleRegion();
 
  labelImageToOGRDataFilter->SetFieldName(m_FieldName);
  labelImageToOGRDataFilter->SetUse8Connected(m_Use8Connected);
  
  labelImageToOGRDataFilter->Update();
  OGRDataSourcePointerType tmpDS = const_cast<OGRDataSourceType *>(labelImageToOGRDataFilter->GetOutput());
  OGRLayerType tmpLayer = tmpDS->GetLayerChecked(0);
  
  OGRDataSourcePointerType outDS = OGRDataSourceType::New();
  OGRLayerType outLayer = outDS->CreateLayer("Layer");
  OGRFieldDefn field(m_FieldName.c_str(),OFTInteger);
  outLayer.CreateField(field, true);
  OGRFieldDefn fieldSize("size",OFTReal);
  outLayer.CreateField(fieldSize, true);
  OGRFieldDefn fieldPerimeter("perimeter",OFTReal);
  outLayer.CreateField(fieldPerimeter, true);
  
  //typename OGRLayerType::const_iterator featIt = tmpLayer.begin();
  typename OGRLayerType::iterator featIt = tmpLayer.begin();
  for(; featIt!=tmpLayer.end(); ++featIt)
  {
    OGRFeatureType outFeature(outLayer.GetLayerDefn());

    if ( (std::find(m_Labels.begin(),m_Labels.end(),(*featIt)[m_FieldName].ogr::Field::GetValue<int>()) != m_Labels.end() ) || (m_Labels.empty() ==true))
    {
      //simplify
      const OGRGeometry * geom = (*featIt).GetGeometry();
      assert(geom && "geometry is NULL ! Can't simplify it.");
      (*featIt).ogr::Feature::SetGeometryDirectly(ogr::Simplify(*geom,m_Tol));
     
      outFeature.SetFrom( *featIt, TRUE );
      outLayer.CreateFeature( outFeature );
    }
    else
    {
    }
  }
  return outDS;
}



} // End namespace otb

#endif
