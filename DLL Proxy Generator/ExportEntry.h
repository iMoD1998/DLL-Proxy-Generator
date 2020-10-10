#pragma once

#include <Windows.h>
#include <imagehlp.h>
#include <string>
#include <vector>
#include <filesystem>

class ExportEntry
{
public:
	static bool IsRVAInDataSection(
		_In_ PLOADED_IMAGE Image,
		_In_ UINT32        RVA
	);

	static bool GetExportEntries(
		_In_  const std::filesystem::path& Path,
		_Out_ std::vector< ExportEntry >&  Entries,
		_In_  bool                         Verbose,
		_Out_ UINT16*                      MachineType
	);

	UINT32 GetOrdinal() const
	{
		return this->Ordinal;
	}

	UINT32 GetOrdinalIndex() const
	{
		return this->OrdinalIndex;
	}

	bool HasName() const
	{
		return this->Name.size() > 0;
	}

	bool IsForwarded() const
	{
		return this->ForwardedName.size() > 0;
	}

	bool IsData() const
	{
		return this->IsDataReference;
	}

	std::string GetName() const
	{
		if ( !this->HasName() )
			return "";

		return this->Name;
	}

	std::string GetForwardedName() const
	{
		if ( !this->IsForwarded() )
			return "";

		return this->ForwardedName;
	}

	UINT32 GetRVA() const
	{
		if ( this->IsForwarded() )
			return NULL;

		return this->RVA;
	}

	void Print() const
	{
		auto Name = this->GetName();

		if ( !this->HasName() )
			Name = "[NONAME]";

		printf( "Ordinal: %4i Name: %-60s", this->GetOrdinal(), Name.c_str() );

		if ( this->IsForwarded() )
		{
			printf( "RVA:   (Forwarded) ->  %-60s", ForwardedName.c_str() );
		}
		else
		{
			printf( "RVA:   %08X       %-60s", this->GetRVA(), "" );

			if ( this->IsData() )
			{
				printf( " [DATA]" );
			}
			else
			{
				printf( " [CODE]" );
			}
		}

		printf( "\n" );
	}

private:
	ExportEntry( 
		_In_ UINT32 Ordinal,
		_In_ UINT32 OrdinalIndex
	) : Ordinal( Ordinal ), OrdinalIndex( OrdinalIndex ), Name( "" ), ForwardedName( "" ), RVA( 0 ), IsDataReference(false)
	{

	}

	void SetIsData( bool IsData )
	{
		this->IsDataReference = IsData;
	}

	void SetFunctionRVA( UINT32 RVA )
	{
		this->RVA = RVA;
	}

	void SetName( const std::string& Name )
	{
		this->Name = Name;
	}

	void SetForwardedName( const std::string& ForwardedName )
	{
		this->ForwardedName = ForwardedName;
	}

	UINT32 Ordinal;
	UINT32 OrdinalIndex;
	std::string Name;
	std::string ForwardedName;
	UINT32 RVA;
	bool IsDataReference;
};