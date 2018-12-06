/*
 * Copyright (C) 2005-2017 Centre National d'Etudes Spatiales (CNES)
 *
 * This file is part of Orfeo Toolbox
 *
 *     https://www.orfeo-toolbox.org/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef otbSarBurstExtractionImageFilter_hxx
#define otbSarBurstExtractionImageFilter_hxx

#include "otbSarBurstExtractionImageFilter.h"

#include "otbSarSensorModelAdapter.h"
#include "otbImageKeywordlist.h"
#include "itkImageScanlineIterator.h"
#include "itkImageScanlineConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"

namespace otb
{
// Constructor
template <class TImage> SarBurstExtractionImageFilter<TImage>::SarBurstExtractionImageFilter()
  : m_LinesRecord(), m_SamplesRecord(), m_BurstIndex(0)
{}

// Needs to be re-implemented since size of output is modified
template< class TImage>
void
SarBurstExtractionImageFilter<TImage>::GenerateOutputInformation()
{
  // Call superclass implementation
  Superclass::GenerateOutputInformation();

  // Retrieve the input image pointer
  const ImageType * inputPtr = this->GetInput();
  ImageType * outputPtr = this->GetOutput();

  // Check that azimuth spacing has not been modified
  if(std::abs(inputPtr->GetSignedSpacing()[1]-1.)>=std::numeric_limits<double>::epsilon())
    itkExceptionMacro("Can not perform deburst if input image azimuth spacing is not 1.");
  
  // Check that the azimuth sampling grid has not been modified
  if(std::abs(inputPtr->GetOrigin()[1]-static_cast<long>(inputPtr->GetOrigin()[1])-0.5)>=std::numeric_limits<double>::epsilon())
    itkExceptionMacro("Can not perform burst extraction if input image azimuth origin is not N.5");
  
  // Retrieve input image keywordlist
  ImageKeywordlist inputKwl = inputPtr->GetImageKeywordlist();
  
  // Try to create a SarSensorModelAdapter
  SarSensorModelAdapter::Pointer sarSensorModel = SarSensorModelAdapter::New();
  bool loadOk = sarSensorModel->LoadState(inputKwl);

  if(!loadOk || !sarSensorModel->IsValidSensorModel())
    itkExceptionMacro(<<"Input image does not contain a valid SAR sensor model.");
  
  // Try to call the burst extraction function
  bool burstExtractionOk = sarSensorModel->BurstExtraction(m_BurstIndex, m_LinesRecord, m_SamplesRecord);

  if(!burstExtractionOk)
    itkExceptionMacro(<<"Could not etract Burst from input image");
  
  // Compute the actual lines to remove
  typename ImageType::RegionType largestPossibleRegion = this->GetInput()->GetLargestPossibleRegion();
  typename ImageType::PointType origin = this->GetInput()->GetOrigin();

  // Export the new keywordlist
  ImageKeywordlist newKwl;

  bool saveOk = sarSensorModel->SaveState(newKwl);

  if(!saveOk)
    itkExceptionMacro(<<"Could not export deburst SAR sensor model to keyword list");

  outputPtr->SetOrigin(origin);
  
  // Compute deburst azimuth size
  typename ImageType::SizeType burstSize = largestPossibleRegion.GetSize();
  
  burstSize[0] = m_SamplesRecord.second - m_SamplesRecord.first + 1;
  burstSize[1] = m_LinesRecord.second - m_LinesRecord.first + 1;
      
  
  // Set largest possible region
  typename ImageType::RegionType outputLargestPossibleRegion = largestPossibleRegion;
  largestPossibleRegion.SetSize(burstSize);
  outputPtr->SetLargestPossibleRegion(largestPossibleRegion);

  newKwl.AddKey("support_data_number_samples", std::to_string(burstSize[0]));
  newKwl.AddKey("support_data_number_lines", std::to_string(burstSize[1]));

  newKwl.AddKey("number_samples", std::to_string(burstSize[0]));
  newKwl.AddKey("number_lines", std::to_string(burstSize[1]));
  
  // Set new keyword list to output image
  outputPtr->SetImageKeywordList(newKwl);
}

template<class TImage>
typename SarBurstExtractionImageFilter<TImage>::RegionType
SarBurstExtractionImageFilter<TImage>::OutputRegionToInputRegion(const RegionType& outputRegion) const
{
  RegionType inputRegion = outputRegion;

  typename RegionType::IndexType index = inputRegion.GetIndex();
  
  index[0] += m_SamplesRecord.first;
  index[1] += m_LinesRecord.first;
  
  inputRegion.SetIndex(index);
  
  return inputRegion;
}

  
// Needs to be re-implemented since size of output is modified
template <class TImage> void SarBurstExtractionImageFilter<TImage>::GenerateInputRequestedRegion()
{
  RegionType outputRequestedRegion = this->GetOutput()->GetRequestedRegion();
  RegionType inputRequestedRegion = OutputRegionToInputRegion(outputRequestedRegion);

  ImageType * inputPtr = const_cast<ImageType *>(this->GetInput());
  
  inputPtr->SetRequestedRegion(inputRequestedRegion);
}


// Actual processing
template <class TImage> 
void 
SarBurstExtractionImageFilter<TImage>::ThreadedGenerateData(const RegionType& outputRegionForThread, 
						    itk::ThreadIdType itkNotUsed(threadId))
{
  // Compute corresponding input region
  RegionType inputRegionForThread = OutputRegionToInputRegion(outputRegionForThread);
  
  itk::ImageRegionConstIterator<ImageType> inputIt(this->GetInput(),inputRegionForThread);
  itk::ImageRegionIterator<ImageType> outputIt(this->GetOutput(),outputRegionForThread);

  inputIt.GoToBegin();
  outputIt.GoToBegin();

  int linesRecordFirst = static_cast<int>(m_LinesRecord.first);
  int linesRecordSecond = static_cast<int>(m_LinesRecord.second);
  int samplesRecordFirst = static_cast<int>(m_SamplesRecord.first);
  int samplesRecordSecond = static_cast<int>(m_SamplesRecord.second);

  while(!inputIt.IsAtEnd()&&!outputIt.IsAtEnd())
    {
      typename ImageType::IndexType currentInputIndex = inputIt.GetIndex();
     
      bool lineToKeep = false;
      bool sampleToKeep = false;

      // Check lines
      if (currentInputIndex[1] >= linesRecordFirst && 
	  currentInputIndex[1] <= linesRecordSecond)
	{
	  lineToKeep = true;
	}

      // Check samples
      if (currentInputIndex[0] >= samplesRecordFirst && 
	  currentInputIndex[0] <= samplesRecordSecond)
	{
	  sampleToKeep = true;
	} 

      // If ok, copy input pixel into output image
      if(lineToKeep && sampleToKeep)
	{
	  outputIt.Set(inputIt.Get());

	  ++outputIt;
	}

      ++inputIt;
    }  
}

} // End namespace otb

#endif
