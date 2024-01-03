#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "SQLSchema.h"
#include "CandleRoutes.h"
#include "AlertRoutes.h"

#include <memory>
#include <queue>
#include <map>

namespace OptionDB {

	class DatabaseManager {
	public:
		DatabaseManager();

		void start();
		void stop();

		void addToInsertionQueue(std::shared_ptr<CandleTags> ct);
		void addToInsertionQueue(std::shared_ptr<Candle> c, TimeFrame tf);

		void setCandleTables();
		void setAlertTables();

	private:
		void candleInsertionLoop();

		std::shared_ptr<nanodbc::connection> conn_;

		std::thread dbInsertionThread;
		std::mutex queueMtx;
		std::condition_variable cv;
		bool stopInsertion{ false };

		// Processing containers
		std::queue<std::pair<UnderlyingTable::CandleForDB, TimeFrame>> underlyingQueue;
		std::queue<std::shared_ptr<CandleTags>> candleProcessingQueue;
	};
}