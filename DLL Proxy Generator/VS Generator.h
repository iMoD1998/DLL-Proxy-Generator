#pragma once

#include <string>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <basetsd.h>
#include <winnt.h>

class VSFile
{
public:
	VSFile( std::string Name )
		: Name( Name )
	{

	}

	virtual std::string GetEntryText() = 0;

	std::string Name;
};

class VSMASMFile : public VSFile
{
public:
	VSMASMFile( std::string Name )
		: VSFile( Name )
	{

	}

	virtual std::string GetEntryText()
	{
		std::stringstream ss;

		ss << "\t\t<MASM Include=\"" << this->Name << "\">" << std::endl;
		ss << "\t\t\t<FileType>Document</FileType>" << std::endl;
		ss << "\t\t\t<UseSafeExceptionHandlers Condition=\"\'$(Configuration)|$(Platform)\' == \'Debug|Win32\'\">true</UseSafeExceptionHandlers>" << std::endl;
		ss << "\t\t\t<UseSafeExceptionHandlers Condition=\"\'$(Configuration)|$(Platform)\' == \'Release|Win32\'\">true</UseSafeExceptionHandlers>" << std::endl;
		ss << "\t\t\t<UseSafeExceptionHandlers Condition=\"\'$(Configuration)|$(Platform)\' == \'Debug|x64\'\">false</UseSafeExceptionHandlers>" << std::endl;
		ss << "\t\t\t<UseSafeExceptionHandlers Condition=\"\'$(Configuration)|$(Platform)\' == \'Release|x64\'\">false</UseSafeExceptionHandlers>" << std::endl;
		ss << "\t\t</MASM>" << std::endl;

		return ss.str();
	}
};

class VSSourceFile : public VSFile
{
public:
	VSSourceFile( std::string Name )
		: VSFile( Name )
	{

	}

	virtual std::string GetEntryText()
	{
		return "\t\t<ClCompile Include=\"" + this->Name + "\"/>\n";
	}
};

class VSHeaderFile : public VSFile
{
public:
	VSHeaderFile( std::string Name )
		: VSFile( Name )
	{

	}

	virtual std::string GetEntryText()
	{
		return "\t\t<ClInclude Include=\"" + this->Name + "\"/>\n";
	}
};

class VSProjectConfig
{
public:
	VSProjectConfig( std::string IncludeName, std::string Type, std::string PlatformName )
		: IncludeName( IncludeName ), Type( Type ), PlatformName( PlatformName )
	{

	}

	std::string IncludeName;
	std::string Type;
	std::string PlatformName;
};

class VSGenerator
{
public:
	VSGenerator(std::string Name, std::filesystem::path OutDir, UINT16 MachineType)
		: Name( Name ), MachineType( MachineType ), OutDir( OutDir / Name )
	{
		std::filesystem::create_directories( OutDir / Name );
	}

	std::vector< VSProjectConfig > GetConfigs()
	{
		std::vector<VSProjectConfig> Configs;

		switch ( MachineType )
		{
			case IMAGE_FILE_MACHINE_AMD64:
				Configs.push_back( VSProjectConfig( "Debug|x64", "Debug", "x64" ) );
				Configs.push_back( VSProjectConfig( "Release|x64", "Release", "x64" ) );
				break;
			case IMAGE_FILE_MACHINE_I386:
				Configs.push_back( VSProjectConfig( "Debug|Win32", "Debug", "Win32" ) );
				Configs.push_back( VSProjectConfig( "Release|Win32", "Release", "Win32" ) );
				break;
			default:
				printf( "Unknown machine type %04X\n", MachineType );
				return Configs;
		}

		return Configs;
	}

	bool GenerateProjectFile(std::filesystem::path Out)
	{
		std::ofstream Stream;

		Stream.open( Out );

		if ( !Stream.is_open() )
			return false;

		Stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
		Stream << "<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">" << std::endl;
		Stream << "\t<ItemGroup Label=\"ProjectConfigurations\">" << std::endl;

		auto Configs = this->GetConfigs();

		if ( Configs.size() == 0 )
			return false;

		for ( auto& Config : Configs )
		{
			Stream << "\t<ProjectConfiguration Include=\"" << Config.IncludeName << "\">" << std::endl;
			Stream << "\t\t<Configuration>" << Config.Type << "</Configuration>" << std::endl;
			Stream << "\t\t<Platform>" << Config.PlatformName << "</Platform>" << std::endl;
			Stream << "\t</ProjectConfiguration>" << std::endl;
		}

		Stream << "\t</ItemGroup>" << std::endl;

		Stream << "\t<PropertyGroup Label=\"Globals\">" << std::endl;
		Stream << "\t<VCProjectVersion>16.0</VCProjectVersion>" << std::endl;
		Stream << "\t<ProjectGuid>{3ff28f45-6628-4023-bba2-734b67252be9}</ProjectGuid>" << std::endl;
		Stream << "\t<RootNamespace>{3ff28f45-6628-4023-bba2-734b67252be9}</RootNamespace>" << std::endl;
		Stream << "\t<WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>" << std::endl;
		Stream << "\t</PropertyGroup>" << std::endl;

		Stream << "\t<Import Project=\"$(VCTargetsPath)\Microsoft.Cpp.Default.props\"/>" << std::endl;

		for ( auto& Config : Configs )
		{
			if ( Config.Type == "Debug" )
			{
				Stream << "\t<PropertyGroup Condition=\"'$(Configuration)|$(Platform)\'==\'" << Config.IncludeName << "\'\" Label=\"Configuration\">" << std::endl;
				Stream << "\t\t<ConfigurationType>DynamicLibrary</ConfigurationType>" << std::endl;
				Stream << "\t\t<UseDebugLibraries>true</UseDebugLibraries>" << std::endl;
				Stream << "\t\t<PlatformToolset>v142</PlatformToolset>" << std::endl;
				Stream << "\t\t<CharacterSet>Unicode</CharacterSet>" << std::endl;
				Stream << "\t</PropertyGroup>" << std::endl;
			}
			else
			{
				Stream << "\t<PropertyGroup Condition=\"'$(Configuration)|$(Platform)\'==\'" << Config.IncludeName << "\'\" Label=\"Configuration\">" << std::endl;
				Stream << "\t\t<ConfigurationType>DynamicLibrary</ConfigurationType>" << std::endl;
				Stream << "\t\t<UseDebugLibraries>false</UseDebugLibraries>" << std::endl;
				Stream << "\t\t<PlatformToolset>v142</PlatformToolset>" << std::endl;
				Stream << "\t\t<WholeProgramOptimization>true</WholeProgramOptimization>" << std::endl;
				Stream << "\t\t<CharacterSet>Unicode</CharacterSet>" << std::endl;
				Stream << "\t</PropertyGroup>" << std::endl;
			}
		}

		Stream << "\t<Import Project=\"$(VCTargetsPath)\Microsoft.Cpp.props\" />" << std::endl;
		Stream << "\t<ImportGroup Label=\"ExtensionSettings\">" << std::endl;
		Stream << "\t\t<Import Project=\"$(VCTargetsPath)\\BuildCustomizations\\masm.props\"/>" << std::endl;
		Stream << "\t</ImportGroup>" << std::endl;
		Stream << "\t<ImportGroup Label=\"Shared\">" << std::endl;
		Stream << "\t</ImportGroup>" << std::endl;

		for ( auto& Config : Configs )
		{
			Stream << "\t<ImportGroup Label=\"PropertySheets\" Condition=\"\'$(Configuration)|$(Platform)\'==\'" << Config.IncludeName << "\'\">" << std::endl;
			Stream << "\t\t<Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists(\'$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\')\" Label=\"LocalAppDataPlatform\" />" << std::endl;
			Stream << "\t</ImportGroup>" << std::endl;
		}

		Stream << "\t<PropertyGroup Label=\"UserMacros\"\/>" << std::endl;

		for ( auto& Config : Configs )
		{
			Stream << "\t<PropertyGroup Condition=\"'$(Configuration)|$(Platform)\'==\'" << Config.IncludeName << "\'\" Label=\"Configuration\">" << std::endl;
			Stream << "\t\t<LinkIncremental>true</LinkIncremental>" << std::endl;
			Stream << "\t</PropertyGroup>" << std::endl;
		}

		for ( auto& Config : Configs )
		{
			Stream << "\t<ItemDefinitionGroup Condition=\"\'$(Configuration)|$(Platform)\' == \'" << Config.IncludeName << "\'\">" << std::endl;
			Stream << "\t\t<ClCompile>" << std::endl;
			Stream << "\t\t\t<WarningLevel>Level3</WarningLevel>" << std::endl;
			Stream << "\t\t\t<SDLCheck>true</SDLCheck>" << std::endl;
			Stream << "\t\t\t<ConformanceMode>true</ConformanceMode>" << std::endl;
			Stream << "\t\t\t<PrecompiledHeader>NotUsing</PrecompiledHeader>" << std::endl;
			Stream << "\t\t</ClCompile>" << std::endl;
			
			Stream << "\t\t<Link>" << std::endl;
			Stream << "\t\t\t<SubSystem>Windows</SubSystem>" << std::endl;

			if ( Config.Type == "Debug" )
				Stream << "\t\t\t<GenerateDebugInformation>true</GenerateDebugInformation>" << std::endl;
			else
				Stream << "\t\t\t<GenerateDebugInformation>false</GenerateDebugInformation>" << std::endl;

			if( DefinitionFile.size() > 0 )
				Stream << "\t\t\t<ModuleDefinitionFile>"<< DefinitionFile << "</ModuleDefinitionFile>" << std::endl;
			
			Stream << "\t\t</Link>" << std::endl;
			Stream << "\t</ItemDefinitionGroup>" << std::endl;
		}

		Stream << "\t<ItemGroup>" << std::endl;

		for ( auto File : this->Files )
		{
			Stream << File->GetEntryText() << std::endl;
		}

		Stream << "\t</ItemGroup>" << std::endl;

		if ( DefinitionFile.size() > 0 )
		{
			Stream << "\t<ItemGroup>" << std::endl;
			Stream << "\t\t<None Include=\"" << DefinitionFile << "\"/>" << std::endl;
			Stream << "\t</ItemGroup>" << std::endl;
		}

		Stream << "\t<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />" << std::endl;

		Stream << "\t<ImportGroup Label=\"ExtensionTargets\">" << std::endl;
		Stream << "\t\t<Import Project=\"$(VCTargetsPath)\\BuildCustomizations\\masm.targets\"/>" << std::endl;
		Stream << "\t</ImportGroup>" << std::endl;

		Stream << "</Project>" << std::endl;

		return true;
	}

	bool Generate()
	{
		/*TODO: Generate .sln?*/

		if ( !this->GenerateProjectFile( OutDir / Name += ".vcxproj" ) )
		{
			printf( "Failed To Generate Project File\n" );
			return false;
		}

		return true;
	}

	std::filesystem::path GetProjectPath() const
	{
		return this->OutDir;
	}

	template <typename T, typename... TArgs>
	inline std::shared_ptr< T > AddFile( TArgs&&... Args )
	{
		auto NewFile = std::make_shared<T>( std::forward<TArgs>( Args )... );

		this->Files.push_back( NewFile );

		return NewFile;
	}

	void SetDefinitionFile( std::string Name )
	{
		this->DefinitionFile = Name;
	}
	
protected:
	std::string Name;
	std::string DefinitionFile;
	std::filesystem::path OutDir;
	UINT16 MachineType;
	std::vector<std::shared_ptr<VSFile>> Files;
};