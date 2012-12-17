
#include "SimpleITKTestHarness.h"

void DataFinder::SetRuntimeDirectoryFromArgv0 ( std::string argv0 )
  {
  std::string errorMessage, path, dir, file;
  bool result = itksys::SystemTools::FindProgramPath ( argv0.c_str(), path, errorMessage );
  mRuntimeDirectory = "";
  if ( result == false )
    {
    std::cerr << "SetRuntimeDirectoryFromArgv0: couldn't determine the location of "
      << argv0 << " error was: " << errorMessage << std::endl;
    }
  else
    {
    result = itksys::SystemTools::SplitProgramPath ( path.c_str(), dir, file );
    if ( result == false )
      {
      std::cerr << "SetRuntimeDirectoryFromArgv0: couldn't split directory from path " << path << std::endl;
      }
    else
      {
      mRuntimeDirectory = dir;
      }
    }
  }

std::vector<std::string> DataFinder::FindExecutable ( std::string exe )
  {
  return std::vector<std::string>( 1, this->GetRuntimeDirectory() + "/" + exe + EXECUTABLE_SUFFIX );
  }

std::vector<std::string> DataFinder::GetLuaExecutable ()
  {
  return this->FindExecutable ( "SimpleITKLua" );
  }

std::vector<std::string> DataFinder::GetTclExecutable ()
  {
  return this->FindExecutable ( "SimpleITKTclsh" );
  }

std::vector<std::string> DataFinder::GetPythonExecutable ()
  {
  return std::vector<std::string> ( 1, PYTHON_EXECUTABLE_PATH );
  }

std::vector<std::string> DataFinder::GetRubyExecutable ()
  {
  return std::vector<std::string> ( 1, RUBY_EXECUTABLE_PATH );
  }

std::vector<std::string> DataFinder::GetRExecutable ()
  {
  return std::vector<std::string> ( 1, R_EXECUTABLE_PATH );
  }

std::vector<std::string> DataFinder::GetJavaExecutable ()
  {
  return std::vector<std::string> ( 1, JAVA_EXECUTABLE_PATH );
  }

std::vector<std::string> DataFinder::GetCSharpCompiler ()
  {
  return std::vector<std::string> ( 1, CSHARP_COMPILER );
  }

std::vector<std::string> DataFinder::GetCSharpInterpreter ()
  {
  return std::vector<std::string> ( 1, CSHARP_INTERPRETER );
  }

int ExternalProgramRunner::RunExecutable ( std::vector<std::string> CommandLine, bool showOutput )
  {
  std::string fullCommand;
  for ( unsigned int idx = 0; idx < CommandLine.size(); idx++ ) {
    fullCommand += CommandLine[idx];
    fullCommand += " ";
  }

  if ( showOutput )
    {
    std::cout << "Running command: '" << fullCommand << "'" << std::endl;
    }

  // Allocate what we need
  const char* StringCommandLine[256];
  for ( unsigned int idx = 0; idx < CommandLine.size(); idx++ )
    {
    StringCommandLine[idx] = CommandLine[idx].c_str();
    }
  StringCommandLine[CommandLine.size()] = NULL;

  itksysProcess* process = itksysProcess_New();
  itksysProcess_SetCommand ( process, StringCommandLine );
  itksysProcess_Execute ( process );

  int status;
  char* processData;
  int processDataLength;
  while ( 1 )
    {
    status = itksysProcess_WaitForData ( process, &processData, &processDataLength, NULL );
    if ( status == itksysProcess_Pipe_STDOUT || status == itksysProcess_Pipe_STDERR )
      {
      if ( showOutput )
        {
        std::cout << std::string ( processData ) << std::endl;
        }
      }
    else
      {
      // Nothing ready, so wait for the process to exit...
      break;
      }
    }

  bool failed = false;
  int ret = -1;

  itksysProcess_WaitForExit ( process, 0 );

  switch (itksysProcess_GetState( process ))
    {
    case itksysProcess_State_Starting:
      std::cerr << "No process has been executed." << std::endl;
      failed = true;
      break;
    case itksysProcess_State_Executing:
      std::cerr << "The process is still executing." << std::endl;;
      failed = true;
      break;
    case itksysProcess_State_Expired:
      std::cerr << "Child was killed when timeout expired." << std::endl;
      failed = true;
      break;
    case itksysProcess_State_Exited:
      ret = itksysProcess_GetExitValue(process);
      break;
    case itksysProcess_State_Killed:
      std::cerr << "Child was killed by parent." << std::endl;
      failed = true;
      break;
    case itksysProcess_State_Exception:
      std::cerr << "Child terminated abnormally: " << itksysProcess_GetExceptionString( process ) << std::endl;;
      failed = true;
      break;
    case itksysProcess_State_Disowned:
      std::cerr << "Child was disowned." << std::endl;
      failed = true;
      break;
    case itksysProcess_State_Error:
      std::cerr << "Error in administrating child process: [" << itksysProcess_GetErrorString( process ) << "]" << std::endl;
      failed = true;
      break;
    default:
      std::cerr << "Unknown Process State" << std::endl;
      failed = true;
    };

  itksysProcess_Delete ( process );

  if ( failed )
    {
    // HACK: GTest currently expects functions of void type, by
    // using the comma operator we can get around this.
    FAIL(), -1;
    }
  return ret;
}


void ExternalProgramRunner::CheckImageHash( const std::string &fileName, const std::string &hash )
  {
  ASSERT_TRUE ( dataFinder.FileExists ( fileName ) ) << "check if " << fileName << " exists.";

  itk::simple::Image image = itk::simple::ReadImage( fileName );
  EXPECT_EQ ( hash, itk::simple::Hash( image ) );
  }

void ExternalProgramRunner::SetEnvironment ( std::string key, std::string value )
  {
#ifdef WIN32
  std::string v = key + "=" + value;
  _putenv ( v.c_str() );
  std::cout << "SetEnvironment: " << v << std::endl;
#else
  setenv ( key.c_str(), value.c_str(), 1 );
  std::cout << "SetEnvironment: " << key << "=" << value << std::endl;
#endif
  }