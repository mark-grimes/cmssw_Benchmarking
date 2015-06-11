#include "CheckRSSService.h"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <boost/algorithm/string.hpp> // For splitting up strings read from /proc/<pid>/statm
#include <DataFormats/Provenance/interface/ModuleDescription.h>
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/ActivityRegistry.h"

// The signals in ActivityRegistry changed drastically to cover threaded
// use, so I need to conditionally compile certain things depending on the
// version of CMSSW. I can't find any macros about the CMSSW version, so
// I'll just check one of the internal use ActivityRegistry macros. This
// wasn't defined in the old ActivityRegistry file (pre 7_4_something).
#ifdef AR_WATCH_USING_METHOD_3
#	define USE_NEW_ACTIVITYREGISTRY_SIGNALS
#	include "FWCore/ServiceRegistry/interface/ModuleCallingContext.h"
#endif

//
// Use the unnamed namespace for things only used in this file.
//
namespace
{
	struct MemoryUse
	{
		int rss; // VmRSS
		int size; // VmSize
	};

	// Okay, I know this is a global but I only ever set it once. Don't see the point
	// in constantly getting it from the system config. Should really use some kind of
	// std::call_once but there should only ever be one service instance, and it's only
	// modified in the constructor.
	int global_pageSizeInKb=0; // Set to zero so it's obvious if an uninitiated value is ever used.

	::MemoryUse getMemoryUse( const std::string& pid )
	{
		std::ifstream inputFile( "/proc/"+pid+"/statm" );
		if( !inputFile.is_open() ) throw std::runtime_error( "Unable to open /proc/"+pid+"/statm" );

	    std::string fileContents( std::istreambuf_iterator<char>(inputFile), (std::istreambuf_iterator<char>()) );
	    inputFile.close();

	    std::vector<std::string> columns;
	    boost::split(columns, fileContents, boost::is_any_of(" "));
	    if( columns.size()<2 ) throw std::runtime_error( "Unable to split the columns in /proc/"+pid+"/statm properly" );

	    // First column is size, second is RSS. See http://linux.die.net/man/5/proc.
	    // Note that statm reports in mutiples of the page size, so need to multiply
	    // by that.
	    return ::MemoryUse{ std::stoi(columns[1])*::global_pageSizeInKb, std::stoi(columns[0])*::global_pageSizeInKb };
	}

	float getSystemLoad()
	{
		std::ifstream inputFile( "/proc/loadavg" );
		if( !inputFile.is_open() ) throw std::runtime_error( "Unable to open /proc/loadavg" );

	    std::string fileContents( std::istreambuf_iterator<char>(inputFile), (std::istreambuf_iterator<char>()) );
	    inputFile.close();

	    std::vector<std::string> columns;
	    boost::split(columns, fileContents, boost::is_any_of(" "));
	    if( columns.size()<1 ) throw std::runtime_error( "Unable to split the columns in /proc/loadavg properly" );

	    // First column is the system load averaged over the last minute. See http://linux.die.net/man/5/proc
	    return std::stof(columns[0]);
	}

	/** @brief Dumps the current RSS and VmSize to std out, prefixed by the given string */
	void dumpRSS( std::ostream& output, const std::string& pid, const std::string& prefix )
	{
		::MemoryUse currentUsage=::getMemoryUse( pid );
		float systemLoad=getSystemLoad();

		output << prefix << " RSS/KiB " << currentUsage.rss << " Size/KiB " << currentUsage.size << " Load " << systemLoad << "\n";
	}

	void dumpRSSForModuleDescription( const edm::ModuleDescription& description, const std::string& pid, std::function<std::string()> methodNameFunctor )
	{
		::dumpRSS( std::cout, pid, " *RSSDUMP* "+methodNameFunctor()+" "+description.moduleLabel()+" "+description.moduleName() );
	}

#ifdef USE_NEW_ACTIVITYREGISTRY_SIGNALS
	void dumpRSSForCallingContext( edm::ModuleCallingContext const& mcc, const std::string& pid, std::function<std::string()> methodNameFunctor )
	{
		::dumpRSSForModuleDescription( *mcc.moduleDescription(), pid, methodNameFunctor );
	}
#endif

}


markstools::services::CheckRSSService::CheckRSSService( const edm::ParameterSet& parameterSet, edm::ActivityRegistry& activityRegister )
	: eventNumber_(0), runNumber_(0), lumiNumber_(0)
{
	std::string pid=std::to_string( getpid() );
	::global_pageSizeInKb=sysconf(_SC_PAGESIZE)/1024;

	activityRegister.watchPreModuleConstruction( std::bind( &::dumpRSSForModuleDescription, std::placeholders::_1, pid, []{return "Start_Construction";} )  );
	activityRegister.watchPostModuleConstruction( std::bind( &::dumpRSSForModuleDescription, std::placeholders::_1, pid, []{return "End_Construction";} ) );

	activityRegister.watchPreModuleBeginJob( std::bind( &::dumpRSSForModuleDescription, std::placeholders::_1, pid, []{return "Start_BeginJob";} ) );
	activityRegister.watchPostModuleBeginJob( std::bind( &::dumpRSSForModuleDescription, std::placeholders::_1, pid, []{return "End_BeginJob";} ) );

	activityRegister.watchPreModuleEndJob( std::bind( &::dumpRSSForModuleDescription, std::placeholders::_1, pid, []{return "Start_EndJob";} ) );
	activityRegister.watchPostModuleEndJob( std::bind( &::dumpRSSForModuleDescription, std::placeholders::_1, pid, []{return "End_EndJob";} ) );

#ifdef USE_NEW_ACTIVITYREGISTRY_SIGNALS
	activityRegister.watchPostEvent( [&](edm::StreamContext const&){++eventNumber_;} );
	activityRegister.watchPostGlobalEndRun( [&](edm::GlobalContext const&){++runNumber_;} );
	activityRegister.watchPostGlobalEndLumi( [&](edm::GlobalContext const&){++lumiNumber_;} );

	activityRegister.watchPreModuleEvent( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "Start_Event"+std::to_string(eventNumber_);} ) );
	activityRegister.watchPostModuleEvent( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "End_Event"+std::to_string(eventNumber_);} ) );

	activityRegister.watchPreModuleBeginStream( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, []{return "Start_ModuleBeginStream";} ) );
	activityRegister.watchPostModuleBeginStream( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, []{return "End_ModuleBeginStream";} ) );
	activityRegister.watchPreModuleEndStream( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, []{return "Start_ModuleEndStream";} ) );
	activityRegister.watchPostModuleEndStream( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, []{return "End_ModuleEndStream";} ) );

	activityRegister.watchPreModuleStreamBeginRun( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "Start_ModuleStreamBeginRun"+std::to_string(runNumber_);} ) );
	activityRegister.watchPostModuleStreamBeginRun( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "End_ModuleStreamBeginRun"+std::to_string(runNumber_);} ) );
	activityRegister.watchPreModuleStreamEndRun( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "Start_ModuleStreamEndRun"+std::to_string(runNumber_);} ) );
	activityRegister.watchPostModuleStreamEndRun( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "End_ModuleStreamEndRun"+std::to_string(runNumber_);} ) );

	activityRegister.watchPreModuleStreamBeginLumi( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "Start_ModuleStreamBeginLumi"+std::to_string(lumiNumber_);} ) );
	activityRegister.watchPostModuleStreamBeginLumi( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "End_ModuleStreamBeginLumi"+std::to_string(lumiNumber_);} ) );
	activityRegister.watchPreModuleStreamEndLumi( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "Start_ModuleStreamEndLumi"+std::to_string(lumiNumber_);} ) );
	activityRegister.watchPostModuleStreamEndLumi( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "End_ModuleStreamEndLumi"+std::to_string(lumiNumber_);} ) );

	activityRegister.watchPreModuleGlobalBeginRun( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "Start_ModuleGlobalBeginRun"+std::to_string(runNumber_);} ) );
	activityRegister.watchPostModuleGlobalBeginRun( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "End_ModuleGlobalBeginRun"+std::to_string(runNumber_);} ) );
	activityRegister.watchPreModuleGlobalEndRun( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "Start_ModuleGlobalEndRun"+std::to_string(runNumber_);} ) );
	activityRegister.watchPostModuleGlobalEndRun( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "End_ModuleGlobalEndRun"+std::to_string(runNumber_);} ) );

	activityRegister.watchPreModuleGlobalBeginLumi( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "Start_ModuleGlobalBeginLumi"+std::to_string(lumiNumber_);} ) );
	activityRegister.watchPostModuleGlobalBeginLumi( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "End_ModuleGlobalBeginLumi"+std::to_string(lumiNumber_);} ) );
	activityRegister.watchPreModuleGlobalEndLumi( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "Start_ModuleGlobalEndLumi"+std::to_string(lumiNumber_);} ) );
	activityRegister.watchPostModuleGlobalEndLumi( std::bind( &::dumpRSSForCallingContext, std::placeholders::_2, pid, [&]{return "End_ModuleGlobalEndLumi"+std::to_string(lumiNumber_);} ) );
#else
	activityRegister.watchPostProcessEvent( [&](const edm::Event&,const edm::EventSetup&){++eventNumber_;} );
	activityRegister.watchPostEndLumi( [&](edm::LuminosityBlock const&, edm::EventSetup const&){++lumiNumber_;} );
	activityRegister.watchPostEndRun( [&](edm::Run const&, edm::EventSetup const&){++runNumber_;} );

	activityRegister.watchPreModuleBeginRun( std::bind( &::dumpRSSForModuleDescription, std::placeholders::_1, pid, [&]{return "Start_BeginRun"+std::to_string(runNumber_);} ) );
	activityRegister.watchPostModuleBeginRun( std::bind( &::dumpRSSForModuleDescription, std::placeholders::_1, pid, [&]{return "End_BeginRun"+std::to_string(runNumber_);} ) );

	activityRegister.watchPreModuleBeginLumi( std::bind( &::dumpRSSForModuleDescription, std::placeholders::_1, pid, [&]{return "Start_BeginLumi"+std::to_string(lumiNumber_);} ) );
	activityRegister.watchPostModuleBeginLumi( std::bind( &::dumpRSSForModuleDescription, std::placeholders::_1, pid, [&]{return "End_BeginLumi"+std::to_string(lumiNumber_);} ) );

	activityRegister.watchPreModule( std::bind( &::dumpRSSForModuleDescription, std::placeholders::_1, pid, [&]{return "Start_Event"+std::to_string(eventNumber_);} ) );
	activityRegister.watchPostModule( std::bind( &::dumpRSSForModuleDescription, std::placeholders::_1, pid, [&]{return "End_Event"+std::to_string(eventNumber_);} ) );

	activityRegister.watchPreModuleEndLumi( std::bind( &::dumpRSSForModuleDescription, std::placeholders::_1, pid, [&]{return "Start_EndLumi"+std::to_string(lumiNumber_);} ) );
	activityRegister.watchPostModuleEndLumi( std::bind( &::dumpRSSForModuleDescription, std::placeholders::_1, pid, [&]{return "End_EndLumi"+std::to_string(lumiNumber_);} ) );

	activityRegister.watchPreModuleEndRun( std::bind( &::dumpRSSForModuleDescription, std::placeholders::_1, pid, [&]{return "Start_EndRun"+std::to_string(runNumber_);} ) );
	activityRegister.watchPostModuleEndRun( std::bind( &::dumpRSSForModuleDescription, std::placeholders::_1, pid, [&]{return "End_EndRun"+std::to_string(runNumber_);} ) );
#endif
}

markstools::services::CheckRSSService::~CheckRSSService()
{
	// No operation
}
