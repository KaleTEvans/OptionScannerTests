#define _CRT_SECURE_NO_WARNINGS

#include "../pch.h"

#include "../MockClasses/MockClient.h"
#include "../MockClasses/MockWrapper.h"
#include "../MockClasses/MockOptionScanner.h"
#include "ContractData.h"

#include <thread>

using namespace testing;


// Repurposing tests just to confirm tags and candles are being routed properly
TEST(MockOptionScannerTests, OptionChainGenerationTest) {
	// Set random generator to generate every 100 miliseconds
	MockOptionScanner mos(1000, true);

	std::thread t([&] {
		mos.streamOptionData();
		});

	// New SPX price is used to update strikes every 100 miliseconds, so only sleep a short amount of time
	std::this_thread::sleep_for(std::chrono::milliseconds(10000));

	mos.YW.setmDone(true);
	if (t.joinable()) t.join();
	mos.waitForDBM();

	int underlyingCandleCt = mos.getUnderlyingDBCount();
	int optionCandleCt = mos.getOptionDBCount();

	std::shared_ptr<std::unordered_map<int, std::shared_ptr<ContractData>>> contractChain = mos.getContractChain();

	int underlyingChainCt = 0;
	underlyingChainCt += contractChain->at(1234)->fiveSecData().size();
	underlyingChainCt += contractChain->at(1234)->thirtySecData().size();
	underlyingChainCt += contractChain->at(1234)->oneMinData().size();
	underlyingChainCt += contractChain->at(1234)->fiveMinData().size();

	EXPECT_EQ(underlyingCandleCt, underlyingChainCt);

	int optionChainCt = 0;
	for (auto& i : *contractChain) {
		if (i.first != 1234) {
			//std::cout << i.first << std::endl;
			optionChainCt += i.second->fiveSecData().size();
			optionChainCt += i.second->thirtySecData().size();
			optionChainCt += i.second->oneMinData().size();
			optionChainCt += i.second->fiveMinData().size();
		}
	}

	EXPECT_EQ(optionCandleCt, optionChainCt);
}

// Output test to ensure updates are made with mutexes and condition variables in place
// Also ensuring buffer matches the option chain size
//TEST(MockOptionScannerTests, SPXPriceUpdateTest) {
//	MockOptionScanner mos(25);
//
//	std::thread t([&] {
//		mos.streamOptionData();
//		});
//
//	for (size_t i = 0; i < 10; i++) {
//		// Lock the option scanner mutex to wait for updated data
//		std::unique_lock<std::mutex> lock(mos.optScanMutex_);
//		mos.mosCnditional.wait(lock, [&] { return mos.strikesUpdated; });
//
//		// std::cout << "Contract Map Size: " << mos.checkContractMap() << std::endl;
//		// std::cout << "Diff Chain Size: " << mos.diffChainSize() << std::endl;
//		// std::cout << "Current SPX: " << mos.currentSPX() << std::endl;
//
//		EXPECT_EQ(mos.prevBufferCapacity(), mos.checkContractMap());
//
//		// Reset strikesUpdated and wait for next update
//		mos.strikesUpdated = false;
//
//		lock.unlock();
//
//		if (i == 9) {
//			mos.YW.setmDone(true);
//			t.join();
//		}
//	}
//
//	EXPECT_EQ(mos.YW.getBufferCapacity(), mos.YW.checkActiveReqs());
//	EXPECT_EQ(mos.YW.getBufferCapacity(), mos.finalContractCt().size());
//
//	std::vector<int> optScanReqs = mos.finalContractCt();
//	std::unordered_set<int> wrapperReqs = mos.YW.getActiveReqs();
//
//	for (auto i : optScanReqs) {
//		EXPECT_TRUE(wrapperReqs.find(i) != wrapperReqs.end());
//	}
//}


//////////////////////////////////////////////////////////
// Alert Tests

// Test alert callbacks from contract data and ensure correct data is returned to Option Scanner
//TEST(MockOptionScannerTests, AlerCallbackTests) {
//	MockOptionScanner mos(10);
//
//	std::thread t([&] {
//		mos.streamOptionData();
//	});
//
//	for (size_t i = 0; i < 50; i++) {
//		// Lock the option scanner mutex to wait for updated data
//		std::unique_lock<std::mutex> lock(mos.optScanMutex_);
//		mos.mosCnditional.wait(lock, [&] { return mos.strikesUpdated; });
//
//		if (!mos.alertQueue.empty()) {
//			while (!mos.alertQueue.empty()) {
//				Alert a = mos.alertQueue.front();
//				double stDevVol = a.cd->volStDev(a.tf).numStDev(a.candle->volume());
//				if (a.tf == TimeFrame::FiveSecs) {
//					EXPECT_TRUE(stDevVol > 2);
//				}
//				else if (a.tf == TimeFrame::ThirtySecs) {
//					EXPECT_TRUE(stDevVol > 1.5);
//				}
//				else {
//					EXPECT_TRUE(stDevVol > 1);
//				}
//
//				mos.alertQueue.pop();
//			}
//		}
//
//		// Reset strikesUpdated and wait for next update
//		mos.strikesUpdated = false;
//		lock.unlock();
//	}
//
//	mos.YW.setmDone(true);
//	t.join();
//}