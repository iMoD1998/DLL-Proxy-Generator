#pragma once
#include <filesystem>
#include <fstream>

class DLLMainGenerator
{
public:
	DLLMainGenerator(
		_In_ std::filesystem::path Path
	) :	Path( Path )
	{

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

	void AddInclude( const std::string Text )
	{
		Includes += Text;
	}

	void AddBody( const std::string Text )
	{
		BodyText += Text;
	}

	void AddProcessAttach( const std::string Text )
	{
		ProcessAttachText += Text;
	}

	void AddProcessDetach( const std::string Text )
	{
		ProcessDetachText += Text;
	}

	void Write()
	{
		File << "#include <Windows.h>" << std::endl;

		File << BodyText << std::endl;

		File << "DWORD WINAPI ProcessAttach(\n\t_In_ LPVOID Parameter\n)\n{\n\tif ( Parameter == NULL )\n\t\treturn FALSE;\n" << std::endl;
		File << ProcessAttachText << "\n\treturn TRUE;\n}" << std::endl;

		File << "DWORD WINAPI ProcessDetach(\n\t_In_ LPVOID Parameter\n)\n{\n\tif ( Parameter == NULL )\n\t\treturn FALSE;\n" << std::endl;
		File << ProcessDetachText << "\n\treturn TRUE;\n}" << std::endl;

		File << "BOOL APIENTRY DllMain( \n\t_In_ HINSTANCE Instance,\n\t_In_ DWORD     Reason,\n\t_In_ LPVOID    Reserved \n)\n{\n\tswitch ( Reason )\n\t{\n\t\tcase DLL_PROCESS_ATTACH:\n\t\t\tDisableThreadLibraryCalls( Instance ); // Disable DLL_THREAD_ATTACH and DLL_THREAD_DETACH notifications\n\t\t\treturn ProcessAttach( Instance );\n\t\tcase DLL_PROCESS_DETACH:\n\t\t\treturn ProcessDetach( Instance );\n\t}\n\n\treturn TRUE;\n}\n";
	}

protected:
	std::filesystem::path Path;
	std::ofstream File;
	std::string Includes;
	std::string BodyText;
	std::string ProcessAttachText;
	std::string ProcessDetachText;
};