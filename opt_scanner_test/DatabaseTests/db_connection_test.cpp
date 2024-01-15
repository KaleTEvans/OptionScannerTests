#define _CRT_SECURE_NO_WARNINGS

#include "SQLSchema.h"
#include "CandleRoutes.h"
#include "AlertRoutes.h"
#include "../pch.h"

#include "../MockClasses/MockClient.h"
#include "../MockClasses/MockWrapper.h"

using namespace testing;

// Test to create, send, and receive single rows from SQL tables
TEST(DBTests, candleTables) {
	nanodbc::connection conn = OptionDB::connectToDB();

	OptionDB::resetTables(conn);

	OptionDB::UnixTable::setTable(conn);

	auto currentTime = std::chrono::system_clock::now().time_since_epoch();

	// Convert milliseconds to seconds (Unix timestamp is in seconds)
	std::time_t unixTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
	long time = static_cast<long>(unixTime);
	std::cout << "TIME: " << time << std::endl;

	OptionDB::UnixTable::post(conn, time);

	OptionDB::OptionTable::setTable(conn);

	std::shared_ptr<Candle> c = std::make_shared<Candle>(4800, time, 5.60, 5.90, 5.25, 5.55, 125);
	std::shared_ptr<CandleTags> ct = std::make_shared<CandleTags>(c, TimeFrame::FiveSecs, Alerts::OptionType::Call,
		Alerts::TimeOfDay::Hour1, Alerts::VolumeStDev::Over1, Alerts::VolumeThreshold::Vol100, Alerts::PriceDelta::Under1,
		Alerts::DailyHighsAndLows::Inside, Alerts::LocalHighsAndLows::NLH);

	ct->addUnderlyingTags(Alerts::RelativeToMoney::ITM1, Alerts::PriceDelta::Over2,
		Alerts::DailyHighsAndLows::Inside, Alerts::LocalHighsAndLows::NLH);

	OptionDB::OptionTable::post(conn, ct);

	OptionDB::UnderlyingTable::setTable(conn);

	std::string date = c->date();

	OptionDB::UnderlyingTable::CandleForDB underlying(1234, date, time, 4800.05, 4810, 4750, 4790, 0);
	OptionDB::UnderlyingTable::post(conn, underlying, TimeFrame::FiveSecs);

	// Sleep for a bit and then begin retrieving data
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	std::vector<long> unix = OptionDB::UnixTable::get(conn);

	EXPECT_EQ(unixTime, unix[0]);

	std::vector<Candle> candles = OptionDB::UnderlyingTable::get(conn, TimeFrame::FiveSecs);
	Candle c1 = candles[0];

	EXPECT_TRUE(c1.reqId() == 1234);
	EXPECT_TRUE(c1.date() == date);
	EXPECT_TRUE(c1.time() == time);
	EXPECT_TRUE(c1.open() == 4800.05);
	EXPECT_TRUE(c1.high() == 4810);
	EXPECT_TRUE(c1.low() == 4750);
	EXPECT_TRUE(c1.close() == 4790);
	EXPECT_TRUE(c1.volume() == 0);
	
	std::vector<CandleTags> ctags = OptionDB::OptionTable::get(conn);
	CandleTags ct1 = ctags[0];

	EXPECT_TRUE(ct1.candle.reqId() == 4800);
	EXPECT_TRUE(ct1.candle.date() == date);
	EXPECT_TRUE(ct1.candle.time() == time);
	EXPECT_TRUE(ct1.candle.open() == 5.60);
	EXPECT_TRUE(ct1.candle.high() == 5.90);
	EXPECT_TRUE(ct1.candle.low() == 5.25);
	EXPECT_TRUE(ct1.candle.close() == 5.55);
	EXPECT_TRUE(ct1.candle.volume() == 125);
	EXPECT_TRUE(ct1.getTimeFrame() == TimeFrame::FiveSecs);
	EXPECT_TRUE(ct1.getOptType() == Alerts::OptionType::Call);
	EXPECT_TRUE(ct1.getTOD() == Alerts::TimeOfDay::Hour1);
	EXPECT_TRUE(ct1.getRTM() == Alerts::RelativeToMoney::ITM1);
	EXPECT_TRUE(ct1.getVolStDev() == Alerts::VolumeStDev::Over1);
	EXPECT_TRUE(ct1.getVolThresh() == Alerts::VolumeThreshold::Vol100);
	EXPECT_TRUE(ct1.getOptPriceDelta() == Alerts::PriceDelta::Under1);
	EXPECT_TRUE(ct1.getDHL() == Alerts::DailyHighsAndLows::Inside);
	EXPECT_TRUE(ct1.getLHL() == Alerts::LocalHighsAndLows::NLH);
	EXPECT_TRUE(ct1.getUnderlyingPriceDelta() == Alerts::PriceDelta::Over2);
	EXPECT_TRUE(ct1.getUnderlyingDHL() == Alerts::DailyHighsAndLows::Inside);
	EXPECT_TRUE(ct1.getUnderlyingLHL() == Alerts::LocalHighsAndLows::NLH);
}

//TEST(DBTests, setTagTable) {
//	nanodbc::connection conn = OptionDB::connectToDB();
//
//	OptionDB::AlertTables::setTagTable(conn);
//}