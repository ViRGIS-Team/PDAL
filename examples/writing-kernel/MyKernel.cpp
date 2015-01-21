// MyKernel.cpp

#include "MyKernel.hpp"

#include <boost/program_options.hpp>

#include <pdal/Filter.hpp>
#include <pdal/Kernel.hpp>
#include <pdal/KernelFactory.hpp>
#include <pdal/KernelSupport.hpp>
#include <pdal/Options.hpp>
#include <pdal/pdal_macros.hpp>
#include <pdal/PointBuffer.hpp>
#include <pdal/PointContext.hpp>
#include <pdal/Reader.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/Writer.hpp>

#include <memory>
#include <string>

namespace po = boost::program_options;

CREATE_KERNEL_PLUGIN(mykernel, MyKernel)

void MyKernel::validateSwitches()
{
  if (m_input_file == "")
    throw pdal::app_usage_error("--input/-i required");
  if (m_output_file == "")
    throw pdal::app_usage_error("--output/-o required");
}

void MyKernel::addSwitches()
{
  po::options_description* options = new po::options_description("file options");
  options->add_options()
  ("input,i", po::value<std::string>(&m_input_file)->default_value(""), "input file name")
  ("output,o", po::value<std::string>(&m_output_file)->default_value(""), "output file name")
  ;

  addSwitchSet(options);
  addPositionalSwitch("input", 1);
  addPositionalSwitch("output", 1);
}

int MyKernel::execute()
{
  pdal::PointContextRef ctx;
  pdal::StageFactory f;

  std::unique_ptr<pdal::Reader> reader(f.createReader("readers.las"));
  pdal::Options readerOptions;
  readerOptions.add("filename", m_input_file);
  reader->setOptions(readerOptions);

  std::unique_ptr<pdal::Filter> filter(f.createFilter("filters.decimation"));
  pdal::Options filterOptions;
  filterOptions.add("step", 10);
  filter->setOptions(filterOptions);
  filter->setInput(reader.get());

  std::unique_ptr<pdal::Writer> writer(f.createWriter("writers.text"));
  pdal::Options writerOptions;
  writerOptions.add("filename", m_output_file);
  writer->setOptions(writerOptions);
  writer->setInput(filter.get());
  writer->prepare(ctx);
  writer->execute(ctx);

  return 0;
}

