#include <Windows.h>
#include <imagehlp.h>

#include <string>
#include <vector>

int main(int argc, const char* argv[])
{
	LOADED_IMAGE LoadedImage;

	if ( MapAndLoad( "C:\\Windows\\System32\\user32.dll", NULL, &LoadedImage, TRUE, TRUE ) )
	{
		if ( !( LoadedImage.FileHeader->FileHeader.Characteristics & IMAGE_FILE_DLL ) )
		{
			printf( "File Not A DLL" );

			UnMapAndLoad( &LoadedImage );
			
			return false;
		}

		ULONG ImageDirectorySize = 0;

		auto ImageExportDirectory = (IMAGE_EXPORT_DIRECTORY*)ImageDirectoryEntryToData( LoadedImage.MappedAddress, FALSE, IMAGE_DIRECTORY_ENTRY_EXPORT, &ImageDirectorySize );

		if ( ImageExportDirectory == NULL )
			return false;

		printf( "Characteristics    %08X\n", ImageExportDirectory->Characteristics );
		printf( "Time Date Stamp    %08X\n", ImageExportDirectory->TimeDateStamp );
		printf( "Ordinal Base       %i\n",   ImageExportDirectory->Base );
		printf( "Nuber Of Functions %i\n",   ImageExportDirectory->NumberOfFunctions );
		printf( "Nuber Of Names     %i\n",   ImageExportDirectory->NumberOfNames );
		printf( "Version            %hu.%02hu\n", ImageExportDirectory->MajorVersion, ImageExportDirectory->MinorVersion );

		auto FunctionArray    = (UINT32*)ImageRvaToVa( LoadedImage.FileHeader, LoadedImage.MappedAddress, ImageExportDirectory->AddressOfFunctions, NULL );
		auto NameOrdinalArray = (UINT16*)ImageRvaToVa( LoadedImage.FileHeader, LoadedImage.MappedAddress, ImageExportDirectory->AddressOfNameOrdinals, NULL );
		auto NameArray        = (UINT32*)ImageRvaToVa( LoadedImage.FileHeader, LoadedImage.MappedAddress, ImageExportDirectory->AddressOfNames, NULL );

		for ( UINT32 OrdinalIndex = 0; OrdinalIndex < ImageExportDirectory->NumberOfFunctions; OrdinalIndex++ )
		{
			auto        Ordinal         = ImageExportDirectory->Base + OrdinalIndex;
			auto        FunctionRVA     = FunctionArray[ OrdinalIndex ];
			auto        FunctionAddress = (UINT_PTR)ImageRvaToVa( LoadedImage.FileHeader, LoadedImage.MappedAddress, FunctionRVA, NULL );
			bool        HasName         = false;
			bool        Forwarded       = false;
			const char* ForwardedName   = "";
			const char* Name            = "";

			printf( "Ordinal: %4i", Ordinal );

			/*Try find this ordinal in the name ordinal array*/
			for ( UINT32 NameOrdinalIndex = 0; NameOrdinalIndex < ImageExportDirectory->NumberOfNames; NameOrdinalIndex++ )
			{
				if ( OrdinalIndex == NameOrdinalArray[ NameOrdinalIndex ] )
				{
					/*Found the ordinal in the name ordinal array now use that index in the name array*/
					Name = (const char*)ImageRvaToVa( LoadedImage.FileHeader, LoadedImage.MappedAddress, NameArray[ NameOrdinalIndex ], NULL );

					printf( " Name: %-60s", Name );
		
					HasName = true;

					break;
				}
			}

			if ( !HasName )
			{
				printf( " Name: %-60s", "[NONAME]" );
			}

			/* If function address is within the image export directory its a forwarded entry */
			if ( FunctionAddress >= (UINT_PTR)ImageExportDirectory &&
				FunctionAddress < ( (UINT_PTR)ImageExportDirectory + ImageDirectorySize ) )
			{
				ForwardedName = (const char*)FunctionAddress;

				printf( "RVA:   (Forwarded) ->  %-60s", ForwardedName );

				Forwarded = true;
			}
			else
			{
				printf( "RVA:   %08X  ", FunctionRVA );
				printf( "     %-60s", ForwardedName );
			}

			printf( "\n" );
		}

		UnMapAndLoad( &LoadedImage );
	}

	getchar();

	return 0;
}