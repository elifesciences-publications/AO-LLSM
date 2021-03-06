/*=========================================================================
 Authors: The GoFigure Dev. Team.
 at Megason Lab, Systems biology, Harvard Medical school, 2009

 Copyright (c) 2009, President and Fellows of Harvard College.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 Neither the name of the  President and Fellows of Harvard College
 nor the names of its contributors may be used to endorse or promote
 products derived from this software without specific prior written
 permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include <iostream>
#include <vector>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "itkSettingsInfoExtractionFilter.h"
#include "itkStitchingSharedData.h"
#include "itkExtractStitchTransformImageFilter.h"
#include "anyoption.h"
#include "itkTimeProbe.h"

int main ( int argc, char* argv[] )
{
  const unsigned int Dimension = 3;
  typedef std::vector< std::string > StringVectorType;
  typedef std::vector< double > DoubleVectorType;
  typedef vnl_matrix< double > vnlMatrixType;
  typedef vnl_vector< double > vnlVectorType;
  typedef itk::Directory DirectoryType;
  typedef std::vector< std:: vector< std::vector< std::string > > > StringArray3DType;

  typedef unsigned short PixelType;
  typedef itk::Image< PixelType, Dimension > ImageType;
  typedef itk::ImageFileReader< ImageType > ReaderType;
  typedef itk::ImageRegionIteratorWithIndex< ImageType > IteratorType;

  typedef ImageType::SpacingType SpacingType;
  typedef ImageType::SizeType SizeType;
  typedef ImageType::IndexType IndexType;
  typedef ImageType::RegionType RegionType;
  typedef ImageType::PointType PointType;
  typedef SizeType::SizeValueType SizeValueType;

  typedef itk::SettingsInfoExtractionFilter< double, ImageType > SettingsFilterType;
  typedef itk::StitchingSharedData< ImageType > SharedDataType;
  typedef itk::ExtractStitchTransformImageFilter< ImageType > ExtractTransformFilterType;

  // Measure time taken
  itk::TimeProbe cputimer;
  cputimer.Start();
  
  /* 1. CREATE AN OBJECT */
  AnyOption *opt = new AnyOption();

  /* 2. SET PREFERENCES  */
  //opt->noPOSIX(); /* do not check for POSIX style character options */
  //opt->setVerbose(); /* print warnings about unknown options */
  //opt->autoUsagePrint(true); /* print usage for bad options */

  /* 3. SET THE USAGE/HELP   */
  opt->addUsage( "" );
  opt->addUsage( "Usage: " );
  opt->addUsage( "" );
  opt->addUsage( " iSettings file directory " );
  opt->addUsage( " iTile directories " );
  opt->addUsage( " -h   --help    Prints this help " );
  opt->addUsage( " -c   --channel 0   (default) channel value" );
  opt->addUsage( " -t   --time    0   (default) timepoint" );
  opt->addUsage( " -n   --threads 1   (default) number of threads" );
  opt->addUsage( " -x   --exp     _ch (default) string marking channel information" );
  opt->addUsage( " -o   --offset      (default) offset filename" );
  opt->addUsage( " -s   --zstart  0   (default) z tile" );
  opt->addUsage( " -e   --zend    1   (default) z tile" );
  opt->addUsage( " -l   --length  5.0 (default) search length vector" );
  opt->addUsage( " -d   --delta   0.5 (default) delta step length vector" );
  opt->addUsage( "" );

  /* 4. SET THE OPTION STRINGS/CHARACTERS */

  /* by default all  options will be checked on the command line
    and from option/resource file */

  /* a flag (takes no argument), supporting long and short form */
  opt->setFlag(  "help",  'h' );

  /* an option (takes an argument), supporting long and short form */
  opt->setOption(  "channel", 'c' );
  opt->setOption(  "time",    't' );
  opt->setOption(  "threads", 'n' );
  opt->setOption(  "exp",     'x' );
  opt->setOption(  "offset",  'o' );
  opt->setOption(  "zstart",  's' );
  opt->setOption(  "zend",    'e' );
  opt->setOption(  "length",  'l' );
  opt->setOption(  "delta",   'd' );
  
  /* 5. PROCESS THE COMMANDLINE AND RESOURCE FILE */
  /* read options from a  option/resource file with ':'
  separated options or flags, one per line */

  opt->processFile( ".options" );
  opt->processCommandArgs( argc, argv );

  if( ! opt->hasOptions())
  {
    opt->printUsage();
    delete opt;
    return EXIT_FAILURE;
  }

  unsigned int ch = 0;
  unsigned int tp = 0;
  unsigned int zStart = 0;
  unsigned int zEnd = 100;
  std::string OffsetFilePath = "";
  std::string searchCH = "_ch";
  unsigned int numOfThreads = 1;
  std::vector<double> searchRadius;
  std::vector<double> stepLength;

  if( opt->getArgc() < 2 )
  {
    std::cerr << "Insufficient # of arguments " << opt->getArgc() << std::endl;
    opt->printUsage();
    delete opt;
    return EXIT_FAILURE;
  }
  if( opt->getFlag( "help" ) || opt->getFlag( 'h' ) )
  {
    opt->printUsage();
    delete opt;
    return EXIT_FAILURE;
  }

  if( opt->getValue( 'c' ) != NULL  || opt->getValue( "channel" ) != NULL  )
  {
    ch = atoi( opt->getValue( 'c' ) );
  }
  if( opt->getValue( 't' ) != NULL  || opt->getValue( "time" ) != NULL  )
  {
    tp = atoi( opt->getValue( 't' ) );
  }
  if( opt->getValue( 'n' ) != NULL  || opt->getValue( "threads" ) != NULL  )
  {
    numOfThreads = atoi( opt->getValue( 'n' ) );
  }
  if( opt->getValue( 'x' ) != NULL  || opt->getValue( "exp" ) != NULL  )
  {
    searchCH = opt->getValue( 'x' );
  }
  if( opt->getValue( 's' ) != NULL  || opt->getValue( "zstart" ) != NULL  )
  {
    zStart = atoi( opt->getValue( 's' ) );
    zEnd = zStart;
  }
  if( opt->getValue( 'e' ) != NULL  || opt->getValue( "zend" ) != NULL  )
  {
    zEnd = atoi( opt->getValue( 'e' ) );
  }
  if( opt->getValue( 'd' ) != NULL  || opt->getValue( "delta" ) != NULL  )
  {
    std::string input = opt->getValue( 'd' );
    std::istringstream ss( input );
    std::string token;

    while( std::getline(ss, token, ',') )
    {
      stepLength.push_back( atof(token.c_str()) );
    }
  }
  else
  {
    stepLength.push_back( 0.5 );
  }
  if( opt->getValue( 'l' ) != NULL  || opt->getValue( "length" ) != NULL  )
  {
    std::string input = opt->getValue( 'l' );
    std::istringstream ss( input );
    std::string token;

    while( std::getline(ss, token, ',') )
    {
      searchRadius.push_back( atof(token.c_str()) );
    }
  }
  else
  {
    searchRadius.push_back( 5.0 );
  }
  if( opt->getValue( 'o' ) != NULL  || opt->getValue( "offset" ) != NULL  )
  {
    OffsetFilePath = opt->getValue( 'o' );
  }

  SharedDataType::Pointer m_SharedData = SharedDataType::New();

  SettingsFilterType::Pointer settingsReader = SettingsFilterType::New();
  settingsReader->SetSettingsDirectory( argv[1] );
  settingsReader->SetTileDirectory( argv[2] );
  settingsReader->SetChannelNumber( ch );
  settingsReader->SetChannelPrefix( searchCH );
  settingsReader->SetTimePoint( tp );
  settingsReader->SetSharedData( m_SharedData );
  settingsReader->Read();

  ExtractTransformFilterType::Pointer extractTransform = ExtractTransformFilterType::New();
  extractTransform->SetStepLength( stepLength );
  extractTransform->SetSearchRadius( searchRadius );
  extractTransform->SetZTileStart( zStart );
  extractTransform->SetZTileEnd( zEnd );
  extractTransform->SetSharedData( m_SharedData );
  extractTransform->SetOffsetFilePath( OffsetFilePath );
  extractTransform->SetNumberOfThreads( numOfThreads );
  extractTransform->BeginRegister();
  
  cputimer.Stop();
  std::cout << "Registration took " << cputimer.GetMean() << " seconds" << std::endl;

  return EXIT_SUCCESS;
}
