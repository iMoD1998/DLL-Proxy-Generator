#include <Windows.h>
#include <fstream>
#include <lyra/lyra.hpp>

#include "ExportEntry.h"
#include "Export Generator.h"
#include "Def File Generator.h"
#include "Pragma File Generator.h"
#include "Asm File Generator.h"
#include "VS Generator.h"
#include "DLLMain Generator.h"

void GenerateForwardedExports( 
	_Inout_    VSGenerator&              VSProject,
	_In_ const std::filesystem::path&    OutDir,
	_In_ const std::string&              DLLName,
	_In_ const std::string&              NewDLLName,
	_In_ const std::vector<ExportEntry>& Entries,
	_In_ bool                            UseDefFile
)
{
	auto LinkerGenerator = std::shared_ptr<ExportGenerator>();
	auto MainGenerator   = DLLMainGenerator( OutDir / "DLLMain.cpp" );

	if ( !MainGenerator.Open() )
	{
		printf( "Failed to open DLL Main File\n" );
		return;
	}

	VSProject.AddFile<VSSourceFile>( "DLLMain.cpp" );

	if ( UseDefFile )
	{
		LinkerGenerator = std::make_shared< DefFileGenerator >( OutDir / ( DLLName + ".def" ) );

		if ( !LinkerGenerator->Open() )
		{
			printf( "Failed to open Def File\n" );
			return;
		}

		VSProject.SetDefinitionFile( DLLName + ".def" );
	}
	else
	{
		LinkerGenerator = std::make_shared< PragmaFileGenerator >( OutDir / ( DLLName + "Exports.h" ) );

		if ( !LinkerGenerator->Open() )
		{
			printf( "Failed to open Pragma Exports File\n" );
			return;
		}

		VSProject.AddFile<VSHeaderFile>( DLLName + "Exports.h" );
		MainGenerator.AddInclude( DLLName + "Exports.h" );
	}

	if ( !LinkerGenerator->Begin( NULL, NULL ) )
	{
		printf( "Linker generator failed to begin\n" );
		return;
	}

	for ( const auto& Export : Entries )
	{
		LinkerGenerator->AddForwardedExportEntry( Export, NewDLLName );
	}

	MainGenerator.Write();

	LinkerGenerator->End();

	VSProject.Generate();

	printf( "Wrote %i def file exports to %s\n", Entries.size(), ( DLLName + ".def" ).c_str() );
}

void GenerateASM(
	_Inout_    VSGenerator&              VSProject,
	_In_ const std::filesystem::path&    OutDir,
	_In_ const std::string&              DLLName,
	_In_ const std::vector<ExportEntry>& Entries,
	_In_ bool                            UseDefFile,
	_In_ UINT16                          MachineType
)
{
	std::ofstream DefPragmaFileOut;
	std::ofstream ASMFileOut;
	std::ofstream DLLMainFileOut;

	auto LinkerGenerator = std::shared_ptr<ExportGenerator>();
	auto StubGenerator   = ASMFileGenerator( OutDir / ( DLLName + "ASMStubs.asm" ) );
	auto MainGenerator   = DLLMainGenerator( OutDir / "DLLMain.cpp" );

	VSProject.AddFile<VSMASMFile>( DLLName + "ASMStubs.asm" );
	VSProject.AddFile<VSSourceFile>( "DLLMain.cpp" );

	if ( !StubGenerator.Open() )
	{
		printf( "Failed to open ASMStubs File\n" );
		return;
	}

	if ( !MainGenerator.Open() )
	{
		printf( "Failed to open DLL Main File\n" );
		return;
	}

	if ( UseDefFile )
	{
		LinkerGenerator = std::make_shared< DefFileGenerator >( OutDir / ( DLLName + "Stubs.def" ) );

		if ( !LinkerGenerator->Open() )
		{
			printf( "Failed to open Def File\n" );
			return;
		}

		VSProject.SetDefinitionFile( DLLName + "Stubs.def" );
	}
	else
	{
		LinkerGenerator = std::make_shared< PragmaFileGenerator >( OutDir / ( DLLName + "StubExports.h" ) );

		if ( !LinkerGenerator->Open() )
		{
			printf( "Failed to open Pragma Exports File\n" );
			return;
		}

		VSProject.AddFile<VSHeaderFile>( DLLName + "StubsExports.h" );
		MainGenerator.AddInclude( DLLName + "StubExports.h" );
	}

	if ( !StubGenerator.Begin( MachineType, Entries.size() ) )
	{
		printf( "Stub generator failed to begin\n" );
		return;
	}

	if ( !LinkerGenerator->Begin( MachineType, NULL ) )
	{
		printf( "Linker generator failed to begin\n" );
		return;
	}

	MainGenerator.AddBody( "extern \"C\" void* g_FunctionTable[];\n\n" );
	MainGenerator.AddBody( "void PopulateFunctionTable()\n{\n" );
	MainGenerator.AddBody( "\tHMODULE OriginalModule = LoadLibraryA( \"" + DLLName + ".dll\" );\n" );

	for ( const auto& Export : Entries )
	{
		if ( Export.IsData() )
		{
			if ( Export.HasName() )
				printf( "Warning export %s is data\n", Export.GetName().c_str() );
			else
				printf( "Warning export ordinal %i is data\n", Export.GetOrdinal() );

			continue;
		}

		std::string SymbolName = "Ordinal_" + std::to_string( Export.GetOrdinal() );
		std::string ExportName = "#" + std::to_string( Export.GetOrdinal() );

		if ( Export.HasName() )
		{
			SymbolName = Export.GetName();
			ExportName = Export.GetName();
		}

		StubGenerator.AddExportEntry( Export, SymbolName );

		LinkerGenerator->AddExportEntry( Export, SymbolName );

		MainGenerator.AddBody( "\tg_FunctionTable[ " + std::to_string( Export.GetOrdinalIndex() ) + " ] = GetProcAddress( OriginalModule, \"" + ExportName + "\" );\n" );
	}

	MainGenerator.AddBody( "}\n" );
	MainGenerator.AddProcessAttach( "\tPopulateFunctionTable();\n" );

	StubGenerator.End();

	LinkerGenerator->End();

	MainGenerator.Write();

	VSProject.Generate();
}

int main(int argc, const char* argv[])
{
	std::string DLLPathIn;
	std::string OutDirIn;
	std::string VSProjectName;
	std::string ForwardDLL;

	bool Verbose           = false;
	bool ShouldShowHelp    = false;
	bool GenerateVSProject = false;
	bool PreferDef         = false;

	auto CommandLineParser = lyra::cli();

	CommandLineParser.add_argument( lyra::help( ShouldShowHelp ) );
	CommandLineParser.add_argument( lyra::opt ( Verbose )                        [ "-v" ]  [ "--verbose" ]     ( "Show infomation about exports" ) );
	CommandLineParser.add_argument( lyra::opt ( GenerateVSProject )              [ "-p" ]  [ "--visualstudio" ]( "Generate Visual Studio project" ) );
	CommandLineParser.add_argument( lyra::opt ( PreferDef )                      [ "-d" ]  [ "--def" ]         ( "Prefer def file over #pragma" ) );
	CommandLineParser.add_argument( lyra::opt ( ForwardDLL,    "NEWDLLNAME" )    [ "-f" ]  [ "--forward" ]     ( "Use export forwarding to forward exports to old DLL with new name" ) );
	CommandLineParser.add_argument( lyra::opt ( VSProjectName, "PROJNAME" )      [ "-n" ]  [ "--vsname" ]      ( "Name for visual studio project" ) );
	CommandLineParser.add_argument( lyra::opt ( OutDirIn,      "OUTDIR" )        [ "-o" ]  [ "--out" ]         ( "Out directory for files" ) );
	CommandLineParser.add_argument( lyra::arg ( DLLPathIn,     "DLLPATH" )                                     ( "Path of the DLL to get exports from" ).required() );

	// Parse the program arguments:
	auto ParsedArgs = CommandLineParser.parse( { argc, argv } );

	// Check that the arguments where valid:
	if ( !ParsedArgs )
	{
		std::cerr << ParsedArgs.errorMessage() << std::endl;
		return 1;
	}

	if ( ShouldShowHelp )
	{
		std::cout << CommandLineParser << std::endl;
		return 0;
	}

	std::vector<ExportEntry> Entries;

	std::filesystem::path DLLPath = DLLPathIn;

	UINT16 MachineType = 0;

	if ( OutDirIn.size() != 0 )
	{
		if ( !std::filesystem::exists( OutDirIn ) || !std::filesystem::is_directory( OutDirIn ) )
		{
			printf( "Enter valid output directory\n" );
			return 1;
		}
	}

	if ( !std::filesystem::exists( DLLPath ) )
	{
		printf( "DLL file doesnt exist\n" );
		return 2;
	}

	auto DLLName = DLLPath.filename().replace_extension( "" ).string();

	if ( VSProjectName.size() == 0 )
		VSProjectName = DLLName + " Proxy";

	if ( ExportEntry::GetExportEntries( DLLPath, Entries, Verbose, &MachineType ) )
	{
		auto VSGen = VSGenerator( VSProjectName, OutDirIn, MachineType );

		std::filesystem::path OutputDir = OutDirIn;

		if ( GenerateVSProject )
			OutputDir = VSGen.GetProjectPath();

		if ( ForwardDLL.size() )
		{
			GenerateForwardedExports( VSGen, OutputDir, DLLName, std::filesystem::path( ForwardDLL ).replace_extension().string(), Entries, PreferDef );
		}
		else
		{
			GenerateASM( VSGen, OutputDir, DLLName, Entries, PreferDef, MachineType );
		}

		//GenerateDefForwardedExports( DLLName, Entries );
		//GeneratePragmasForwardedExports( DLLName, Entries );
		//GenerateASM( DLLName, Entries, true, MachineType );
	}

	return 0;
}