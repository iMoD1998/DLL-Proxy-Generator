#include <Windows.h>
#include <fstream>

#include "ExportEntry.h"

void GenerateDef( 
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

void GeneratePragmas(
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

		PragmaFileOut << "\")" << std::endl;
	}

}

int main(int argc, const char* argv[])
{
	std::vector<ExportEntry> Entries;

	std::filesystem::path DLLPath = "C:\\Windows\\System32\\user32.dll";

	if ( ExportEntry::GetExportEntries( DLLPath, Entries ) )
	{
		auto DLLName = DLLPath.filename().replace_extension( "" ).string();

		GenerateDef( DLLName, Entries );
		GeneratePragmas( DLLName, Entries );
	}

	getchar();

	return 0;
}