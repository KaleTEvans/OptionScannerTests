#define _CRT_SECURE_NO_WARNINGS

#include "SQLSchema.h"
#include "CandleRoutes.h"
#include "AlertRoutes.h"
#include "ContractData.h"
#include "../pch.h"

#include "../MockClasses/MockClient.h"
#include "../MockClasses/MockWrapper.h"

using namespace testing;

// Rather than use the mock option scanner, this test will use several manually created candles,
// to ensure accuracy for all tags 
TEST(TagInsertionTests, TagAccuracy) {
	// Baseline candles
	// These will serve as starting points to make copies of and be modified to test edge cases
	
	long time = 1704996306; // Expected to be output as hour 4
	// For underlying, use two repeating values for high and low prices to create a small standard deviation
	double uHigh1 = 4806;
	double uLow1 = 4801; // diff of 5
	double uHigh2 = 4803;
	double uLow2 = 4793; // diff of 10, will create mean delta of 7.5 and stdev of 2.7
	// Outlier values
	double uHigh3 = 4825;
	double uLow3 = 4801; // diff from mean is 3.5, over 1 stdev
	double uHigh4 = 4813.5;
	double uLow4 = 4801; // diff from mean is 5, under 2 stdev

	long uTime = time;

	std::unique_ptr<Candle> c1 = std::make_unique<Candle>(1234, uTime, 4800, uHigh1, uLow1, 4804, 0);
	ContractData cd(1234);
	cd.updateData(std::move(c1));

	// Create 720 Candles for an hour of data
	for (int i = 1; i <= 720; i++) {
		std::unique_ptr<Candle> c;
		if (i == 300) {
			c = std::make_unique<Candle>(1234, uTime, 4800, uHigh3, uLow3, 4804, 0);
		}
		else if (i == 450) {
			c = std::make_unique<Candle>(1234, uTime, 4800, uHigh4, uLow4, 4804, 0);
		}
		else {
			if (i % 2 == 0) c = std::make_unique<Candle>(1234, uTime, 4800, uHigh1, uLow1, 4804, 0);
			else c = std::make_unique<Candle>(1234, uTime, 4800, uHigh2, uLow2, 4804, 0);
		}

		double high = c->high();
		double low = c->low();

		cd.updateData(std::move(c));
		time += 5;

		double numStDev = cd.priceStDev(TimeFrame::FiveSecs).numStDev(high - low);
		//std::cout << "High - Low " << high - low << std::endl;
		std::cout << "Num St Dev: " << numStDev << std::endl;

		if (numStDev < 1) {
			EXPECT_TRUE(cd.priceDelta(TimeFrame::FiveSecs) == Alerts::PriceDelta::Under1);
		}
		else if (numStDev >= 1 && numStDev <= 2) {
			EXPECT_TRUE(cd.priceDelta(TimeFrame::FiveSecs) == Alerts::PriceDelta::Under2);
		}
		else if (numStDev > 2) {
			EXPECT_TRUE(cd.priceDelta(TimeFrame::FiveSecs) == Alerts::PriceDelta::Over2);
		}
	}
}