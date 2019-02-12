/*
 * Copyright (C) 2005-2019 Centre National d'Etudes Spatiales (CNES)
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

#include "otbVectorDataProjectionFilter.h"
#include "otbVectorData.h"
#include "otbVectorDataFileReader.h"
#include "otbVectorDataFileWriter.h"

int otbVectorDataProjectionFilterFromMapToGeo(int argc, char * argv[])
{

  if (argc < 2)
    {
    std::cout << argv[0] << " <input vector filename> <output vector filename> "  << std::endl;

    return EXIT_FAILURE;
    }

  typedef otb::VectorData<double> InputVectorDataType;
  typedef otb::VectorData<double> OutputVectorDataType;

  typedef otb::VectorDataFileReader<InputVectorDataType> VectorDataFileReaderType;
  VectorDataFileReaderType::Pointer reader = VectorDataFileReaderType::New();

  reader->SetFileName(argv[1]);
  reader->UpdateOutputInformation();

  typedef otb::VectorDataProjectionFilter<InputVectorDataType, OutputVectorDataType> VectorDataFilterType;
  VectorDataFilterType::Pointer vectorDataProjection = VectorDataFilterType::New();

  vectorDataProjection->SetInput(reader->GetOutput());

  typedef otb::VectorDataFileWriter<OutputVectorDataType> VectorDataFileWriterType;
  VectorDataFileWriterType::Pointer writer = VectorDataFileWriterType::New();

  writer->SetFileName(argv[2]);
  writer->SetInput(vectorDataProjection->GetOutput());
  writer->Update();

  return EXIT_SUCCESS;
}
