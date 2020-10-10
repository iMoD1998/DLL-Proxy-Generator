#pragma once
#include "Export Generator.h"

class ASMFileGenerator : public ExportGenerator
{
public:
	ASMFileGenerator( std::filesystem::path Path ) :
		ExportGenerator( Path ), FunctionTableName( "" ), MachinePointerSize( 0 )
	{

	}

	virtual bool Begin(
		_In_opt_ UINT16 MachineType,
		_In_opt_ SIZE_T NumberOfEntries
	)
	{
		if ( NumberOfEntries == 0 )
			return false;

		switch ( MachineType )
		{
			case IMAGE_FILE_MACHINE_AMD64:
				File << ".DATA" << std::endl;
				File << "g_FunctionTable QWORD " << NumberOfEntries << " dup(?)" << std::endl;
				File << "PUBLIC g_FunctionTable" << std::endl << std::endl;
				FunctionTableName = "g_FunctionTable";
				MachinePointerSize = sizeof( UINT64 );
				break;
			case IMAGE_FILE_MACHINE_I386:
				File << ".MODEL FLAT" << std::endl;
				File << ".DATA" << std::endl;
				File << "_g_FunctionTable DWORD " << NumberOfEntries << " dup(?)" << std::endl;
				File << "PUBLIC _g_FunctionTable" << std::endl << std::endl;
				FunctionTableName = "_g_FunctionTable"; // shitty calling convention decoration
				MachinePointerSize = sizeof( UINT32 );
				break;
			default:
				printf( "Unknown machine type %04X\n", MachineType );
				return false;
		}

		File << ".CODE" << std::endl;

		return true;
	}

	virtual bool End()
	{
		File << "END" << std::endl;
		return true;
	}

	virtual bool AddExportEntry(
		_In_ const ExportEntry& Export,
		_In_ const std::string& SymbolName
	)
	{
		/*
			Generate Stub Like

			SymbolName PROC
				jmp [FunctionTableName + FunctionIndex * MachinePointerSize]
			SymbolName ENDP
		
		*/

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

			return false;
		}

		File << SymbolName << " PROC" << std::endl;
		File << "\tjmp [" << this->FunctionTableName << " + " << Export.GetOrdinalIndex() << " * " << this->MachinePointerSize << "]" << std::endl;
		File << SymbolName << " ENDP" << std::endl;
		File << std::endl;

		return true;
	}

	virtual bool AddForwardedExportEntry(
		_In_ const ExportEntry& Export,
		_In_ const std::string& DLLNameToForwardTo
	)
	{
		return false;
	}

	std::string GetFunctionTableName() const
	{
		return this->FunctionTableName;
	}

	SIZE_T GetPointerSize() const
	{
		return this->MachinePointerSize;
	}

protected:
	std::string FunctionTableName;
	SIZE_T      MachinePointerSize;
};