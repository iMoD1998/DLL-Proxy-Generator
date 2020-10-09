#pragma once

#include "Export Generator.h"

class DefFileGenerator : public ExportGenerator
{
public:
	DefFileGenerator( std::filesystem::path Path ) :
		ExportGenerator( Path )
	{

	}

	virtual bool Begin(
		_In_opt_ UINT16 MachineType     = 0,
		_In_opt_ SIZE_T NumberOfEntries = 0
	)
	{
		File << "LIBRARY" << std::endl;
		File << "EXPORTS" << std::endl;
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
			File << "\t" << Export.GetName() << std::endl;
		}
		else
		{
			File << "\t" << SymbolName << " @ " << Export.GetOrdinal() << " NONAME" << std::endl;
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
			File << "\t" << Export.GetName() << "=";
		}
		else
		{
			File << "\t#" << Export.GetOrdinal() << "=";
		}

		if ( Export.IsForwarded() )
		{
			File << Export.GetForwardedName();

			if ( !Export.HasName() )
			{
				File << " @ " << Export.GetOrdinal() << " NONAME";
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
				File << "#" << Export.GetOrdinal() << " @ " << Export.GetOrdinal() << " NONAME";
			}
		}

		File << std::endl;

		return true;
	}
};