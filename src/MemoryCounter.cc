#include "MarksTools/Benchmarking/interface/MemoryCounter.h"

#include <DataFormats/Provenance/interface/ModuleDescription.h>
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/ActivityRegistry.h"

// The signals in ActivityRegistry changed drastically to cover threaded
// use, so I need to conditionally compile certain things depending on the
// version of CMSSW. I can't find any macros about the CMSSW version, so
// I'll just check one of the internal use ActivityRegistry macros. This
// wasn't defined in the old ActivityRegistry file (pre 7_4_something).
#ifdef AR_WATCH_USING_METHOD_3
#	define MEMORYCOUNTER_USE_NEW_ACTIVITYREGISTRY_SIGNALS
#	include "FWCore/ServiceRegistry/interface/ModuleCallingContext.h"
#endif

#include <dlfcn.h>

#include <iostream>
#include <fstream>

// This is the interface from the memory counter program. The include location is set in
// the BuildFile.xml.
#include <memcounter/IMemoryCounter.h>

//
// unnamed namespace for things only used in this file
//
namespace
{
	struct ModuleDetails
	{
		memcounter::IMemoryCounter* pMemoryCounter;
		long int previousRecordedSize;
		std::string previousEvent;
		ModuleDetails() : pMemoryCounter(nullptr), previousRecordedSize(-1) {}
		ModuleDetails( memcounter::IMemoryCounter* pNewCounter ) : pMemoryCounter(pNewCounter), previousRecordedSize(-1) {}
	};

} // end of the unnamed namespace

//
// Define the pimple class
//
namespace markstools
{
	namespace services
	{
		class MemoryCounterPimple
		{
		public:
			MemoryCounterPimple() : eventNumber_(1), lumiNumber_(1), runNumber_(1), verbose_(false), createNewMemoryCounter(NULL) {}
			std::map<std::string,::ModuleDetails> memoryCounters_;
			size_t eventNumber_;
			size_t lumiNumber_;
			size_t runNumber_;
			std::vector<std::string> modulesToAnalyse_;
			bool verbose_;
			memcounter::IMemoryCounter* (*createNewMemoryCounter)( void );
			std::map<memcounter::IMemoryCounter*,long int> previousRecordedSize_; // The size recorded for the previous event. Used to calculate event content size.
		public:
			void enableMemoryCounter( const edm::ModuleDescription& description, std::function<std::string()> methodNameFunctor );
			void disableMemoryCounterAndPrint( const edm::ModuleDescription& description, std::function<std::string()> methodNameFunctor );


#ifdef MEMORYCOUNTER_USE_NEW_ACTIVITYREGISTRY_SIGNALS
			void enableMemoryCounterForStreams( edm::ModuleCallingContext const& mcc, std::function<std::string()> methodNameFunctor )
			{
				enableMemoryCounter( *mcc.moduleDescription(), methodNameFunctor );
			}
			void disableMemoryCounterAndPrintForStreams( edm::ModuleCallingContext const& mcc, std::function<std::string()> methodNameFunctor )
			{
				disableMemoryCounterAndPrint( *mcc.moduleDescription(), methodNameFunctor );
			}
#endif
			void preModuleConstruction( const edm::ModuleDescription& description );
		}; // end of the PlottingTimerPimple class

	} // end of the markstools::services namespace
} // end of the markstools namespace

markstools::services::MemoryCounter::MemoryCounter( const edm::ParameterSet& parameterSet, edm::ActivityRegistry& activityRegister )
	: pImple_(new MemoryCounterPimple)
{
	//
	// Try and get the interface to the memory counter from the preloaded library.
	//
	using memcounter::IMemoryCounter;
	if( void *sym = dlsym(0, "createNewMemoryCounter") )
	{
		pImple_->createNewMemoryCounter = __extension__(IMemoryCounter*(*)(void)) sym;

		//
		// Get some preferences from the config file and decide which modules to analyse
		//
		if( parameterSet.exists("modulesToAnalyse") ) pImple_->modulesToAnalyse_=parameterSet.getParameter< std::vector<std::string> >("modulesToAnalyse");
		else std::cout << "MemoryCounter: the parameter \"modulesToAnalyse\" has not been set, so MemoryCounter will analyse all modules" << std::endl;

		if( parameterSet.exists("verbose") ) pImple_->verbose_=parameterSet.getParameter<bool>("verbose");

		//
		// Register all of the watching functions
		//
		activityRegister.watchPreModuleConstruction( pImple_, &MemoryCounterPimple::preModuleConstruction );
		activityRegister.watchPostModuleConstruction( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrint, pImple_, std::placeholders::_1, []{return "Construction";} ) );

		activityRegister.watchPreModuleBeginJob( std::bind( &MemoryCounterPimple::enableMemoryCounter, pImple_, std::placeholders::_1, []{return "beginJob";} ) );
		activityRegister.watchPostModuleBeginJob( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrint, pImple_, std::placeholders::_1, []{return "beginJob";} ) );

#ifdef MEMORYCOUNTER_USE_NEW_ACTIVITYREGISTRY_SIGNALS
		activityRegister.watchPreModuleEvent( std::bind( &MemoryCounterPimple::enableMemoryCounterForStreams, pImple_, std::placeholders::_2, [&]{return "event"+std::to_string(pImple_->eventNumber_);} ) );
		activityRegister.watchPostModuleEvent( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrintForStreams, pImple_, std::placeholders::_2, [&]{return "event"+std::to_string(pImple_->eventNumber_);} ) );
		activityRegister.watchPostEvent( [&](edm::StreamContext const&){++pImple_->eventNumber_;} );

		activityRegister.watchPreModuleBeginStream( std::bind( &MemoryCounterPimple::enableMemoryCounterForStreams, pImple_, std::placeholders::_2, []{return "ModuleBeginStream";} ) );
		activityRegister.watchPostModuleBeginStream( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrintForStreams, pImple_, std::placeholders::_2, []{return "ModuleBeginStream";} ) );
		activityRegister.watchPreModuleEndStream( std::bind( &MemoryCounterPimple::enableMemoryCounterForStreams, pImple_, std::placeholders::_2, []{return "ModuleEndStream";} ) );
		activityRegister.watchPostModuleEndStream( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrintForStreams, pImple_, std::placeholders::_2, []{return "ModuleEndStream";} ) );

		activityRegister.watchPreModuleStreamBeginRun( std::bind( &MemoryCounterPimple::enableMemoryCounterForStreams, pImple_, std::placeholders::_2, []{return "ModuleStreamBeginRun";} ) );
		activityRegister.watchPostModuleStreamBeginRun( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrintForStreams, pImple_, std::placeholders::_2, []{return "ModuleStreamBeginRun";} ) );
		activityRegister.watchPreModuleStreamEndRun( std::bind( &MemoryCounterPimple::enableMemoryCounterForStreams, pImple_, std::placeholders::_2, []{return "ModuleStreamEndRun";} ) );
		activityRegister.watchPostModuleStreamEndRun( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrintForStreams, pImple_, std::placeholders::_2, []{return "ModuleStreamEndRun";} ) );

		activityRegister.watchPreModuleStreamBeginLumi( std::bind( &MemoryCounterPimple::enableMemoryCounterForStreams, pImple_, std::placeholders::_2, []{return "ModuleStreamBeginLumi";} ) );
		activityRegister.watchPostModuleStreamBeginLumi( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrintForStreams, pImple_, std::placeholders::_2, []{return "ModuleStreamBeginLumi";} ) );
		activityRegister.watchPreModuleStreamEndLumi( std::bind( &MemoryCounterPimple::enableMemoryCounterForStreams, pImple_, std::placeholders::_2, []{return "ModuleStreamEndLumi";} ) );
		activityRegister.watchPostModuleStreamEndLumi( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrintForStreams, pImple_, std::placeholders::_2, []{return "ModuleStreamEndLumi";} ) );

		activityRegister.watchPreModuleGlobalBeginRun( std::bind( &MemoryCounterPimple::enableMemoryCounterForStreams, pImple_, std::placeholders::_2, []{return "ModuleGlobalBeginRun";} ) );
		activityRegister.watchPostModuleGlobalBeginRun( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrintForStreams, pImple_, std::placeholders::_2, []{return "ModuleGlobalBeginRun";} ) );
		activityRegister.watchPreModuleGlobalEndRun( std::bind( &MemoryCounterPimple::enableMemoryCounterForStreams, pImple_, std::placeholders::_2, []{return "ModuleGlobalEndRun";} ) );
		activityRegister.watchPostModuleGlobalEndRun( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrintForStreams, pImple_, std::placeholders::_2, []{return "ModuleGlobalEndRun";} ) );

		activityRegister.watchPreModuleGlobalBeginLumi( std::bind( &MemoryCounterPimple::enableMemoryCounterForStreams, pImple_, std::placeholders::_2, []{return "ModuleGlobalBeginLumi";} ) );
		activityRegister.watchPostModuleGlobalBeginLumi( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrintForStreams, pImple_, std::placeholders::_2, []{return "ModuleGlobalBeginLumi";} ) );
		activityRegister.watchPreModuleGlobalEndLumi( std::bind( &MemoryCounterPimple::enableMemoryCounterForStreams, pImple_, std::placeholders::_2, []{return "ModuleGlobalEndLumi";} ) );
		activityRegister.watchPostModuleGlobalEndLumi( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrintForStreams, pImple_, std::placeholders::_2, []{return "ModuleGlobalEndLumi";} ) );

		activityRegister.watchPostGlobalEndRun( [&](edm::GlobalContext const&){++pImple_->runNumber_;} );
		activityRegister.watchPostGlobalEndLumi( [&](edm::GlobalContext const&){++pImple_->lumiNumber_;} );
#else
		activityRegister.watchPreModuleBeginRun( std::bind( &MemoryCounterPimple::enableMemoryCounter, pImple_, std::placeholders::_1, [&]{return "beginRun"+std::to_string(pImple_->runNumber_);} ) );
		activityRegister.watchPostModuleBeginRun( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrint, pImple_, std::placeholders::_1, [&]{return "beginRun"+std::to_string(pImple_->runNumber_);} ) );

		activityRegister.watchPreModuleBeginLumi( std::bind( &MemoryCounterPimple::enableMemoryCounter, pImple_, std::placeholders::_1, [&]{return "beginLumi"+std::to_string(pImple_->lumiNumber_);} ) );
		activityRegister.watchPostModuleBeginLumi( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrint, pImple_, std::placeholders::_1, [&]{return "beginLumi"+std::to_string(pImple_->lumiNumber_);} ) );

		activityRegister.watchPreModule( std::bind( &MemoryCounterPimple::enableMemoryCounter, pImple_, std::placeholders::_1, [&]{return "event"+std::to_string(pImple_->eventNumber_);} ) );
		activityRegister.watchPostModule( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrint, pImple_, std::placeholders::_1, [&]{return "event"+std::to_string(pImple_->eventNumber_);} ) );
		activityRegister.watchPostProcessEvent( [&](const edm::Event&,const edm::EventSetup&){++pImple_->eventNumber_;} );

		activityRegister.watchPreModuleEndLumi( std::bind( &MemoryCounterPimple::enableMemoryCounter, pImple_, std::placeholders::_1, [&]{return "endLumi"+std::to_string(pImple_->lumiNumber_);} ) );
		activityRegister.watchPostModuleEndLumi( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrint, pImple_, std::placeholders::_1, [&]{return "endLumi"+std::to_string(pImple_->lumiNumber_);} ) );
		activityRegister.watchPostEndLumi( [&](edm::LuminosityBlock const&, edm::EventSetup const&){++pImple_->lumiNumber_;} );

		activityRegister.watchPreModuleEndRun( std::bind( &MemoryCounterPimple::enableMemoryCounter, pImple_, std::placeholders::_1, [&]{return "endRun"+std::to_string(pImple_->runNumber_);} ) );
		activityRegister.watchPostModuleEndRun( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrint, pImple_, std::placeholders::_1, [&]{return "endRun"+std::to_string(pImple_->runNumber_);} ) );
		activityRegister.watchPostEndRun( [&](edm::Run const&, edm::EventSetup const&){++pImple_->runNumber_;} );
#endif
		activityRegister.watchPreModuleEndJob( std::bind( &MemoryCounterPimple::enableMemoryCounter, pImple_, std::placeholders::_1, []{return "endJob";} ) );
		activityRegister.watchPostModuleEndJob( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrint, pImple_, std::placeholders::_1, []{return "endJob";} ) );
	}
	else
	{
		std::cerr << " ***" << "\n"
				<< " *** MemoryCounter: couldn't get the symbol in the analysing library. Are you running under intrusiveMemoryAnalyser? MemoryCounter will not function without it." << "\n"
				<< " ***                See https://github.com/mark-grimes/MemCounter for details on how to get intrusiveMemoryAnalyser installed." << "\n"
				<< " ***" << std::endl;
	}
}

markstools::services::MemoryCounter::~MemoryCounter()
{
	delete pImple_;
}

void markstools::services::MemoryCounterPimple::enableMemoryCounter( const edm::ModuleDescription& description, std::function<std::string()> methodNameFunctor )
{
	const std::string& methodName=methodNameFunctor();

	auto iModuleDetails=memoryCounters_.find( description.moduleLabel() );
	if( iModuleDetails!=memoryCounters_.end() )
	{
		iModuleDetails->second.pMemoryCounter->resetMaximum();
		if( iModuleDetails->second.previousRecordedSize!=-1 ) iModuleDetails->second.previousRecordedSize-=iModuleDetails->second.pMemoryCounter->currentSize();
		iModuleDetails->second.pMemoryCounter->enable();

		if( verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" in method " << methodName << "." << std::endl;
	}
}

void markstools::services::MemoryCounterPimple::disableMemoryCounterAndPrint( const edm::ModuleDescription& description, std::function<std::string()> methodNameFunctor )
{
	const std::string& methodName=methodNameFunctor();

	auto iModuleDetails=memoryCounters_.find( description.moduleLabel() );
	if( iModuleDetails!=memoryCounters_.end() )
	{
		memcounter::IMemoryCounter* pMemoryCounter=iModuleDetails->second.pMemoryCounter;
		pMemoryCounter->disable();

		std::cout << " *MEMCOUNTER* " << methodName << "," << description.moduleLabel() << "," << description.moduleName()
				<< "," << pMemoryCounter->currentSize() << "," << pMemoryCounter->maximumSize()
				<< "," << pMemoryCounter->currentNumberOfAllocations() << "," << pMemoryCounter->maximumNumberOfAllocations();
		if( iModuleDetails->second.previousRecordedSize!=-1 ) std::cout << "," << iModuleDetails->second.previousEvent
				<< "," << iModuleDetails->second.previousRecordedSize;
		std::cout << std::endl;

		iModuleDetails->second.previousRecordedSize=pMemoryCounter->currentSize();
		iModuleDetails->second.previousEvent=methodName;
	}
}

void markstools::services::MemoryCounterPimple::preModuleConstruction( const edm::ModuleDescription& description )
{
	// If the vector is empty then I went to analyse all the modules. Otherwise check to see if the module name is in the list of modules to be analysed
	if( modulesToAnalyse_.empty() || std::find( modulesToAnalyse_.begin(), modulesToAnalyse_.end(), description.moduleLabel() )!=modulesToAnalyse_.end() )
	{
		memcounter::IMemoryCounter* pMemoryCounter=createNewMemoryCounter();
		if( pMemoryCounter )
		{
			memoryCounters_.insert( std::make_pair(description.moduleLabel(),::ModuleDetails(pMemoryCounter)) );
			if( verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" of type \"" << description.moduleName() << "\"." << std::endl;
			pMemoryCounter->resetMaximum();
			pMemoryCounter->enable();
		}
		else std::cout << "Couldn't get a pointer for a new IMemoryAnalyser, so module \"" << description.moduleLabel() << "\" will not be analysed." << std::endl;
	}
	else std::cout << "MemCounter not enabled for module \"" << description.moduleLabel() << "\"." << std::endl;
}
