#define _CRT_SECURE_NO_WARNINGS

#include "SQLSchema.h"
#include "CandleRoutes.h"
#include "AlertRoutes.h"
#include "../pch.h"

#include "../MockClasses/MockClient.h"
#include "../MockClasses/MockWrapper.h"

using namespace testing;

TEST(DBTests, connectToDB) {
	//nanodbc::connection conn = OptionDB::connectToDB();

	// Create some historic candles with the mock wrapper
	MockWrapper mWrapper;
	MockClient mClient(mWrapper);

	double spxRefPrice = 4580;
	long spxRefVol = 50000000;

	double priceRange = 20;
	long volumeRange = 1000000;

	Contract con;
	con.symbol = "SPX";
	con.secType = "IND";

	std::string endDateTime = "20230705 09:30:00";
	std::string durationStr = "600 S";
	std::string barSizeSetting = "5 secs";

	mClient.reqHistoricalData(1234, con, endDateTime, durationStr, barSizeSetting, "", 0, 0, false);

	while (mWrapper.notDone()) continue;

	auto data = mWrapper.getHistoricCandles();
	//std::vector<std::shared_ptr<Candle>> testData;;

	for (auto& c : data) {
		std::cout << c->reqId() << "|" << c->date() << "|" << c->time() << "|" << c->open() << "|" <<
			c->high() << "|" << c->low() << "|" << c->close() << "|" << c->volume() << std::endl;
	}
}

//TEST(DBTests, setTagTable) {
//	nanodbc::connection conn = OptionDB::connectToDB();
//
//	OptionDB::AlertTables::setTagTable(conn);
//}