/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkCastImageFilter_hxx
#define itkCastImageFilter_hxx

#if ITK_VERSION_MAJOR >= 5

#include "otbCastImageFilter.h"
#include "itkProgressReporter.h"
#include "itkImageAlgorithm.h"

namespace otb
{


template< typename TInputImage, typename TOutputImage >
CastImageFilter< TInputImage, TOutputImage >
::CastImageFilter()
{
  this->SetNumberOfRequiredInputs(1);
  this->InPlaceOff();
  this->DynamicMultiThreadingOn();
}

template< typename TInputImage, typename TOutputImage >
void
CastImageFilter< TInputImage, TOutputImage >
::GenerateData()
{
  if ( this->GetInPlace() && this->CanRunInPlace() )
    {
    // nothing to do, so avoid iterating over all the pixels
    // for nothing! Allocate the output, generate a fake progress and exit
    this->AllocateOutputs();
    itk::ProgressReporter progress(this, 0, 1);
    return;
    }
  //else do normal Before+Threaded+After
  Superclass::GenerateData();
}


template< typename TInputImage, typename TOutputImage >
void
CastImageFilter< TInputImage, TOutputImage >
::GenerateOutputInformation()
  {
    // do not call the superclass' implementation of this method since
    // this filter allows the input the output to be of different dimensions

    // get pointers to the input and output
    TOutputImage *outputPtr = this->GetOutput();
    const TInputImage *inputPtr  = this->GetInput();

    if ( !outputPtr || !inputPtr )
      {
      return;
      }

    // Set the output image largest possible region.  Use a RegionCopier
    // so that the input and output images can be different dimensions.
    OutputImageRegionType outputLargestPossibleRegion;
    this->CallCopyInputRegionToOutputRegion( outputLargestPossibleRegion,
                                             inputPtr->GetLargestPossibleRegion() );
    outputPtr->SetLargestPossibleRegion(outputLargestPossibleRegion);

    itk::ImageToImageFilterDetail::ImageInformationCopier<Superclass::OutputImageDimension,
                                                     Superclass::InputImageDimension>
      informationCopier;
    informationCopier(outputPtr, inputPtr);

  }


template< typename TInputImage, typename TOutputImage >
void
CastImageFilter< TInputImage, TOutputImage >
::DynamicThreadedGenerateData(const OutputImageRegionType & outputRegionForThread)
{
  DynamicThreadedGenerateDataDispatched<InputPixelType>(outputRegionForThread,
                                               std::is_convertible<InputPixelType, OutputPixelType>());
}

template< typename TInputImage, typename TOutputImage >
template<typename TInputPixelType>
void
CastImageFilter< TInputImage, TOutputImage >
::DynamicThreadedGenerateDataDispatched(const OutputImageRegionType & outputRegionForThread, std::true_type)
{
  const TInputImage *inputPtr = this->GetInput();
  TOutputImage *outputPtr = this->GetOutput(0);

  // Define the portion of the input to walk for this thread, using
  // the CallCopyOutputRegionToInputRegion method allows for the input
  // and output images to be different dimensions
  typename TInputImage::RegionType inputRegionForThread;

  this->CallCopyOutputRegionToInputRegion(inputRegionForThread, outputRegionForThread);

  itk::ImageAlgorithm::Copy( inputPtr, outputPtr, inputRegionForThread, outputRegionForThread );
}


template< typename TInputImage, typename TOutputImage >
template<typename TInputPixelType>
void
CastImageFilter< TInputImage, TOutputImage >
::DynamicThreadedGenerateDataDispatched(const OutputImageRegionType & outputRegionForThread, std::false_type)
{
  // Implementation for non-implicit convertible pixels which are
  // itk-array-like.

  //static_assert( OutputPixelType::Dimension == InputPixelType::Dimension, "Vector dimensions are required to match!" );
  static_assert( std::is_convertible<typename InputPixelType::ValueType, typename OutputPixelType::ValueType>::value, "Component types are required to be convertible." );

  const typename OutputImageRegionType::SizeType &regionSize = outputRegionForThread.GetSize();

  if( regionSize[0] == 0 )
    {
    return;
    }
  const TInputImage *inputPtr = this->GetInput();
  TOutputImage *outputPtr = this->GetOutput(0);

  // Define the portion of the input to walk for this thread, using
  // the CallCopyOutputRegionToInputRegion method allows for the input
  // and output images to be different dimensions
  typename TInputImage::RegionType inputRegionForThread;

  this->CallCopyOutputRegionToInputRegion(inputRegionForThread, outputRegionForThread);

  // Define the iterators
  itk::ImageScanlineConstIterator< TInputImage > inputIt(inputPtr, inputRegionForThread);
  itk::ImageScanlineIterator< TOutputImage > outputIt(outputPtr, outputRegionForThread);

  inputIt.GoToBegin();
  outputIt.GoToBegin();
  while ( !inputIt.IsAtEnd() )
    {
    while ( !inputIt.IsAtEndOfLine() )
      {
      const InputPixelType &inputPixel = inputIt.Get();
      OutputPixelType value;
      for ( unsigned int k = 0; k < OutputPixelType::Dimension; k++ )
        {
        value[k] = static_cast< typename OutputPixelType::ValueType >( inputPixel[k] );
        }
      outputIt.Set( value );

      ++inputIt;
      ++outputIt;
      }
    inputIt.NextLine();
    outputIt.NextLine();
    }

}

} // end namespace otb

#endif

#endif // ITK_VERSION_MAJOR