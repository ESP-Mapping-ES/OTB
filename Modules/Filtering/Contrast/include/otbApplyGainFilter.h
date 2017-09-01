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

#ifndef otbApplyGainFilter_h
#define otbApplyGainFilter_h

#include "itkImageToImageFilter.h"
#include "otbImage.h"

namespace otb
{

template < class TInputImage , class TLut , class TOutputImage >
class ITK_EXPORT ApplyGainFilter :
  public itk::ImageToImageFilter< TInputImage , TOutputImage >
{
public :
  typedef TInputImage InputImageType;
  typedef TOutputImage OutputImageType;
  typedef TLut LutType;

  /** typedef for standard classes. */
  typedef ApplyGainFilter Self;
  typedef itk::ImageToImageFilter< InputImageType, OutputImageType > Superclass;
  typedef itk::SmartPointer< Self > Pointer;
  typedef itk::SmartPointer< const Self > ConstPointer;

  typedef typename InputImageType::InternalPixelType InputPixelType;
  typedef typename OutputImageType::InternalPixelType OutputPixelType;

  typedef typename OutputImageType::RegionType OutputImageRegionType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);
  /** Run-time type information (and related methods). */
  itkTypeMacro(ComputeHistoFilter, ImageToImageFilter);

  itkSetMacro(NoData, InputPixelType);
  itkGetMacro(NoData, InputPixelType);

  itkSetMacro(Min, InputPixelType);
  itkGetMacro(Min, InputPixelType);

  itkSetMacro(Max, InputPixelType);
  itkGetMacro(Max, InputPixelType);

  void SetInputLut( const LutType * lut) ;

  void SetInputImage( const InputImageType * input) ;

protected :
  ApplyGainFilter();
  ~ApplyGainFilter() ITK_OVERRIDE {}
  
  const InputImageType * GetInputImage() const;

  
  const LutType * GetInputLut() const;

  void GenerateInputRequestedRegion();

  void BeforeThreadedGenerateData();

  void ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
                            itk::ThreadIdType threadId);

  float InterpoleGain( typename LutType::ConstPointer gridLut ,
                       int pixelValue , 
                       typename InputImageType::IndexType index);

private :
  ApplyGainFilter(const Self &); //purposely not implemented
  void operator =(const Self&); //purposely not implemented

  InputPixelType m_NoData;
  InputPixelType m_Min;
  InputPixelType m_Max;
  double m_Step;
  typename LutType::SizeType m_LutSize;
  typename InputImageType::SizeType m_ThumbSize;


};

}  // End namespace otb
  
#ifndef OTB_MANUAL_INSTANTIATION
#include "otbApplyGainFilter.txx"
#endif


#endif