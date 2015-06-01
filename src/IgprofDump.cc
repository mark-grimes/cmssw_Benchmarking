#include "MarksTools/Benchmarking/interface/IgprofDump.h"
#include <boost/filesystem/operations.hpp>

#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <fstream>
#include "DataFormats/Provenance/interface/ModuleDescription.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/ActivityRegistry.h"
// The signals in ActivityRegistry changed drastically to cover threaded
// use, so I need to conditionally compile certain things depending on the
// version of CMSSW. I can't find any macros about the CMSSW version, so
// I'll just check one of the internal use ActivityRegistry macros. This
// wasn't defined in the old ActivityRegistry file (pre 7_4_something).
#ifdef AR_WATCH_USING_METHOD_3
#	define IGPROFDUMP_USE_NEW_ACTIVITYREGISTRY_SIGNALS
#	include "FWCore/ServiceRegistry/interface/ModuleCallingContext.h"
#endif

//
// Use the unnamed namespace for things only used in this file
//
namespace
{
	/** @brief When dumps should be made for specific modules */
	struct ModuleDumpOptions
	{
		std::string moduleName;
		std::vector<int> eventStartNumbers;
		std::vector<int> eventEndNumbers;
	};

}

//
// Define the pimple class
//
namespace markstools
{
	namespace services
	{
		struct IgprofDumpPimple
		{
			boost::filesystem::path filenameToTouch_;
			boost::filesystem::path filenameOfIgprofDump_;
			std::chrono::milliseconds timeToSleepAfterTouch_;
			size_t eventNumber_;
			size_t userDumps_; ///< The number of times "dumpNow" has been called. Used to create a unique filename.

			IgprofDumpPimple() : timeToSleepAfterTouch_(500), eventNumber_(0), userDumps_(0) {}
			void touchFileAndMoveDump( const std::string& dumpNameSuffix );

			// At some point I'll expand this to be able to check multiple modules and conditions
			::ModuleDumpOptions dumpOptions_;

			void checkWhetherToDump( const edm::ModuleDescription& description, bool isEventStart );

#ifdef IGPROFDUMP_USE_NEW_ACTIVITYREGISTRY_SIGNALS
			void checkWhetherToDumpWithStream( edm::ModuleCallingContext const& mcc, bool isEventStart )
			{
				return checkWhetherToDump( *mcc.moduleDescription(), isEventStart );
			}
#endif
		}; // end of the PlottingTimerPimple class

	} // end of the markstools::services namespace
} // end of the markstools namespace

void markstools::services::IgprofDumpPimple::touchFileAndMoveDump( const std::string& dumpNameSuffix )
{
	try
	{
//		// First touch the file to make igprof do a dump
//		std::time_t n=std::time(0);
//		boost::filesystem::last_write_time( filenameToTouch_, n );
		std::ofstream watchFile( filenameToTouch_.native(), std::ios_base::app );
		watchFile.close();

		// Not sure if I should sleep here to give igprof time to make the dump?
		std::this_thread::sleep_for(timeToSleepAfterTouch_);

		// Now move the dump file so that it's not overwritten but subsequent dumps
		boost::filesystem::path newFilename=filenameOfIgprofDump_;
		newFilename+=dumpNameSuffix;
		boost::filesystem::rename( filenameOfIgprofDump_, newFilename );
	}
	catch( std::exception& error )
	{
		edm::LogError("IgprofDump") << "Unable to make igprof dump, got the exception: " << error.what();
	}
}

void markstools::services::IgprofDumpPimple::checkWhetherToDump( const edm::ModuleDescription& description, bool isEventStart )
{
	if( description.moduleLabel()!=dumpOptions_.moduleName ) return;

	if( isEventStart )
	{
		std::vector<int>& vector=dumpOptions_.eventStartNumbers;
		if( std::count(vector.begin(),vector.end(),eventNumber_) != 0 ) touchFileAndMoveDump( "StartEvent"+std::to_string(eventNumber_) );
	}
	else
	{
		std::vector<int>& vector=dumpOptions_.eventEndNumbers;
		if( std::count(vector.begin(),vector.end(),eventNumber_) != 0 ) touchFileAndMoveDump( "EndEvent"+std::to_string(eventNumber_) );
	}
}

markstools::services::IgprofDump::IgprofDump( const edm::ParameterSet& parameterSet, edm::ActivityRegistry& activityRegister )
	: pImple_( new markstools::services::IgprofDumpPimple )
{
	pImple_->filenameToTouch_=parameterSet.getParameter<std::string>("igprofWatchFile");
	pImple_->filenameOfIgprofDump_=parameterSet.getParameter<std::string>("igprofDump");

	// Now see when to create the dumps
	pImple_->dumpOptions_.moduleName=parameterSet.getParameter<std::string>("moduleName");
	pImple_->dumpOptions_.eventStartNumbers=parameterSet.getParameter< std::vector<int> >("eventStartNumbers");
	pImple_->dumpOptions_.eventEndNumbers=parameterSet.getParameter< std::vector<int> >("eventEndNumbers");

#ifdef IGPROFDUMP_USE_NEW_ACTIVITYREGISTRY_SIGNALS
	activityRegister.watchPreModuleEvent( std::bind( &IgprofDumpPimple::checkWhetherToDumpWithStream, pImple_, std::placeholders::_2, true ) );
	activityRegister.watchPostModuleEvent( std::bind( &IgprofDumpPimple::checkWhetherToDumpWithStream, pImple_, std::placeholders::_2, false ) );
	activityRegister.watchPostEvent( [&](edm::StreamContext const&){++pImple_->eventNumber_;} );
#endif
}

markstools::services::IgprofDump::~IgprofDump()
{
	delete pImple_;
}

void markstools::services::IgprofDump::dumpNow( const std::string dumpNameSuffix )
{
	++pImple_->userDumps_;
	if( !dumpNameSuffix.empty() ) pImple_->touchFileAndMoveDump( dumpNameSuffix );
	else pImple_->touchFileAndMoveDump( "userDump"+std::to_string(pImple_->userDumps_) );
}
