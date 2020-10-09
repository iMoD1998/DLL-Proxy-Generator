#include <Windows.h>
#include <fstream>
#include <lyra/lyra.hpp>

#include "ExportEntry.h"
#include "Export Generator.h"
#include "Def File Generator.h"
#include "Pragma File Generator.h"
#include "Asm File Generator.h"

void GenerateDefForwardedExports( 
    _In_ const std::string&              DLLName,
    _In_ const std::vector<ExportEntry>& Entries
)
{
	auto Generator = DefFileGenerator( DLLName + ".def" );

	if ( !Generator.Open() )
	{
		printf( "Failed to open Pragma Exports File\n" );
		return;
	}

	Generator.Begin( );

	for ( const auto& Export : Entries )
	{
		Generator.AddForwardedExportEntry( Export, DLLName );
	}

	Generator.End();

	printf( "Wrote %i def file exports to %s\n", Entries.size(), ( DLLName + ".def" ).c_str() );
}

void GeneratePragmasForwardedExports(
    _In_ const std::string&              DLLName,
    _In_ const std::vector<ExportEntry>& Entries
)
{
	auto Generator = PragmaFileGenerator( DLLName + "Exports.h" );

	if ( !Generator.Open() )
	{
		printf( "Failed to open Pragma Exports File\n" );
		return;
	}

	Generator.Begin();

	for ( const auto& Export : Entries )
	{
		Generator.AddForwardedExportEntry( Export, DLLName );
	}

	Generator.End();

	printf( "Wrote %i pragma exports to %s\n", Entries.size(), ( DLLName + "Exports.h" ).c_str() );
}

void GenerateASM(
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
	auto StubGenerator   = ASMFileGenerator( DLLName + "ASMStubs.h" );

	if ( !StubGenerator.Open() )
	{
		printf( "Failed to open ASMStubs File\n" );
		return;
	}

	if ( UseDefFile )
	{
		LinkerGenerator = std::make_shared< DefFileGenerator >( DLLName + "Stubs.def" );

		if ( !LinkerGenerator->Open() )
		{
			printf( "Failed to open Def File\n" );
			return;
		}
	}
	else
	{
		LinkerGenerator = std::make_shared< PragmaFileGenerator >( DLLName + "StubsExports.h" );

		if ( !LinkerGenerator->Open() )
		{
			printf( "Failed to open Pragma Exports File\n" );
			return;
		}
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

		if ( Export.HasName() )
			SymbolName = Export.GetName();

		StubGenerator.AddExportEntry( Export, SymbolName );

		LinkerGenerator->AddExportEntry( Export, SymbolName );
	}

	StubGenerator.End();

	LinkerGenerator->End();
}

int main(int argc, const char* argv[])
{
	std::string DLLPathIn;
	bool Verbose = false;
	bool ShouldShowHelp = false;

	auto CommandLineParser = lyra::cli();

	CommandLineParser.add_argument( lyra::help( ShouldShowHelp ) );
	CommandLineParser.add_argument( lyra::opt( Verbose )[ "-v" ][ "--verbose" ]( "Show infomation about exports" ) );
	CommandLineParser.add_argument( lyra::arg( DLLPathIn, "DLLPATH" )( "Path of the DLL to get exports from" ).required() );

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

	if ( ExportEntry::GetExportEntries( DLLPath, Entries, Verbose, &MachineType ) )
	{
		auto DLLName = DLLPath.filename().replace_extension( "" ).string();

		GenerateDefForwardedExports( DLLName, Entries );
		GeneratePragmasForwardedExports( DLLName, Entries );
		GenerateASM( DLLName, Entries, true, MachineType );
	}

	return 0;
}