#include <Windows.h>
#include <fstream>
#include <lyra/lyra.hpp>

#include "ExportEntry.h"

void GenerateDefForwardedExports( 
    _In_ const std::string&              DLLName,
    _In_ const std::vector<ExportEntry>& Entries
)
{
	std::ofstream DefFileOut;

	DefFileOut.open( DLLName + ".def" );

	if ( !DefFileOut.is_open() )
	{
		printf( "Failed to open Def File\n" );
		return;
	}

	DefFileOut << "LIBRARY" << std::endl;
	DefFileOut << "EXPORTS" << std::endl;

	for ( const auto& Export : Entries )
	{
		if ( Export.HasName() )
		{
			DefFileOut << "\t" << Export.GetName() << "=";
		}
		else
		{
			DefFileOut << "\t#" << Export.GetOrdinal() << "=";
		}

		if ( Export.IsForwarded() )
		{
			DefFileOut << Export.GetForwardedName();

			if ( !Export.HasName() )
			{
				DefFileOut << " @ " << Export.GetOrdinal() << " NONAME";
			}
		}
		else
		{
			DefFileOut << DLLName << ".";

			if ( Export.HasName() )
			{
				DefFileOut << Export.GetName();
			}
			else
			{
				DefFileOut << "#" << Export.GetOrdinal() << " @ " << Export.GetOrdinal() << " NONAME";
			}
		}

		DefFileOut << std::endl;
	}
}

void GeneratePragmasForwardedExports(
    _In_ const std::string&              DLLName,
    _In_ const std::vector<ExportEntry>& Entries
)
{
	std::ofstream PragmaFileOut;

	PragmaFileOut.open( DLLName + "Exports.h" );

	if ( !PragmaFileOut.is_open() )
	{
		printf( "Failed to open Pragma Exports File\n" );
		return;
	}

	for ( const auto& Export : Entries )
	{
		if ( Export.HasName() )
		{
			PragmaFileOut << "#pragma comment(linker,\"/export:" << Export.GetName() << "=";
		}
		else
		{
			PragmaFileOut << "#pragma comment(linker,\"/export:#" << Export.GetOrdinal() << "=";
		}

		if ( Export.IsForwarded() )
		{
			PragmaFileOut << Export.GetForwardedName();

			if ( !Export.HasName() )
			{
				PragmaFileOut << ",@" << Export.GetOrdinal() << ",NONAME";
			}
		}
		else
		{
			PragmaFileOut << DLLName << ".";

			if ( Export.HasName() )
			{
				PragmaFileOut << Export.GetName();
			}
			else
			{
				PragmaFileOut << "#" << Export.GetOrdinal() << ",@" << Export.GetOrdinal() << ",NONAME";
			}
		}

		if ( Export.IsData() )
		{
			PragmaFileOut << ",DATA";
		}

		PragmaFileOut << "\")" << std::endl;
	}

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

	ASMFileOut.open( DLLName + "ASMStubs.h" );

	if ( !ASMFileOut.is_open() )
	{
		printf( "Failed to open ASMStubs File\n" );
		return;
	}

	if ( UseDefFile )
	{
		DefPragmaFileOut.open( DLLName + "Stubs.def" );

		if ( !DefPragmaFileOut.is_open() )
		{
			printf( "Failed to open Def File\n" );
			return;
		}

		DefPragmaFileOut << "LIBRARY" << std::endl;
		DefPragmaFileOut << "EXPORTS" << std::endl;
	}
	else
	{
		DefPragmaFileOut.open( DLLName + "StubsExports.h" );

		if ( !DefPragmaFileOut.is_open() )
		{
			printf( "Failed to open Pragma Exports File\n" );
			return;
		}
	}

	std::string FunctionTableName = "";
	SIZE_T      MachinePointerSize = 0;
	UINT32      CurrentFunctionIndex = 0;

	switch ( MachineType )
	{
		case IMAGE_FILE_MACHINE_AMD64:
			ASMFileOut << ".DATA" << std::endl;
			ASMFileOut << "g_FunctionTable QWORD " << Entries.size() << " dup(?)" << std::endl;
			ASMFileOut << "PUBLIC g_FunctionTable" << std::endl << std::endl;
			FunctionTableName = "g_FunctionTable";
			MachinePointerSize = sizeof( UINT64 );
			break;
		case IMAGE_FILE_MACHINE_I386:
			ASMFileOut << ".MODEL FLAT" << std::endl;
			ASMFileOut << ".DATA" << std::endl;
			ASMFileOut << "_g_FunctionTable DWORD " << Entries.size() << " dup(?)" << std::endl;
			ASMFileOut << "PUBLIC _g_FunctionTable" << std::endl << std::endl;
			FunctionTableName = "_g_FunctionTable"; // shitty calling convention decoration
			MachinePointerSize = sizeof( UINT32 );
			break;
		default:
			printf( "Unknown machine type %04X\n", MachineType );
			return;
	}

	ASMFileOut << ".CODE" << std::endl;

	for ( const auto& Export : Entries )
	{
		if ( Export.IsData() )
		{
			if ( Export.HasName() )
			{
				printf( "Warning export %s is data\n", Export.GetName().c_str() );
			}
			else
			{
				printf( "Warning export ordinal %i is data\n", Export.GetOrdinal() );
			}

			continue;
		}

		std::string SymbolName = "Ordinal_" + std::to_string( CurrentFunctionIndex );

		if ( Export.HasName() )
		{
			SymbolName = Export.GetName();
		}

		/*
			Generate Stub Like

			SymbolName PROC
				jmp [FunctionTableName + FunctionIndex * MachinePointerSize]
			SymbolName ENDP
		
		*/

		ASMFileOut << SymbolName << " PROC" << std::endl;
		ASMFileOut << "\tjmp [" << FunctionTableName << " + " << CurrentFunctionIndex << " * " << MachinePointerSize << "]" << std::endl;
		ASMFileOut << SymbolName << " ENDP" << std::endl;
		ASMFileOut << std::endl;

		if ( UseDefFile )
		{
			if ( Export.HasName() )
			{
				DefPragmaFileOut << Export.GetName() << std::endl;
			}
			else
			{
				DefPragmaFileOut << SymbolName <<  " @ " << Export.GetOrdinal() << " NONAME" << std::endl;
			}
		}
		else
		{
			if ( Export.HasName() )
			{
				DefPragmaFileOut << "#pragma comment(linker,\"/export:" << Export.GetName() << "\")" << std::endl;
			}
			else
			{
				DefPragmaFileOut << "#pragma comment(linker,\"/export:" << SymbolName << ",@" << Export.GetOrdinal() << ",NONAME" << "\")" << std::endl;
			}
		}

		CurrentFunctionIndex++;
	}

	ASMFileOut << "END" << std::endl;
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