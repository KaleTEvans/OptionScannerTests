#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "SQLSchema.h"
#include "CandleRoutes.h"
#include "AlertRoutes.h"

#include <memory>
#include <queue>
#include <map>
#include <unordered_set>
#include <map>

// Comparator for OptionCandle min heap 
struct candleTimeComparator {
	int operator() (const std::shared_ptr<CandleTags> ct1, const std::shared_ptr<CandleTags> ct2) {
		return ct1->candle.time() > ct2->candle.time();
	}
};

namespace OptionDB {

	class DatabaseManager {
	public:
		DatabaseManager(bool testConfigNoDB);

		void start();
		void stop();
		bool processingComplete() const;

		void addToInsertionQueue(std::shared_ptr<CandleTags> ct);
		void addToInsertionQueue(std::shared_ptr<Candle> c, TimeFrame tf);

		int getUnderlyingCount();
		int getOptionCount();

		void setCandleTables();
		void setAlertTables();

		// Just output data, don't send to db
		bool testConfigNoDB = true;

		std::mutex& getMtx();
		std::condition_variable& getCV();

	private:
		void candleInsertionLoop();

		std::shared_ptr<nanodbc::connection> conn_;

		std::thread dbInsertionThread;
		std::mutex queueMtx;
		std::condition_variable cv;
		bool stopInsertion{ false };
		bool processingComplete_{ false };

		// Processing containers
		std::queue<std::pair<UnderlyingTable::CandleForDB, TimeFrame>> underlyingQueue;
		std::queue<std::shared_ptr<CandleTags>> candleProcessingQueue;
		std::map<long, std::queue<std::shared_ptr<CandleTags>>> candleQueue;
		std::priority_queue<std::shared_ptr<CandleTags>, std::vector<std::shared_ptr<CandleTags>>, candleTimeComparator> candlePriorityQueue;

		// Keeps track of each time increment
		std::unordered_set<long> timeSet;
	};
}