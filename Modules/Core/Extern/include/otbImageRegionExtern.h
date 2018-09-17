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

#ifndef itkImageRegionExtern_h
#define itkImageRegionExtern_h

#include "OTBExternExport.h"
#include "itkImageRegion.h"
#include "itkImageRegionSplitter.h"
#include "otbImageRegionSquareTileSplitter.h"
#include "otbImageRegionAdaptativeSplitter.h"
#include "otbRemoteSensingRegion.h"

namespace itk {
// Prevent implicit instanciation of common types to improve build performance
// Explicit instanciations are provided in the .cxx
extern template class ImageRegion<2>;
extern template class OTBExtern_EXPORT_TEMPLATE ImageRegionSplitter<2>;
}

namespace otb {
extern template class OTBExtern_EXPORT_TEMPLATE ImageRegionSquareTileSplitter<2>;
extern template class OTBExtern_EXPORT_TEMPLATE ImageRegionAdaptativeSplitter<2>;
extern template class RemoteSensingRegion<double>;
}

#endif // otbImageRegion_h
