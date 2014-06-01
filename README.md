cmssw_Benchmarking
==================

Benchmarking utilities for CMSSW - memory use and time split by module.
To include in a CMSSW build (assuming you have initialised it for git somehow):

    cd $CMSSW_BASE/src
    git clone git@github.com:mark-grimes/ cmssw_Benchmarking.git MarksTools/Benchmarking

Analysing memory use requires starting cmsRun under the standalone utility MemCounter available from https://github.com/mark-grimes/MemCounter. Note that MemCounter doesn't work with jemalloc (the default for cmsRun). **You have to run with cmsRunGlibC**. For example:

    intrusiveMemoryAnalyser cmsRunGlibC myConfigFile_cfg.py

To have either of the analysers running, you need to have

    process.MemoryCounter = cms.Service( "MemoryCounter" )
    process.ModuleTimer = cms.Service( "ModuleTimer" )

somewhere in your config file. It is not recommended to have both running at the same time, MemoryCounter will likely give you erroneosly large results for ModuleTimer.
