#pragma once

#include "Export Generator.h"

class PragmaFileGenerator : public ExportGenerator
{
public:
	PragmaFileGenerator( std::filesystem::path Path ) :
		ExportGenerator( Path )
	{

	}

	virtual bool Begin(
		_In_opt_ UINT16 MachineType     = 0,
		_In_opt_ SIZE_T NumberOfEntries = 0
	)
	{
		return true;
	}

	virtual bool End()
	{
		return true;
	}

	virtual bool AddExportEntry(
		_In_ const ExportEntry& Export,
		_In_ const std::string& SymbolName
	)
	{
		if ( Export.HasName() )
		{
			File << "#pragma comment(linker,\"/export:" << Export.GetName() << "\")" << std::endl;
		}
		else
		{
			File << "#pragma comment(linker,\"/export:" << SymbolName << ",@" << Export.GetOrdinal() << ",NONAME" << "\")" << std::endl;
		}

		return true;
	}

	virtual bool AddForwardedExportEntry(
		_In_ const ExportEntry& Export,
		_In_ const std::string& DLLNameToForwardTo
	)
	{
		if ( Export.HasName() )
		{
			File << "#pragma comment(linker,\"/export:" << Export.GetName() << "=";
		}
		else
		{
			File << "#pragma comment(linker,\"/export:#" << Export.GetOrdinal() << "=";
		}

		if ( Export.IsForwarded() )
		{
			File << Export.GetForwardedName();

			if ( !Export.HasName() )
			{
				File << ",@" << Export.GetOrdinal() << ",NONAME";
			}
		}
		else
		{
			File << DLLNameToForwardTo << ".";

			if ( Export.HasName() )
			{
				File << Export.GetName();
			}
			else
			{
				File << "#" << Export.GetOrdinal() << ",@" << Export.GetOrdinal() << ",NONAME";
			}
		}

		if ( Export.IsData() )
		{
			File << ",DATA";
		}

		File << "\")" << std::endl;

		return true;
	}
};