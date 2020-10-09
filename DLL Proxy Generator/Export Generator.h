#pragma once

#include <fstream>
#include <filesystem>
#include <basetsd.h>
#include "ExportEntry.h"

class ExportGenerator
{
public:
	ExportGenerator(std::filesystem::path Path) :
		Path( Path )
	{
		/*Guess this can fail but we will handle outside to make sure path exists etc*/
	}

	bool Open()
	{
		File.open( Path.c_str() );

		if ( !File.is_open() )
		{
			printf( "Failed to open file", Path.string().c_str() );
			return false;
		}

		return true;
	}

	virtual bool Begin( 
		_In_opt_ UINT16 MachineType,
		_In_opt_ SIZE_T NumberOfEntries
	) = 0;
	
	virtual bool End() = 0;

	virtual bool AddExportEntry( 
		_In_ const ExportEntry& Export,
		_In_ const std::string& SymbolName
	) = 0;
	
	virtual bool AddForwardedExportEntry( 
		_In_ const ExportEntry& Export,
		_In_ const std::string& DLLNameToForwardTo
	) = 0;

protected:
	std::filesystem::path Path;
	std::ofstream File;
};