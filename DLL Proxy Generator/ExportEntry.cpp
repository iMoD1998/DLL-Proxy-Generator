#include "ExportEntry.h"

bool ExportEntry::IsRVAInDataSection( 
	_In_ PLOADED_IMAGE Image,
	_In_ UINT32 RVA 
)
{
	for ( ULONG SectionIndex = 0; SectionIndex < Image->NumberOfSections; SectionIndex++ )
	{
		const auto Section = Image->Sections[ SectionIndex ];

		if( RVA >= Section.VirtualAddress && 
			RVA <  Section.VirtualAddress + Section.Misc.VirtualSize )
		{
			/*Found section rva is in*/

			if ( Section.Characteristics & IMAGE_SCN_CNT_CODE )
				return false;

			return true;
		}
	}

	return false;
}

bool ExportEntry::GetExportEntries(
	_In_  const std::filesystem::path& Path,
	_Out_ std::vector< ExportEntry >&  Entries
)
{
	LOADED_IMAGE LoadedImage;

	if ( !std::filesystem::exists( Path ) )
	{
		printf( "File doesnt exist\n" );
		return false;
	}

	if ( MapAndLoad( Path.string().c_str(), NULL, &LoadedImage, TRUE, TRUE ) )
	{
		if ( !( LoadedImage.FileHeader->FileHeader.Characteristics & IMAGE_FILE_DLL ) )
		{
			printf( "File Not A DLL\n" );

			UnMapAndLoad( &LoadedImage );

			return false;
		}

		ULONG ImageDirectorySize = 0;

		auto ImageExportDirectory = (IMAGE_EXPORT_DIRECTORY*)ImageDirectoryEntryToData( LoadedImage.MappedAddress, FALSE, IMAGE_DIRECTORY_ENTRY_EXPORT, &ImageDirectorySize );

		if ( ImageExportDirectory == NULL )
			return false;

		printf( "Characteristics    %08X\n",      ImageExportDirectory->Characteristics );
		printf( "Time Date Stamp    %08X\n",      ImageExportDirectory->TimeDateStamp );
		printf( "Ordinal Base       %i\n",        ImageExportDirectory->Base );
		printf( "Nuber Of Functions %i\n",        ImageExportDirectory->NumberOfFunctions );
		printf( "Nuber Of Names     %i\n",        ImageExportDirectory->NumberOfNames );
		printf( "Version            %hu.%02hu\n", ImageExportDirectory->MajorVersion, ImageExportDirectory->MinorVersion );
	
		auto FunctionArray    = (UINT32*)ImageRvaToVa( LoadedImage.FileHeader, LoadedImage.MappedAddress, ImageExportDirectory->AddressOfFunctions, NULL );
		auto NameOrdinalArray = (UINT16*)ImageRvaToVa( LoadedImage.FileHeader, LoadedImage.MappedAddress, ImageExportDirectory->AddressOfNameOrdinals, NULL );
		auto NameArray        = (UINT32*)ImageRvaToVa( LoadedImage.FileHeader, LoadedImage.MappedAddress, ImageExportDirectory->AddressOfNames, NULL );

		for ( UINT32 OrdinalIndex = 0; OrdinalIndex < ImageExportDirectory->NumberOfFunctions; OrdinalIndex++ )
		{
			auto Export          = ExportEntry( ImageExportDirectory->Base + OrdinalIndex );
			auto FunctionRVA     = FunctionArray[ OrdinalIndex ];
			auto FunctionAddress = (UINT_PTR)ImageRvaToVa( LoadedImage.FileHeader, LoadedImage.MappedAddress, FunctionRVA, NULL );

			for ( UINT32 NameOrdinalIndex = 0; NameOrdinalIndex < ImageExportDirectory->NumberOfNames; NameOrdinalIndex++ )
			{
				if ( OrdinalIndex == NameOrdinalArray[ NameOrdinalIndex ] )
				{
					/*Found the ordinal in the name ordinal array now use that index in the name array*/
					
					auto Name = (const char*)ImageRvaToVa( LoadedImage.FileHeader, LoadedImage.MappedAddress, NameArray[ NameOrdinalIndex ], NULL );

					if ( Name == NULL )
					{
						printf( "ERROR: Ordinal %i had invalid name\n", Export.GetOrdinal() );
						return false;
					}

					Export.SetName( Name );

					break;
				}
			}

			/* If function address is within the image export directory its a forwarded entry */
			if ( FunctionAddress >=   (UINT_PTR)ImageExportDirectory &&
				 FunctionAddress <  ( (UINT_PTR)ImageExportDirectory + ImageDirectorySize ) )
			{
				auto ForwardedName = (const char*)FunctionAddress;

				if ( ForwardedName == NULL )
				{
					printf( "ERROR: Ordinal %i had invalid forwarded name\n", Export.GetOrdinal() );
					return false;
				}

				Export.SetForwardedName( ForwardedName );
			}
			else
			{
				if ( FunctionRVA == NULL )
					continue; // Ordinal not used

				Export.SetFunctionRVA( FunctionRVA );
			}

			if ( !Export.IsForwarded() )
			{
				Export.SetIsData( ExportEntry::IsRVAInDataSection( &LoadedImage, Export.GetRVA() ) );
			}

			Export.Print();

			Entries.push_back( Export );
		}

		UnMapAndLoad( &LoadedImage );

		return true;
	}

	return false;
}